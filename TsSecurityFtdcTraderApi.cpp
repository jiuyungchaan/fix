////////////////////////
///@author Kenny Chiu
///@date 20170120
///@summary Implementation of CTsSecurityFtdcTraderApi and ImplTsFtdcTraderApi
///         
///
////////////////////////

#include "TsSecurityFtdcTraderApi.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <cctype>
#include <algorithm>
#include <vector>


const char *second_now();
void split(const std::string& str, const std::string& del, std::vector<std::string>& v);

class ImplTsFtdcTraderApi : public CTsSecurityFtdcTraderApi{
 public:
  explicit ImplTsFtdcTraderApi(const char *configPath);

  // interfaces of CTsSecurityFtdcTraderApi
  void Release();
  void Init();
  int Join();
  const char *GetTradingDay();
  void RegisterFront(char *pszFrontAddress);
  void RegisterSpi(CTsSecurityFtdcTraderSpi *pSpi);
  void SubscribePrivateTopic(SECURITY_TE_RESUME_TYPE nResumeType);
  void SubscribePublicTopic(SECURITY_TE_RESUME_TYPE nResumeType);
  int ReqUserLogin(CSecurityFtdcReqUserLoginField *pReqUserLoginField,
                   int nRequestID);
  int ReqUserLogout(CSecurityFtdcUserLogoutField *pUserLogout,
                    int nRequestID);
  int ReqFetchAuthRandCode(CSecurityFtdcAuthRandCodeField *pAuthRandCode,
                           int nRequestID);
  int ReqOrderInsert(CSecurityFtdcInputOrderField *pInputOrder,
                     int nRequestID);
  int ReqOrderAction(CSecurityFtdcInputOrderActionField *pInputOrderAction,
                     int nRequestID);

 protected:
  virtual ~ImplTsFtdcTraderApi();

 private:
  class InputOrder {
   public:
    struct SubTrade {
      int quantity;
      double price;
      char time[32];
      SubTrade() : quantity(0), price(0.0), time{0} {}
      SubTrade(int vol, double prc) : quantity(vol), price(prc), time{0} {}
    };

    CSecurityFtdcInputOrderField basic_order;
    char client_id[64];
    SubTrade *trades[100];
    int trade_size;

    InputOrder();
    InputOrder(CSecurityFtdcInputOrderField *pInputOrder);
    void add_trade(int volume, double price);
  };

  class OrderPool {
   public:
    static const int MAX_ORDER_SIZE = 900000;

    OrderPool();
    InputOrder *get(int order_id);
    InputOrder *get(std::string sys_id);
    InputOrder *add(CSecurityFtdcInputOrderField *pInputOrder);
    void add_pair(std::string sys_id, int local_id);
    bool has_order(std::string sys_id);

   private:
    InputOrder *order_pool_[MAX_ORDER_SIZE];
    std::map<std::string, int> sys_to_local_;
  };

  static void *recv_thread(void *obj);
  static void *callback_onlogin(void *obj);
  static void *callback_onlogout(void *obj);
  static void *callback_onrandcode(void *obj);

  void decode(const char *message);

  // typedef std::map<std::string, std::string> stringmap;

  CSecurityFtdcOrderField ToOrderField(std::map<std::string, std::string>& properties);
  CSecurityFtdcTradeField ToTradeField(std::map<std::string, std::string>& properties);
  CSecurityFtdcInputOrderField ToInputOrderField(std::map<std::string, std::string>& properties);
  CSecurityFtdcInputOrderActionField ToActionField(std::map<std::string, std::string>& properties);
  CSecurityFtdcRspInfoField ToRspField(std::map<std::string, std::string>& properties);

  CTsSecurityFtdcTraderSpi *trader_spi_;
  char front_addr_[64];
  char user_id_[64];
  char user_passwd_[64];

  int server_fd_;
  struct sockaddr_in server_addr_;

  pthread_t socket_thread_;

  OrderPool order_pool_;
  std::fstream log_file_;
};


/* ===============================================
   =============== IMPLEMENTATION ================ */

using namespace std;

