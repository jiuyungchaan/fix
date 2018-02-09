////////////////////////
///@author Kenny Chiu
///@date 20170120
///@summary Implementation of CTsFtdcTraderApi and ImplTsFtdcTraderApi
///         
///
////////////////////////

#include "TsFtdcTraderApi.h"

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

class ImplTsFtdcTraderApi : public CTsFtdcTraderApi{
 public:
  explicit ImplTsFtdcTraderApi(const char *configPath);

  // interfaces of CTsFtdcTraderApi
  void Init();
  int Join();
  void RegisterFront(char *pszFrontAddress);
  void RegisterSpi(CTsFtdcTraderSpi *pSpi);
  void SubscribePrivateTopic(THOST_TE_RESUME_TYPE nResumeType);
  void SubscribePublicTopic(THOST_TE_RESUME_TYPE nResumeType);
  int ReqUserLogin(CThostFtdcReqUserLoginField *pReqUserLoginField,
                   int nRequestID);
  int ReqUserLogout(CThostFtdcUserLogoutField *pUserLogout,
                    int nRequestID);
  int ReqOrderInsert(CThostFtdcInputOrderField *pInputOrder,
                     int nRequestID);
  int ReqOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction,
                     int nRequestID);
  int ReqMassOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction,
                         int nRequestID);
  int ReqQryInvestorPosition(CThostFtdcQryInvestorPositionField *pQryInvestorPosition,
                             int nRequestID);
  int ReqQryTradingAccount(CThostFtdcQryTradingAccountField *pQryTradingAccount,
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

    CThostFtdcInputOrderField basic_order;
    char client_id[64];
    SubTrade *trades[100];
    int trade_size;

    InputOrder();
    InputOrder(CThostFtdcInputOrderField *pInputOrder);
    void add_trade(int volume, double price);
  };

  class OrderPool {
   public:
    static const int MAX_ORDER_SIZE = 900000;

    OrderPool();
    InputOrder *get(int order_id);
    InputOrder *get(std::string sys_id);
    InputOrder *add(CThostFtdcInputOrderField *pInputOrder);
    void add_pair(std::string sys_id, int local_id);
    bool has_order(std::string sys_id);

   private:
    InputOrder *order_pool_[MAX_ORDER_SIZE];
    std::map<std::string, int> sys_to_local_;
  };

  static void *recv_thread(void *obj);
  static void *callback_onlogin(void *obj);
  static void *callback_onlogout(void *obj);

  void decode(const char *message);

  // typedef std::map<std::string, std::string> stringmap;

  CThostFtdcOrderField ToOrderField(std::map<std::string, std::string>& properties);
  CThostFtdcTradeField ToTradeField(std::map<std::string, std::string>& properties);
  CThostFtdcInputOrderField ToInputOrderField(std::map<std::string, std::string>& properties);
  CThostFtdcInputOrderActionField ToActionField(std::map<std::string, std::string>& properties);
  CThostFtdcRspInfoField ToRspField(std::map<std::string, std::string>& properties);

  CTsFtdcTraderSpi *trader_spi_;
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

CTsFtdcTraderApi *CTsFtdcTraderApi::CreateFtdcTraderApi(const char *configPath) {
  ImplTsFtdcTraderApi *api = new ImplTsFtdcTraderApi(configPath);
  return (CTsFtdcTraderApi *)api;
}

ImplTsFtdcTraderApi::InputOrder::InputOrder() : client_id{0}, trade_size(0) {
  memset(&basic_order, 0, sizeof(basic_order));
  memset(trades, 0, sizeof(trades));
}

ImplTsFtdcTraderApi::InputOrder::InputOrder(
      CThostFtdcInputOrderField *pInputOrder) : client_id{0}, trade_size(0) {
  memcpy(&basic_order, pInputOrder, sizeof(basic_order));
  memset(trades, 0, sizeof(trades));
}

void ImplTsFtdcTraderApi::InputOrder::add_trade(int volume, double price) {
  SubTrade *sub_trade = new SubTrade(volume, price);
  trades[trade_size++] = sub_trade;
}