CTsSecurityFtdcTraderApi *CTsSecurityFtdcTraderApi::CreateFtdcTraderApi(const char *configPath) {
  ImplTsFtdcTraderApi *api = new ImplTsFtdcTraderApi(configPath);
  return (CTsSecurityFtdcTraderApi *)api;
}

ImplTsFtdcTraderApi::InputOrder::InputOrder() : client_id{0}, trade_size(0) {
  memset(&basic_order, 0, sizeof(basic_order));
  memset(trades, 0, sizeof(trades));
}

ImplTsFtdcTraderApi::InputOrder::InputOrder(
      CSecurityFtdcInputOrderField *pInputOrder) : client_id{0}, trade_size(0) {
  memcpy(&basic_order, pInputOrder, sizeof(basic_order));
  memset(trades, 0, sizeof(trades));
}

void ImplTsFtdcTraderApi::InputOrder::add_trade(int volume, double price) {
  SubTrade *sub_trade = new SubTrade(volume, price);
  trades[trade_size++] = sub_trade;
}

ImplTsFtdcTraderApi::OrderPool::OrderPool() {
  memset(order_pool_, sizeof(order_pool_), 0);
}

ImplTsFtdcTraderApi::InputOrder *ImplTsFtdcTraderApi::OrderPool::get(
      int order_id) {
  if (order_id >= 0 && order_id < MAX_ORDER_SIZE) {
    return order_pool_[order_id];
  } else {
    return (ImplTsFtdcTraderApi::InputOrder *)NULL;
  }
}

ImplTsFtdcTraderApi::InputOrder *ImplTsFtdcTraderApi::OrderPool::get(
      string sys_id) {
  map<string, int>::iterator it = sys_to_local_.find(sys_id);
  if (it == sys_to_local_.end()) {
    printf("SysOrderID-%s not found!\n", sys_id.c_str());
    return (ImplTsFtdcTraderApi::InputOrder *)NULL;
  } else {
    return get(it->second);
  }
}

bool ImplTsFtdcTraderApi::OrderPool::has_order(string sys_id) {
  map<string, int>::iterator it = sys_to_local_.find(sys_id);
  return (it != sys_to_local_.end());
}

ImplTsFtdcTraderApi::InputOrder *ImplTsFtdcTraderApi::OrderPool::add(
      CSecurityFtdcInputOrderField *pInputOrder) {
  ImplTsFtdcTraderApi::InputOrder *input_order = new
      ImplTsFtdcTraderApi::InputOrder(pInputOrder);
  int order_id = atoi(pInputOrder->OrderRef) / 100;
  if (order_id < 0 || order_id >= MAX_ORDER_SIZE) {
    printf("Invalid Order Ref:%s\n", pInputOrder->OrderRef);
  } else {
    if (order_pool_[order_id] == NULL) {
      order_pool_[order_id] = input_order;
    } else {
      printf("Overwrite InputOrder in Position-%d\n", order_id);
      order_pool_[order_id] = input_order;
    }
  }
  return input_order;
}

void ImplTsFtdcTraderApi::OrderPool::add_pair(string sys_id, int local_id) {
  map<string, int>::iterator it = sys_to_local_.find(sys_id);
  if (it != sys_to_local_.end()) {
    printf("SysOrderID-%s already match for %d !\n", sys_id.c_str(),
           it->second);
  } else {
    sys_to_local_.insert(std::pair<std::string, int>(sys_id, local_id));
  }
}

ImplTsFtdcTraderApi::ImplTsFtdcTraderApi(const char *pszFlowPath) :
    front_addr_{0}, user_id_{0}, user_passwd_{0} {
  char log_file_name[128];
  if (strcmp(pszFlowPath, "") == 0) {
    snprintf(log_file_name, sizeof(log_file_name), "ts.log");
  } else {
    snprintf(log_file_name, sizeof(log_file_name), "%s.ts.log", pszFlowPath);
  }
#ifndef __DEBUG__
  log_file_.open(log_file_name, fstream::out | fstream::app);
#endif
}

ImplTsFtdcTraderApi::~ImplTsFtdcTraderApi() {
  // TODO
}

void *ImplTsFtdcTraderApi::recv_thread(void *obj) {
  ImplTsFtdcTraderApi *self = (ImplTsFtdcTraderApi *)obj;
  int fd = self->server_fd_;
  int recv_len;
  char buffer[2048];
  while((recv_len = recv(fd, buffer, 2048, 0)) > 0) {
    buffer[recv_len] = '\0';
    printf("Data total received-[%d]\n", recv_len);
    char *start = buffer;
    while (true) {
      char *p = strchr(start, '\0');
      if (p == NULL) 
        break;
      if (p - buffer < recv_len - 1) {
        printf("Multi datagram-[%ld]:%s\n", p - start, start);
        self->decode(start);
        start = p + 1;
      } else {
        printf("Last datagram-[%ld]:%s\n", p - start, start);
        self->decode(start);
        break;
      }
    }  // while
  }
  printf("Failed to receive data! Exit thread...\n");
  return (void *)NULL;
}

void ImplTsFtdcTraderApi::decode(const char *message) {
  vector<string> pairs;
  split(string(message), ";", pairs);
  if (pairs.size() < 1) {
    return;
  }

  map<string, string> properties;
  vector<string> key_val;
  for(size_t i = 0; i < pairs.size(); i++) {
    key_val.clear();
    split(pairs[i], "=", key_val);
    if (key_val.size() != 2) {
#ifndef __DEBUG__
      log_file_ << "Invalid key value: " << pairs[i] << endl;
#endif  // __DEBUG__
      continue;
    }
    properties[key_val[0]] = key_val[1];
  }
  
  string msg_type = properties["TYPE"];

  if (msg_type == "UPDATE") {
    string status = properties["STATUS"];
    if (status == "SENDING") {
      // NOTHING TO DO
    } else if (status == "QUEUED") {
      // NOTHING TO DO
    } else if (status == "RECEIVED") {
      // OnRtnOrder
      // if RECEIVED is already received, this must be a cancel received confirmation.
      if (!order_pool_.has_order(properties["SYS_ORDER_ID"])) {
        CSecurityFtdcOrderField order_field = ToOrderField(properties);
        // add LocalOrderID&SysOrderID in the OrderPool?
        int local_id = strtol(order_field.OrderRef, NULL, 10)/100;
        order_pool_.add_pair(properties["SYS_ORDER_ID"], local_id);
        trader_spi_->OnRtnOrder(&order_field);
      }
    } else if (status == "PARTIALLYFILLED") {
      // OnRtnTrade
      if (properties["AVG_FILLED_PRICE"] != "" && 
          properties["AVG_FILLED_PRICE"] != "0.0") {
        CSecurityFtdcTradeField trade_field = ToTradeField(properties);
        trader_spi_->OnRtnTrade(&trade_field);
      }  // if AVG_FILLED_PRICE == 0.0, it is a cancel request confirmation
    } else if (status == "PARTIALLYFILLEDUROUT") {
      // OnRtnOrder -- success cancel
      CSecurityFtdcOrderField order_field = ToOrderField(properties);
      trader_spi_->OnRtnOrder(&order_field);
    } else if (status == "FILLED") {
      // OnRtnTrade and OnRtnOrder
      CSecurityFtdcTradeField trade_field = ToTradeField(properties);
      CSecurityFtdcOrderField order_field = ToOrderField(properties);
      trader_spi_->OnRtnTrade(&trade_field);
      trader_spi_->OnRtnOrder(&order_field);
    } else if (status == "CANCELED") {
      CSecurityFtdcOrderField order_field = ToOrderField(properties);
      trader_spi_->OnRtnOrder(&order_field);
    } else if (status == "REJECTED") {
      CSecurityFtdcInputOrderField order_field = ToInputOrderField(properties);
      CSecurityFtdcRspInfoField info_field = ToRspField(properties);
      trader_spi_->OnRspOrderInsert(&order_field, &info_field, 0, true);
    }
  } // if type == "UPDATE"
}

void ImplTsFtdcTraderApi::Release() {
  // TODO
}