ImplTsFtdcTraderApi::OrderPool::OrderPool() {
  memset(order_pool_, 0, sizeof(order_pool_));
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
      CThostFtdcInputOrderField *pInputOrder) {
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
  char buffer[512];
  while((recv_len = recv(fd, buffer, 512, 0)) > 0) {
    buffer[recv_len] = '\0';
    cout << "Data recieved: " << buffer << endl;
    self->decode(buffer);
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
  
  string status = properties["STATUS"];
  if (status == "SENDING") {
    // NOTHING TO DO
  } else if (status == "QUEUED") {
    // NOTHING TO DO
  } else if (status == "RECEIVED") {
    // OnRtnOrder
    // if RECEIVED is already received, this must be a cancel received confirmation.
    if (!order_pool_.has_order(properties["SYS_ORDER_ID"])) {
      CThostFtdcOrderField order_field = ToOrderField(properties);
      // add LocalOrderID&SysOrderID in the OrderPool?
      int local_id = strtol(order_field.OrderRef, NULL, 10)/100;
      order_pool_.add_pair(properties["SYS_ORDER_ID"], local_id);
      trader_spi_->OnRtnOrder(&order_field);
    }
  } else if (status == "PARTIALLYFILLED") {
    // OnRtnTrade
    if (properties["AVG_FILLED_PRICE"] != "" && 
        properties["AVG_FILLED_PRICE"] != "0.0") {
      CThostFtdcTradeField trade_field = ToTradeField(properties);
      trader_spi_->OnRtnTrade(&trade_field);
    }  // if AVG_FILLED_PRICE == 0.0, it is a cancel request confirmation
  } else if (status == "PARTIALLYFILLEDUROUT") {
    // OnRtnOrder -- success cancel
    CThostFtdcOrderField order_field = ToOrderField(properties);
    trader_spi_->OnRtnOrder(&order_field);
  } else if (status == "FILLED") {
    // OnRtnTrade and OnRtnOrder
    CThostFtdcTradeField trade_field = ToTradeField(properties);
    CThostFtdcOrderField order_field = ToOrderField(properties);
    trader_spi_->OnRtnTrade(&trade_field);
    trader_spi_->OnRtnOrder(&order_field);
  } else if (status == "CANCELED") {
    CThostFtdcOrderField order_field = ToOrderField(properties);
    trader_spi_->OnRtnOrder(&order_field);
  }
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

void ImplTsFtdcTraderApi::RegisterFront(char *pszFrontAddress) {
  snprintf(front_addr_, sizeof(front_addr_), "%s", pszFrontAddress);
}


void ImplTsFtdcTraderApi::RegisterSpi(CTsFtdcTraderSpi *pSpi) {
  trader_spi_ = pSpi;
}

void ImplTsFtdcTraderApi::SubscribePrivateTopic(
      THOST_TE_RESUME_TYPE nResumeType) {
  // dummy function to adapt CTP API
}

void ImplTsFtdcTraderApi::SubscribePublicTopic(
      THOST_TE_RESUME_TYPE nResumeType) {
  // dummy function to adapt CTP API
}

void *ImplTsFtdcTraderApi::callback_onlogin(void *obj) {
  sleep(1);
  ImplTsFtdcTraderApi *self = (ImplTsFtdcTraderApi *)obj;
  CThostFtdcRspUserLoginField login_field;
  memset(&login_field, 0, sizeof(login_field));
  self->trader_spi_->OnRspUserLogin(&login_field, NULL, 0, true);  
  return (void *)NULL;
}

void *ImplTsFtdcTraderApi::callback_onlogout(void *obj) {
  sleep(1);
  ImplTsFtdcTraderApi *self = (ImplTsFtdcTraderApi *)obj;
  CThostFtdcUserLogoutField logout_field;
  memset(&logout_field, 0, sizeof(logout_field));
  CThostFtdcRspInfoField info_field;
  memset(&info_field, 0, sizeof(info_field));
  self->trader_spi_->OnRspUserLogout(&logout_field, &info_field, 0, true);  
  return (void *)NULL;
}

int ImplTsFtdcTraderApi::ReqUserLogin(
      CThostFtdcReqUserLoginField *pReqUserLoginField, int nRequestID) {
  snprintf(user_id_, sizeof(user_id_), "%s", pReqUserLoginField->UserID);
  snprintf(user_passwd_, sizeof(user_passwd_), "%s", pReqUserLoginField->Password);
  pthread_t callback_thread;
  pthread_create(&callback_thread, NULL, callback_onlogin, (void *)this);
  return 0;
}

int ImplTsFtdcTraderApi::ReqUserLogout(
      CThostFtdcUserLogoutField *pUserLogout, int nRequestID) {
  close(server_fd_);
  pthread_t callback_thread;
  pthread_create(&callback_thread, NULL, callback_onlogout, (void *)this);
  return 0;
}

int ImplTsFtdcTraderApi::ReqOrderInsert(
      CThostFtdcInputOrderField *pInputOrder, int nRequestID) {
  InputOrder *input_order = order_pool_.add(pInputOrder);

  char action[16], order_type[16], symbol[16], duration[16], client_id[64];
  char message[512];
  
  if (pInputOrder->Direction == THOST_FTDC_D_Buy) {
    snprintf(action, sizeof(action), "BUY");
  } else {
    snprintf(action, sizeof(action), "SELL");
  }

  if (pInputOrder->InstrumentID[0] == '6') {
    snprintf(symbol, sizeof(symbol), "%s.SH", pInputOrder->InstrumentID);
  } else {
    snprintf(symbol, sizeof(symbol), "%s.SZ", pInputOrder->InstrumentID);
  }

  snprintf(duration, sizeof(duration), "GFD");
  snprintf(order_type, sizeof(order_type), "LIMIT");
  snprintf(client_id, sizeof(client_id), "%s_%s", second_now(),
           pInputOrder->OrderRef);
  snprintf(input_order->client_id, sizeof(input_order->client_id), "%s",
           client_id);

  snprintf(message, sizeof(message), "COMMAND=SENDORDER;ACCOUNT=%s;ACTION=%s;" \
           "SYMBOL=%s;QUANTITY=%d;TYPE=%s;LIMITPRICE=%lf;DURATION=%s;" \
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
      CThostFtdcInputOrderActionField *pInputOrderAction, int nRequestID) {
  char message[512];

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


int ImplTsFtdcTraderApi::ReqQryInvestorPosition(
      CThostFtdcQryInvestorPositionField *pQryInvestorPosition, int nRequestID) {
  // TODO
  return 0;
}

int ImplTsFtdcTraderApi::ReqQryTradingAccount(
      CThostFtdcQryTradingAccountField *pQryTradingAccount, int nRequestID) {
  // TODO
  return 0;
}

CThostFtdcOrderField ImplTsFtdcTraderApi::ToOrderField(
      map<string, string>& properties) {
  CThostFtdcOrderField order_field;
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
  order_field.LimitPrice = atof(limit_price.c_str());
  if (direction == "BUY") {
    order_field.Direction = THOST_FTDC_D_Buy;
  } else if (direction == "SELL") {
    order_field.Direction = THOST_FTDC_D_Sell;
  }

  if (status == "RECEIVED") {
    order_field.OrderStatus = THOST_FTDC_OST_NoTradeQueueing;
  } else if (status == "CANCELED" || status == "PARTIALLYFILLEDUROUT") {
    order_field.OrderStatus = THOST_FTDC_OST_Canceled;
  } else if (status == "FILLED") {
    order_field.OrderStatus = THOST_FTDC_OST_AllTraded;
  }

  return order_field;
}

CThostFtdcTradeField ImplTsFtdcTraderApi::ToTradeField(
      map<string, string>& properties) {
  CThostFtdcTradeField trade_field;
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

  if (direction == "BUY") {
    trade_field.Direction = THOST_FTDC_D_Buy;
  } else if (direction == "SELL") {
    trade_field.Direction = THOST_FTDC_D_Sell;
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
          trade_field.Price = atof(fill_price.c_str());
          trade_field.Volume = atoi(fill_qty.c_str());
          is_valid = false;
          break;
        }
      }
    }  // for loop to 
    if (is_valid) {
      trade_field.Price = total_turnover / (double)total_qty;
      trade_field.Volume = total_qty;
      input_order->add_trade(trade_field.Volume, trade_field.Price);
    }
    return trade_field;
  } else {
    printf("InputOrder [SysID-%s] not found!\n", sys_id.c_str());
    trade_field.Price = atof(fill_price.c_str());
    trade_field.Volume = atoi(fill_qty.c_str());
  }

  return trade_field;
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