void ImplTsFtdcTraderApi::Init() {
  // connect server
  if (strcmp(front_addr_, "") == 0) {
    printf("ERROR: Front Address is not registered!\n");
    return ;
  }
  vector<string> vals;
  split(string(front_addr_), "//", vals);
  if (vals.size() == 2) { 
    vector<string> addr;
    split(vals[1], ":", addr);
    if (addr.size() == 2) {
      int port = atoi(addr[1].c_str());
      server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
      if (server_fd_ == -1) {
        printf("Failed to create socket: %s-%d\n", strerror(errno), errno);
        exit(0);
      }
      char ip_addr[32];
      snprintf(ip_addr, sizeof(ip_addr), "%s", addr[0].c_str());
      memset(&server_addr_, 0, sizeof(server_addr_));
      server_addr_.sin_family = AF_INET;
      server_addr_.sin_port = htons(port);
      if (inet_pton(AF_INET, ip_addr, &server_addr_.sin_addr) <= 0) {
        printf("Failed to parse ip address: %s-%s-%d\n", ip_addr, 
               strerror(errno), errno);
        trader_spi_->OnFrontDisconnected(1001);
        return;
      }
      if (connect(server_fd_, (struct sockaddr*)&server_addr_, 
                  sizeof(server_addr_)) < 0) {
        printf("Failed to connect: [%s:%d]-%s-%d\n", ip_addr, port, 
               strerror(errno), errno);
        trader_spi_->OnFrontDisconnected(1001);
        return; 
      }
      trader_spi_->OnFrontConnected();

      int ret = pthread_create(&socket_thread_, NULL, recv_thread, (void *)this);
      if (ret != 0) {
        printf("Failed to create socket thread!\n");
        exit(0);
      }
      printf("Socket thread create successfully!\n");
    }
  } else {
    printf("ERROR: Invalid Front Address: %s\n", front_addr_);
    trader_spi_->OnFrontDisconnected(1001);   
  }
}

int ImplTsFtdcTraderApi::Join() {
  // dummy function to adapt CTP API
  return 0;
}

const char * ImplTsFtdcTraderApi::GetTradingDay() {
  static char timestamp_str[32];
  time_t rawtime;
  struct tm *timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  
  int date = (1900 + timeinfo->tm_year) * 10000 +
             (1 + timeinfo->tm_mon) * 100 +
             timeinfo->tm_mday;
  snprintf(timestamp_str, sizeof(timestamp_str), "%d", date);
  return timestamp_str;
}

void ImplTsFtdcTraderApi::RegisterFront(char *pszFrontAddress) {
  snprintf(front_addr_, sizeof(front_addr_), "%s", pszFrontAddress);
}


void ImplTsFtdcTraderApi::RegisterSpi(CTsSecurityFtdcTraderSpi *pSpi) {
  trader_spi_ = pSpi;
}

void ImplTsFtdcTraderApi::SubscribePrivateTopic(
      SECURITY_TE_RESUME_TYPE nResumeType) {
  // dummy function to adapt LTS API
}

void ImplTsFtdcTraderApi::SubscribePublicTopic(
      SECURITY_TE_RESUME_TYPE nResumeType) {
  // dummy function to adapt LTS API
}

void *ImplTsFtdcTraderApi::callback_onlogin(void *obj) {
  sleep(1);
  ImplTsFtdcTraderApi *self = (ImplTsFtdcTraderApi *)obj;
  CSecurityFtdcRspUserLoginField login_field;
  memset(&login_field, 0, sizeof(login_field));
  self->trader_spi_->OnRspUserLogin(&login_field, NULL, 0, true);  
  return (void *)NULL;
}

void *ImplTsFtdcTraderApi::callback_onlogout(void *obj) {
  sleep(1);
  ImplTsFtdcTraderApi *self = (ImplTsFtdcTraderApi *)obj;
  CSecurityFtdcUserLogoutField logout_field;
  memset(&logout_field, 0, sizeof(logout_field));
  CSecurityFtdcRspInfoField info_field;
  memset(&info_field, 0, sizeof(info_field));
  self->trader_spi_->OnRspUserLogout(&logout_field, &info_field, 0, true);  
  return (void *)NULL;
}

void *ImplTsFtdcTraderApi::callback_onrandcode(void *obj) {
  sleep(1);
  ImplTsFtdcTraderApi *self = (ImplTsFtdcTraderApi *)obj;
  CSecurityFtdcAuthRandCodeField code_field;
  memset(&code_field, 0, sizeof(code_field));
  snprintf(code_field.RandCode, sizeof(code_field.RandCode), "nonsense");
  CSecurityFtdcRspInfoField info_field;
  memset(&info_field, 0, sizeof(info_field));
  self->trader_spi_->OnRspFetchAuthRandCode(&code_field, &info_field, 0, true);
  return (void *)NULL;
}

int ImplTsFtdcTraderApi::ReqUserLogin(
      CSecurityFtdcReqUserLoginField *pReqUserLoginField, int nRequestID) {
  snprintf(user_id_, sizeof(user_id_), "%s", pReqUserLoginField->UserID);
  snprintf(user_passwd_, sizeof(user_passwd_), "%s", pReqUserLoginField->Password);
  pthread_t callback_thread;
  pthread_create(&callback_thread, NULL, callback_onlogin, (void *)this);
  return 0;
}

int ImplTsFtdcTraderApi::ReqUserLogout(
      CSecurityFtdcUserLogoutField *pUserLogout, int nRequestID) {
  close(server_fd_);
  pthread_t callback_thread;
  pthread_create(&callback_thread, NULL, callback_onlogout, (void *)this);
  return 0;
}

int ImplTsFtdcTraderApi::ReqFetchAuthRandCode(
      CSecurityFtdcAuthRandCodeField *pAuthRandCode, int nRequestID) {
  pthread_t callback_thread;
  pthread_create(&callback_thread, NULL, callback_onrandcode, (void *)this);
  return 0;
}

int ImplTsFtdcTraderApi::ReqOrderInsert(
      CSecurityFtdcInputOrderField *pInputOrder, int nRequestID) {
  InputOrder *input_order = order_pool_.add(pInputOrder);

  char action[16], order_type[16], symbol[16], duration[16], client_id[64];
  char message[1024];
  
  if (pInputOrder->Direction == SECURITY_FTDC_D_Buy) {
    if (pInputOrder->CombOffsetFlag[0] == SECURITY_FTDC_OF_Open) {
      // snprintf(action, sizeof(action), "BUY");
      snprintf(action, sizeof(action), "COLLATERALBUY");
    } else if (pInputOrder->CombOffsetFlag[0] == SECURITY_FTDC_OF_Close) {
      snprintf(action, sizeof(action), "BORROWTOBUY");
    } else if (pInputOrder->CombOffsetFlag[0] == SECURITY_FTDC_OF_CloseToday) {
      snprintf(action, sizeof(action), "BUYTOPAY");
    } else if (pInputOrder->CombOffsetFlag[0] == SECURITY_FTDC_OF_CloseYesterday) {
      snprintf(action, sizeof(action), "PAYBYCASH");
    }
  } else {
    if (pInputOrder->CombOffsetFlag[0] == SECURITY_FTDC_OF_Open) {
      // snprintf(action, sizeof(action), "SELL");
      snprintf(action, sizeof(action), "COLLATERALSELL");
    } else if (pInputOrder->CombOffsetFlag[0] == SECURITY_FTDC_OF_Close) {
      snprintf(action, sizeof(action), "BORROWTOSELL");
    } else if (pInputOrder->CombOffsetFlag[0] == SECURITY_FTDC_OF_CloseToday) {
      snprintf(action, sizeof(action), "SELLTOPAY");
    } else if (pInputOrder->CombOffsetFlag[0] == SECURITY_FTDC_OF_CloseYesterday) {
      snprintf(action, sizeof(action), "PAYBYSTOCK");
    }
  }

  snprintf(symbol, sizeof(symbol), "%s.%s", pInputOrder->InstrumentID, 
           pInputOrder->ExchangeID);

  snprintf(duration, sizeof(duration), "GFD");
  snprintf(order_type, sizeof(order_type), "LIMIT");
  snprintf(client_id, sizeof(client_id), "%s_%s", second_now(),
           pInputOrder->OrderRef);
  snprintf(input_order->client_id, sizeof(input_order->client_id), "%s",
           client_id);

  snprintf(message, sizeof(message), "COMMAND=SENDORDER;ACCOUNT=%s;ACTION=%s;" \
           "SYMBOL=%s;QUANTITY=%d;TYPE=%s;LIMITPRICE=%s;DURATION=%s;" \
           "CLIENTID=%s",
           user_id_, action, symbol, pInputOrder->VolumeTotalOriginal,
           order_type, pInputOrder->LimitPrice, duration, client_id);
  int ret = send(server_fd_, message, strlen(message), 0);
  cout << "Send message:" << message << endl;
  if (ret < 0) {
    printf("Failed to send message - %d\n", ret);
  }
  return ret;
}

int ImplTsFtdcTraderApi::ReqOrderAction(
      CSecurityFtdcInputOrderActionField *pInputOrderAction, int nRequestID) {
  char message[1024];

  int local_id = strtol(pInputOrderAction->OrderRef, NULL, 10)/100;
  InputOrder *input_order = order_pool_.get(local_id);
  int ret = 1;
  if (input_order != NULL) {
    snprintf(message, sizeof(message), "COMMAND=CANCELORDER;CLIENTID=%s",
             input_order->client_id);
    ret = send(server_fd_, message, strlen(message), 0);
    cout << "Send message:" << message << endl;
    if (ret < 0) {
      printf("Failed to send message - %d\n", ret);
    }
  } else {
    printf("Input Order LocalID=%s not found.\n", pInputOrderAction->OrderRef);
  }
  return ret;
}


// int ImplTsFtdcTraderApi::ReqQryInvestorPosition(
//       CThostFtdcQryInvestorPositionField *pQryInvestorPosition, int nRequestID) {
//   // TODO
//   return 0;
// }

// int ImplTsFtdcTraderApi::ReqQryTradingAccount(
//       CThostFtdcQryTradingAccountField *pQryTradingAccount, int nRequestID) {
//   // TODO
//   return 0;
// }

CSecurityFtdcOrderField ImplTsFtdcTraderApi::ToOrderField(
      map<string, string>& properties) {
  CSecurityFtdcOrderField order_field;
  memset(&order_field, 0, sizeof(order_field));

  string account = properties["ACCOUNT"];
  string status = properties["STATUS"];
  string client_id = properties["LOCAL_ORDER_ID"];
  string sys_id = properties["SYS_ORDER_ID"];
  string symbol = properties["SYMBOL"];
  string quantity = properties["QUANTITY"];
  string limit_price = properties["LIMIT_PRICE"];
  string direction = properties["DIRECTION"];
  string local_id = client_id;

  vector<string> ids;
  split(client_id, "_", ids);
  if (ids.size() == 2) {
    local_id = ids[1];
  }

  snprintf(order_field.InvestorID, sizeof(order_field.InvestorID), "%s",
           account.c_str());
  snprintf(order_field.OrderRef, sizeof(order_field.OrderRef), "%s",
           local_id.c_str());
  snprintf(order_field.OrderSysID, sizeof(order_field.OrderSysID), "%s",
           sys_id.c_str());
  snprintf(order_field.InstrumentID, sizeof(order_field.InstrumentID), "%s",
           symbol.c_str());
  order_field.VolumeTotalOriginal = atoi(quantity.c_str());
  snprintf(order_field.LimitPrice, sizeof(order_field.LimitPrice), "%s",
           limit_price.c_str());
  if (direction == "BUY" || 
      direction == "COLLATERALBUY") {
    order_field.Direction = SECURITY_FTDC_D_Buy;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_Open;
  } else if (direction == "SELL" ||
             direction == "COLLATERALSELL") {
    order_field.Direction = SECURITY_FTDC_D_Sell;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_Open;
  } else if (direction == "BORROWTOBUY") {
    order_field.Direction = SECURITY_FTDC_D_Buy;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_Close;
  } else if (direction == "BORROWTOSELL") {
    order_field.Direction = SECURITY_FTDC_D_Sell;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_Close;
  } else if (direction == "PAYBYCASH") {
    order_field.Direction = SECURITY_FTDC_D_Buy;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_CloseToday;
  } else if (direction == "PAYBYSTOCK") {
    order_field.Direction = SECURITY_FTDC_D_Sell;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_CloseToday;
  }

  if (status == "RECEIVED") {
    order_field.OrderStatus = SECURITY_FTDC_OST_NoTradeQueueing ;
  } else if (status == "CANCELED" || status == "PARTIALLYFILLEDUROUT") {
    order_field.OrderStatus = SECURITY_FTDC_OST_Canceled;
  } else if (status == "FILLED") {
    order_field.OrderStatus = SECURITY_FTDC_OST_AllTraded;
  }

  return order_field;
}

CSecurityFtdcTradeField ImplTsFtdcTraderApi::ToTradeField(
      map<string, string>& properties) {
  CSecurityFtdcTradeField trade_field;
  memset(&trade_field, 0, sizeof(trade_field));

  string account = properties["ACCOUNT"];
  string status = properties["STATUS"];
  string client_id = properties["LOCAL_ORDER_ID"];
  string sys_id = properties["SYS_ORDER_ID"];
  string symbol = properties["SYMBOL"];
  string quantity = properties["QUANTITY"];
  string limit_price = properties["LIMIT_PRICE"];
  string direction = properties["DIRECTION"];
  string fill_qty = properties["FILLED_QUANTITY"];
  string fill_price = properties["AVG_FILLED_PRICE"];
  string local_id = client_id;

  vector<string> ids;
  split(client_id, "_", ids);
  if (ids.size() == 2) {
    local_id = ids[1];
  }

  snprintf(trade_field.InvestorID, sizeof(trade_field.InvestorID), "%s",
           account.c_str());
  snprintf(trade_field.OrderRef, sizeof(trade_field.OrderRef), "%s",
           local_id.c_str());
  snprintf(trade_field.OrderSysID, sizeof(trade_field.OrderSysID), "%s",
           sys_id.c_str());
  snprintf(trade_field.InstrumentID, sizeof(trade_field.InstrumentID), "%s",
           symbol.c_str());

  if (direction == "BUY" || 
      direction == "COLLATERALBUY") {
    trade_field.Direction = SECURITY_FTDC_D_Buy;
    trade_field.OffsetFlag = SECURITY_FTDC_OF_Open;
  } else if (direction == "SELL" ||
             direction == "COLLATERALSELL") {
    trade_field.Direction = SECURITY_FTDC_D_Sell;
    trade_field.OffsetFlag = SECURITY_FTDC_OF_Open;
  } else if (direction == "BORROWTOBUY") {
    trade_field.Direction = SECURITY_FTDC_D_Buy;
    trade_field.OffsetFlag = SECURITY_FTDC_OF_Close;
  } else if (direction == "BORROWTOSELL") {
    trade_field.Direction = SECURITY_FTDC_D_Sell;
    trade_field.OffsetFlag = SECURITY_FTDC_OF_Close;
  } else if (direction == "PAYBYCASH") {
    trade_field.Direction = SECURITY_FTDC_D_Buy;
    trade_field.OffsetFlag = SECURITY_FTDC_OF_CloseToday;
  } else if (direction == "PAYBYSTOCK") {
    trade_field.Direction = SECURITY_FTDC_D_Sell;
    trade_field.OffsetFlag = SECURITY_FTDC_OF_CloseToday;
  }

  if (fill_price == "0.0") {
    printf("Invalid Trade Return!\n");
    return trade_field;
  }

  InputOrder *input_order = order_pool_.get(sys_id);
  if (input_order != NULL) {
    int total_qty = atoi(fill_qty.c_str());
    double total_turnover = atof(fill_price.c_str()) * (double)total_qty;
    bool is_valid = true;
    for (int i = 0; i < input_order->trade_size; i++) {
      if (input_order->trades[i] != NULL) {
        total_qty -= input_order->trades[i]->quantity;
        total_turnover -= input_order->trades[i]->price *
                          (double)input_order->trades[i]->quantity;
        if (total_qty <= 0 || total_turnover <= 0.0) {
          printf("Invalid total quantity[%d] or total turnover[%lf]\n", 
                 total_qty, total_turnover);
          snprintf(trade_field.Price, sizeof(trade_field.Price), "%s", fill_price.c_str());
          trade_field.Volume = atoi(fill_qty.c_str());
          is_valid = false;
          break;
        }
      }
    }  // for loop to 
    if (is_valid) {
      double trade_price = total_turnover / (double)total_qty;
      snprintf(trade_field.Price, sizeof(trade_field.Price), "%lf", trade_price);
      trade_field.Volume = total_qty;
      input_order->add_trade(trade_field.Volume, trade_price);
    }
    return trade_field;
  } else {
    printf("InputOrder [SysID-%s] not found!\n", sys_id.c_str());
    snprintf(trade_field.Price, sizeof(trade_field.Price), "%s",
             fill_price.c_str());
    trade_field.Volume = atoi(fill_qty.c_str());
  }

  return trade_field;
}

CSecurityFtdcInputOrderField ImplTsFtdcTraderApi::ToInputOrderField(
      map<string, string>& properties) {
  CSecurityFtdcInputOrderField order_field;
  memset(&order_field, 0, sizeof(order_field));

  string account = properties["ACCOUNT"];
  string status = properties["STATUS"];
  string client_id = properties["LOCAL_ORDER_ID"];
  string sys_id = properties["SYS_ORDER_ID"];
  string symbol = properties["SYMBOL"];
  string quantity = properties["QUANTITY"];
  string limit_price = properties["LIMIT_PRICE"];
  string direction = properties["DIRECTION"];
  string local_id = client_id;

  vector<string> ids;
  split(client_id, "_", ids);
  if (ids.size() == 2) {
    local_id = ids[1];
  }

  snprintf(order_field.InvestorID, sizeof(order_field.InvestorID), "%s",
           account.c_str());
  snprintf(order_field.OrderRef, sizeof(order_field.OrderRef), "%s",
           local_id.c_str());
  snprintf(order_field.InstrumentID, sizeof(order_field.InstrumentID), "%s",
           symbol.c_str());
  order_field.VolumeTotalOriginal = atoi(quantity.c_str());
  snprintf(order_field.LimitPrice, sizeof(order_field.LimitPrice), "%s",
           limit_price.c_str());
  if (direction == "BUY" ||
      direction == "COLLATERALBUY") {
    order_field.Direction = SECURITY_FTDC_D_Buy;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_Open;
  } else if (direction == "SELL" ||
             direction == "COLLATERALSELL") {
    order_field.Direction = SECURITY_FTDC_D_Sell;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_Open;
  } else if (direction == "BORROWTOBUY") {
    order_field.Direction = SECURITY_FTDC_D_Buy;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_Close;
  } else if (direction == "BORROWTOSELL") {
    order_field.Direction = SECURITY_FTDC_D_Sell;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_Close;
  } else if (direction == "PAYBYCASH") {
    order_field.Direction = SECURITY_FTDC_D_Buy;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_CloseToday;
  } else if (direction == "PAYBYSTOCK") {
    order_field.Direction = SECURITY_FTDC_D_Sell;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_CloseToday;
  }

  return order_field;
}

CSecurityFtdcRspInfoField ImplTsFtdcTraderApi::ToRspField(
      map<string, string>& properties) {
  CSecurityFtdcRspInfoField rsp_field;
  memset(&rsp_field, 0, sizeof(rsp_field));

  rsp_field.ErrorID = 50001;
  snprintf(rsp_field.ErrorMsg, sizeof(rsp_field.ErrorMsg), "Order Insert Failure!");

  return rsp_field;
}

void split(const string& str, const string& del, vector<string>& v) {
  string::size_type start, end;
  start = 0;
  end = str.find(del);
  while(end != string::npos) {
    v.push_back(str.substr(start, end - start));
    start = end + del.size();
    end = str.find(del, start);
  }
  if (start != str.length()) {
    v.push_back(str.substr(start));
  }
}

const char* second_now() {
  static char timestamp_str[32];
  time_t rawtime;
  struct tm *timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);

  snprintf(timestamp_str, sizeof(timestamp_str), 
           "%02d:%02d:%02d", timeinfo->tm_hour,
           timeinfo->tm_min, timeinfo->tm_sec);
  return timestamp_str;
}
