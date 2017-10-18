////////////////////////
///@author Kenny Chiu
///@date 20170411
///@summary Implementation of BOFtdcTraderApi
///         
///
////////////////////////

#include "BOFtdcTraderApi.h"

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
const char *time_now();
void split(const std::string& str, const std::string& del, std::vector<std::string>& v);

class ImplBOFtdcTraderApi : public BOFtdcTraderApi{
 public:
  explicit ImplBOFtdcTraderApi(const char *configPath);

  // interfaces of BOFtdcTraderApi
  void Release();
  void Init();
  int Join();
  const char *GetTradingDay();
  void RegisterFront(char *pszFrontAddress);
  void RegisterSpi(BOFtdcTraderSpi *pSpi);
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
  virtual ~ImplBOFtdcTraderApi();

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
    char sys_id[64];
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
  // static void *callback_onlogin(void *obj);
  static void *callback_onlogout(void *obj);
  static void *callback_onrandcode(void *obj);

  void handle_data(char *data, int len);
  void decode(const char *message);

  // typedef std::map<std::string, std::string> stringmap;

  CSecurityFtdcOrderField ToOrderField(std::map<std::string, std::string>& properties);
  CSecurityFtdcTradeField ToTradeField(std::map<std::string, std::string>& properties,
                                       int &has_order);
  CSecurityFtdcInputOrderField ToInputOrderField(std::map<std::string, std::string>& properties);
  CSecurityFtdcInputOrderActionField ToActionField(std::map<std::string, std::string>& properties);
  CSecurityFtdcRspInfoField ToRspField(std::map<std::string, std::string>& properties);

  BOFtdcTraderSpi *trader_spi_;
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

BOFtdcTraderApi *BOFtdcTraderApi::CreateFtdcTraderApi(const char *configPath) {
  ImplBOFtdcTraderApi *api = new ImplBOFtdcTraderApi(configPath);
  return (BOFtdcTraderApi *)api;
}

ImplBOFtdcTraderApi::InputOrder::InputOrder() : client_id{0}, sys_id{0}, trade_size(0) {
  memset(&basic_order, 0, sizeof(basic_order));
  memset(trades, 0, sizeof(trades));
}

ImplBOFtdcTraderApi::InputOrder::InputOrder(
      CSecurityFtdcInputOrderField *pInputOrder) : client_id{0}, sys_id{0}, trade_size(0) {
  memcpy(&basic_order, pInputOrder, sizeof(basic_order));
  memset(trades, 0, sizeof(trades));
}

void ImplBOFtdcTraderApi::InputOrder::add_trade(int volume, double price) {
  SubTrade *sub_trade = new SubTrade(volume, price);
  trades[trade_size++] = sub_trade;
}

ImplBOFtdcTraderApi::OrderPool::OrderPool() {
  memset(order_pool_, sizeof(order_pool_), 0);
}

ImplBOFtdcTraderApi::InputOrder *ImplBOFtdcTraderApi::OrderPool::get(
      int order_id) {
  if (order_id >= 0 && order_id < MAX_ORDER_SIZE) {
    return order_pool_[order_id];
  } else {
    return (ImplBOFtdcTraderApi::InputOrder *)NULL;
  }
}

ImplBOFtdcTraderApi::InputOrder *ImplBOFtdcTraderApi::OrderPool::get(
      string sys_id) {
  map<string, int>::iterator it = sys_to_local_.find(sys_id);
  if (it == sys_to_local_.end()) {
    printf("SysOrderID-%s not found!\n", sys_id.c_str());
    return (ImplBOFtdcTraderApi::InputOrder *)NULL;
  } else {
    return get(it->second);
  }
}

bool ImplBOFtdcTraderApi::OrderPool::has_order(string sys_id) {
  map<string, int>::iterator it = sys_to_local_.find(sys_id);
  return (it != sys_to_local_.end());
}

ImplBOFtdcTraderApi::InputOrder *ImplBOFtdcTraderApi::OrderPool::add(
      CSecurityFtdcInputOrderField *pInputOrder) {
  ImplBOFtdcTraderApi::InputOrder *input_order = new
      ImplBOFtdcTraderApi::InputOrder(pInputOrder);
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

void ImplBOFtdcTraderApi::OrderPool::add_pair(string sys_id, int local_id) {
  map<string, int>::iterator it = sys_to_local_.find(sys_id);
  if (it != sys_to_local_.end()) {
    printf("SysOrderID-%s already match for %d !\n", sys_id.c_str(),
           it->second);
  } else {
    sys_to_local_.insert(std::pair<std::string, int>(sys_id, local_id));
    if (order_pool_[local_id] != NULL) {
      snprintf(order_pool_[local_id]->sys_id, sizeof(order_pool_[local_id]->sys_id), "%s",
            sys_id.c_str()); 
    }
  }
}

ImplBOFtdcTraderApi::ImplBOFtdcTraderApi(const char *pszFlowPath) :
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

ImplBOFtdcTraderApi::~ImplBOFtdcTraderApi() {
  // TODO
}

void *ImplBOFtdcTraderApi::recv_thread(void *obj) {
  static const int BUF_SIZE = 2048;
  char buffer[BUF_SIZE];
  ImplBOFtdcTraderApi *self = (ImplBOFtdcTraderApi *)obj;
  int fd = self->server_fd_;
  int recv_len;
  char *total_buffer = NULL;
  int  sec_cnt = 0;  // section count
  int total_len = 0;
  while((recv_len = recv(fd, buffer, BUF_SIZE, 0)) > 0) {
    // if message received full of buffer
  
    sec_cnt++;
    char *tmp_ptr = (char *)malloc(BUF_SIZE*(sec_cnt + 1)*sizeof(char));
    if (sec_cnt > 1) {
      // memcpy(tmp_ptr, total_buffer, (sec_cnt - 1)*BUF_SIZE);
      memcpy(tmp_ptr, total_buffer, total_len);
      free(total_buffer);
    }
    // memcpy(tmp_ptr + (sec_cnt - 1)*BUF_SIZE, buffer, BUF_SIZE);
    memcpy(tmp_ptr + total_len, buffer, recv_len);
    total_buffer = tmp_ptr;
    total_len += recv_len;

    if (total_len > 1 && total_buffer[total_len - 1] == '\0') {
      self->log_file_ << time_now() << "- Data total received " << total_len << endl;

      self->handle_data(total_buffer, total_len);
      sec_cnt = 0;
      free(total_buffer);
      total_buffer = NULL;
      total_len = 0;
    }
    /*
    if (recv_len == BUF_SIZE) {
      sec_cnt++;
      char *tmp_ptr = (char *)malloc(BUF_SIZE*(sec_cnt + 1)*sizeof(char));
      if (sec_cnt > 1) {
        // memcpy(tmp_ptr, total_buffer, (sec_cnt - 1)*BUF_SIZE);
        memcpy(tmp_ptr, total_buffer, total_len);
        free(total_buffer);
      }
      // memcpy(tmp_ptr + (sec_cnt - 1)*BUF_SIZE, buffer, BUF_SIZE);
      memcpy(tmp_ptr + total_len, buffer, BUF_SIZE);
      total_buffer = tmp_ptr;
      total_len += recv_len;
      if (total_buffer[total_len - 1] == '\0') {
        handle_data(total_buffer, total_len);
        sec_cnt = 0;
        free(total_buffer);
        total_buffer = NULL;
      }
    } else {
      if (sec_cnt == 0) {
        total_buffer = (char *)malloc(BUF_SIZE*sizeof(char));
      }
      memcpy(total_buffer + sec_cnt*BUF_SIZE, buffer, recv_len);
      int total_len = sec_cnt*BUF_SIZE + recv_len;
      total_buffer[total_len] = '\0';

      self->log_file_ << time_now() << "- Data total received " << total_len << endl;
      
      sec_cnt = 0;
      free(total_buffer);
      total_buffer = NULL;
    } */

  }

  printf("Failed to receive data! Exit thread...\n");
  self->log_file_ << time_now() << "- Failed to receive data! Exit thread..." << endl;
  return (void *)NULL;
}

void ImplBOFtdcTraderApi::handle_data(char *data, int len) {
  char *start = data;
  while (true) {
    char *p = strchr(start, '\0');
    if (p == NULL) 
      break;
    if (p - data < len - 1) {
      // printf("Multi datagram-[%ld]:%s\n", p - start, start);
      log_file_ << time_now() << "- Multi datagram-" << (long)(p - start)
                      << ":" << start << endl;
      decode(start);
      start = p + 1;
    } else {
      // printf("Last datagram-[%ld]:%s\n", p - start, start);
      log_file_ << time_now() << "- Last datagram-" << (long)(p - start)
                      << ":" << start << endl;
      decode(start);
      break;
    }
  }  // while
}

void ImplBOFtdcTraderApi::decode(const char *message) {
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
      string account = properties["ACCOUNT"];
      if (strcmp(account.c_str(), user_id_) == 0) {
        // OnRtnOrder
        // if RECEIVED is already received, this must be a cancel received confirmation.
        CSecurityFtdcOrderField order_field = ToOrderField(properties);
        if (!order_pool_.has_order(order_field.OrderSysID)) {
          // add LocalOrderID&SysOrderID in the OrderPool?
          int local_id = strtol(order_field.OrderRef, NULL, 10)/100;
          // cout << "decode add_pair: " << order_field.OrderSysID << " localID:" << local_id << endl;
          order_pool_.add_pair(order_field.OrderSysID, local_id);
          trader_spi_->OnRtnOrder(&order_field);
        }
      }
    } else if (status == "PARTIALLYFILLED") {
      // OnRtnTrade
      string account = properties["ACCOUNT"];
      if (strcmp(account.c_str(), user_id_) == 0) {
        int has_order;
        CSecurityFtdcTradeField trade_field = ToTradeField(properties, has_order);
        if (!has_order) {
          CSecurityFtdcOrderField order_field = ToOrderField(properties);
          order_field.OrderStatus = SECURITY_FTDC_OST_NoTradeQueueing;
          trader_spi_->OnRtnOrder(&order_field);
        }
        if (trade_field.Volume > 0) {
          trader_spi_->OnRtnTrade(&trade_field);
        }
      }
    } else if (status == "PARTIALLYFILLEDUROUT") {
      string account = properties["ACCOUNT"];
      if (strcmp(account.c_str(), user_id_) == 0) {
        // OnRtnOrder -- success cancel
        int has_order;
        CSecurityFtdcTradeField trade_field = ToTradeField(properties, has_order);
        CSecurityFtdcOrderField order_field = ToOrderField(properties);
        if (!has_order) {
          order_field.OrderStatus = SECURITY_FTDC_OST_NoTradeQueueing;
          trader_spi_->OnRtnOrder(&order_field);
          order_field.OrderStatus = SECURITY_FTDC_OST_Canceled;
        }
        if (trade_field.Volume > 0) {
          log_file_ << time_now() << " - Left Trade:" << trade_field.InstrumentID
                    << " " << trade_field.OrderRef << " " << trade_field.OrderSysID
                    << " " << trade_field.Direction << " " << trade_field.Volume
                    << " " << trade_field.Price << endl;
          trader_spi_->OnRtnTrade(&trade_field);
        }
        trader_spi_->OnRtnOrder(&order_field);
      }
    } else if (status == "FILLED") {
      string account = properties["ACCOUNT"];
      if (strcmp(account.c_str(), user_id_) == 0) {
        // OnRtnTrade and OnRtnOrder
        int has_order;
        CSecurityFtdcOrderField order_field = ToOrderField(properties);
        CSecurityFtdcTradeField trade_field = ToTradeField(properties, has_order);
        if (!has_order) {
          order_field.OrderStatus = SECURITY_FTDC_OST_NoTradeQueueing;
          trader_spi_->OnRtnOrder(&order_field);
          order_field.OrderStatus = SECURITY_FTDC_OST_AllTraded;
        }
        trader_spi_->OnRtnTrade(&trade_field);
        trader_spi_->OnRtnOrder(&order_field);
      }
    } else if (status == "CANCELED") {
      string account = properties["ACCOUNT"];
      if (strcmp(account.c_str(), user_id_) == 0) {
        CSecurityFtdcOrderField order_field = ToOrderField(properties);
        trader_spi_->OnRtnOrder(&order_field);
      }
    } else if (status == "REJECTED") {
      string account = properties["ACCOUNT"];
      if (strcmp(account.c_str(), user_id_) == 0) {
        CSecurityFtdcInputOrderField order_field = ToInputOrderField(properties);
        CSecurityFtdcRspInfoField info_field = ToRspField(properties);
        trader_spi_->OnRspOrderInsert(&order_field, &info_field, 0, true);
      }
    } else if (status == "SENDFAILED") {
      string account = properties["ACCOUNT"];
      if (strcmp(account.c_str(), user_id_) == 0) {
        CSecurityFtdcInputOrderField order_field = ToInputOrderField(properties);
        CSecurityFtdcRspInfoField info_field = ToRspField(properties);
        trader_spi_->OnRspOrderInsert(&order_field, &info_field, 0, true);
      }
    }
  } // if type == "UPDATE" 
  else if (msg_type == "LOGIN") {
    string error_code = properties["ERROR"];
    if (error_code == "0") {
      CSecurityFtdcRspUserLoginField login_field;
      memset(&login_field, 0, sizeof(login_field));
      trader_spi_->OnRspUserLogin(&login_field, NULL, 0, true);
    } else {
      string error_msg = properties["MESSAGE"];
      CSecurityFtdcRspUserLoginField login_field;
      memset(&login_field, 0, sizeof(login_field));
      CSecurityFtdcRspInfoField info_field;
      info_field.ErrorID = atoi(error_code.c_str());
      snprintf(info_field.ErrorMsg, sizeof(info_field.ErrorMsg), "%s",
               error_msg.c_str());
      trader_spi_->OnRspUserLogin(&login_field, NULL, 0, true);
    }
  }
}

void ImplBOFtdcTraderApi::Release() {
  // TODO
}

void ImplBOFtdcTraderApi::Init() {
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

int ImplBOFtdcTraderApi::Join() {
  // dummy function to adapt CTP API
  return 0;
}

const char * ImplBOFtdcTraderApi::GetTradingDay() {
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

void ImplBOFtdcTraderApi::RegisterFront(char *pszFrontAddress) {
  snprintf(front_addr_, sizeof(front_addr_), "%s", pszFrontAddress);
}


void ImplBOFtdcTraderApi::RegisterSpi(BOFtdcTraderSpi *pSpi) {
  trader_spi_ = pSpi;
}

void ImplBOFtdcTraderApi::SubscribePrivateTopic(
      SECURITY_TE_RESUME_TYPE nResumeType) {
  // dummy function to adapt LTS API
}

void ImplBOFtdcTraderApi::SubscribePublicTopic(
      SECURITY_TE_RESUME_TYPE nResumeType) {
  // dummy function to adapt LTS API
}

void *ImplBOFtdcTraderApi::callback_onlogout(void *obj) {
  sleep(1);
  ImplBOFtdcTraderApi *self = (ImplBOFtdcTraderApi *)obj;
  CSecurityFtdcUserLogoutField logout_field;
  memset(&logout_field, 0, sizeof(logout_field));
  CSecurityFtdcRspInfoField info_field;
  memset(&info_field, 0, sizeof(info_field));
  self->trader_spi_->OnRspUserLogout(&logout_field, &info_field, 0, true);  
  return (void *)NULL;
}

void *ImplBOFtdcTraderApi::callback_onrandcode(void *obj) {
  sleep(1);
  ImplBOFtdcTraderApi *self = (ImplBOFtdcTraderApi *)obj;
  CSecurityFtdcAuthRandCodeField code_field;
  memset(&code_field, 0, sizeof(code_field));
  snprintf(code_field.RandCode, sizeof(code_field.RandCode), "nonsense");
  CSecurityFtdcRspInfoField info_field;
  memset(&info_field, 0, sizeof(info_field));
  self->trader_spi_->OnRspFetchAuthRandCode(&code_field, &info_field, 0, true);
  return (void *)NULL;
}

int ImplBOFtdcTraderApi::ReqUserLogin(
      CSecurityFtdcReqUserLoginField *pReqUserLoginField, int nRequestID) {
  snprintf(user_id_, sizeof(user_id_), "%s", pReqUserLoginField->UserID);
  snprintf(user_passwd_, sizeof(user_passwd_), "%s", pReqUserLoginField->Password);

  char message[1024];
  snprintf(message, sizeof(message), "COMMAND=LOGIN;ACCOUNT=%s", user_id_);
  int ret = send(server_fd_, message, strlen(message) + 1, 0);
  log_file_ << time_now() << "- Send message:" << strlen(message) + 1 << ":" << message <<  endl;
  if (ret < 0) {
    printf("Failed to send message - %d\n", ret);
    return 1;
  }
  return 0;
}

int ImplBOFtdcTraderApi::ReqUserLogout(
      CSecurityFtdcUserLogoutField *pUserLogout, int nRequestID) {
  close(server_fd_);
  pthread_t callback_thread;
  pthread_create(&callback_thread, NULL, callback_onlogout, (void *)this);
  return 0;
}

int ImplBOFtdcTraderApi::ReqFetchAuthRandCode(
      CSecurityFtdcAuthRandCodeField *pAuthRandCode, int nRequestID) {
  pthread_t callback_thread;
  pthread_create(&callback_thread, NULL, callback_onrandcode, (void *)this);
  return 0;
}

int ImplBOFtdcTraderApi::ReqOrderInsert(
      CSecurityFtdcInputOrderField *pInputOrder, int nRequestID) {
  InputOrder *input_order = order_pool_.add(pInputOrder);

  char action[16], order_type[16], symbol[16], duration[16], client_id[64];
  char message[1024];
  
  switch(pInputOrder->Direction) {
    case SECURITY_FTDC_D_Buy:
      snprintf(action, sizeof(action), "BUY");
      break;
    case SECURITY_FTDC_D_Sell:
      snprintf(action, sizeof(action), "SELL");
      break;
    case SECURITY_FTDC_D_BuyCollateral:
      snprintf(action, sizeof(action), "COLLATERALBUY");
      break;
    case SECURITY_FTDC_D_SellCollateral:
      snprintf(action, sizeof(action), "COLLATERALSELL");
      break;
    case SECURITY_FTDC_D_MarginTrade:
      snprintf(action, sizeof(action), "BORROWTOBUY");
      break;
    case SECURITY_FTDC_D_ShortSell:
      snprintf(action, sizeof(action), "BORROWTOSELL");
      break;
    case SECURITY_FTDC_D_RepayMargin:
      snprintf(action, sizeof(action), "SELLTOPAY");
      break;
    case SECURITY_FTDC_D_RepayStock:
      snprintf(action, sizeof(action), "BUYTOPAY");
      break;
    case SECURITY_FTDC_D_DirectRepayMargin:
      snprintf(action, sizeof(action), "PAYBYCASH");
      break;
    case SECURITY_FTDC_D_DirectRepayStock:
      snprintf(action, sizeof(action), "PAYBYSTOCK");
      break;
  }

  if (strcmp(pInputOrder->ExchangeID, "SH") == 0 ||
      strcmp(pInputOrder->ExchangeID, "SZ") == 0) {
    snprintf(symbol, sizeof(symbol), "%s.%s", pInputOrder->InstrumentID, 
             pInputOrder->ExchangeID);
  } else if (strcmp(pInputOrder->ExchangeID, "SSE") == 0 ||
             strcmp(pInputOrder->ExchangeID, "SZE") == 0) {
    if (strcmp(pInputOrder->ExchangeID, "SZE") == 0) {
      snprintf(symbol, sizeof(symbol), "%s.SZ", pInputOrder->InstrumentID);
    } else {
      snprintf(symbol, sizeof(symbol), "%s.SH", pInputOrder->InstrumentID);
    }
  }

  snprintf(duration, sizeof(duration), "GFD");
  snprintf(order_type, sizeof(order_type), "LIMIT");
  snprintf(client_id, sizeof(client_id), "%s_%s", second_now(),
           pInputOrder->OrderRef);
  snprintf(input_order->client_id, sizeof(input_order->client_id), "%s",
           client_id);

  string price_op = "LIMITPRICE";
  if (pInputOrder->Direction == SECURITY_FTDC_D_DirectRepayMargin) {
    price_op = "PAYBYCASHAMOUNT";
  }
  snprintf(message, sizeof(message), "COMMAND=SENDORDER;ACCOUNT=%s;ACTION=%s;" \
           "SYMBOL=%s;QUANTITY=%d;TYPE=%s;%s=%s;DURATION=%s;CLIENTID=%s",
           user_id_, action, symbol, pInputOrder->VolumeTotalOriginal,
           order_type, price_op.c_str(), pInputOrder->LimitPrice, duration,
           client_id);
  int ret = send(server_fd_, message, strlen(message) + 1, 0);
  log_file_ << time_now() << "- Send message:" << strlen(message) + 1 << ":" << message <<  endl;
  if (ret < 0) {
    printf("Failed to send message - %d\n", ret);
    return 1;
  }
  return 0;
}

int ImplBOFtdcTraderApi::ReqOrderAction(
      CSecurityFtdcInputOrderActionField *pInputOrderAction, int nRequestID) {
  char message[1024];

  int local_id = strtol(pInputOrderAction->OrderRef, NULL, 10)/100;
  InputOrder *input_order = order_pool_.get(local_id);
  int ret = 1;
  if (input_order != NULL) {
    snprintf(message, sizeof(message), "COMMAND=CANCELORDER;INSTRUMENT=%s.%s;"
             "CLIENTID=%s;SYSID=%s;EXCHANGE=%s",
             input_order->basic_order.InstrumentID, input_order->basic_order.ExchangeID,
             input_order->client_id, input_order->sys_id, input_order->basic_order.ExchangeID);
    ret = send(server_fd_, message, strlen(message) + 1, 0);
    // cout << "Send message:" << message << endl;
    log_file_ << time_now() << "- Send message:" << message << endl;
    if (ret < 0) {
      printf("Failed to send message - %d\n", ret);
      return 1;
    }
  } else {
    printf("Input Order LocalID=%s not found.\n", pInputOrderAction->OrderRef);
  }
  return 0;
}


// int ImplBOFtdcTraderApi::ReqQryInvestorPosition(
//       CThostFtdcQryInvestorPositionField *pQryInvestorPosition, int nRequestID) {
//   // TODO
//   return 0;
// }

// int ImplBOFtdcTraderApi::ReqQryTradingAccount(
//       CThostFtdcQryTradingAccountField *pQryTradingAccount, int nRequestID) {
//   // TODO
//   return 0;
// }

CSecurityFtdcOrderField ImplBOFtdcTraderApi::ToOrderField(
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

  string cut_sys_id = sys_id;
  // if (sys_id.size() > 25) {
  //   cut_sys_id = sys_id.substr(24, 19);
  // }
  vector<string> ids;
  split(client_id, "_", ids);
  if (ids.size() == 2) {
    local_id = ids[1];
  }
  vector<string> ins_exch;
  string readable_symbol = symbol;
  string exchange;
  split(symbol, ".", ins_exch);
  if (ins_exch.size() == 2) {
    readable_symbol = ins_exch[0];
    exchange = ins_exch[1];
    if (ins_exch[1] == "SH") {
      exchange = "SSE";
    } else if (ins_exch[1] == "SZ") {
      exchange = "SZE";
    }
  }

  snprintf(order_field.InvestorID, sizeof(order_field.InvestorID), "%s",
           account.c_str());
  snprintf(order_field.OrderRef, sizeof(order_field.OrderRef), "%s",
           local_id.c_str());
  snprintf(order_field.OrderSysID, sizeof(order_field.OrderSysID), "%s",
           cut_sys_id.c_str());
  snprintf(order_field.InstrumentID, sizeof(order_field.InstrumentID), "%s",
           readable_symbol.c_str());
  snprintf(order_field.ExchangeID, sizeof(order_field.ExchangeID), "%s",
           exchange.c_str());
  order_field.VolumeTotalOriginal = atoi(quantity.c_str());
  snprintf(order_field.LimitPrice, sizeof(order_field.LimitPrice), "%s",
           limit_price.c_str());
  if (direction == "BUY") {
    order_field.Direction = SECURITY_FTDC_D_Buy;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_Open;
  } else if (direction == "COLLATERALBUY") {
    order_field.Direction = SECURITY_FTDC_D_BuyCollateral;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_Open;
  } else if (direction == "SELL") {
    order_field.Direction = SECURITY_FTDC_D_Sell;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_CloseYesterday;
  } else if (direction == "COLLATERALSELL") {
    order_field.Direction = SECURITY_FTDC_D_SellCollateral;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_Open;
  } else if (direction == "BORROWTOBUY") {
    order_field.Direction = SECURITY_FTDC_D_MarginTrade;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_Open;
  } else if (direction == "BORROWTOSELL") {
    order_field.Direction = SECURITY_FTDC_D_ShortSell;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_Open;
  } else if (direction == "BUYTOPAY") {
    order_field.Direction = SECURITY_FTDC_D_RepayStock;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_Open;
  } else if (direction == "SELLTOPAY") {
    order_field.Direction = SECURITY_FTDC_D_RepayMargin;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_Open;
  } else if (direction == "PAYBYCASH") {
    order_field.Direction = SECURITY_FTDC_D_DirectRepayMargin;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_Open;
  } else if (direction == "PAYBYSTOCK") {
    order_field.Direction = SECURITY_FTDC_D_DirectRepayStock;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_Open;
  }

  if (status == "RECEIVED") {
    order_field.OrderStatus = SECURITY_FTDC_OST_NoTradeQueueing ;
  } else if (status == "CANCELED" || status == "PARTIALLYFILLEDUROUT") {
    order_field.OrderStatus = SECURITY_FTDC_OST_Canceled;
  } else if (status == "FILLED") {
    if (order_pool_.has_order(cut_sys_id)) {
      order_field.OrderStatus = SECURITY_FTDC_OST_AllTraded;
    } else {
      order_field.OrderStatus = SECURITY_FTDC_OST_NoTradeQueueing;
    }
  } else if (status == "PARTIALLYFILLED") {
    order_field.OrderStatus = SECURITY_FTDC_OST_NoTradeQueueing;
  }

  return order_field;
}

CSecurityFtdcTradeField ImplBOFtdcTraderApi::ToTradeField(
      map<string, string>& properties, int &has_order) {
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
  string fill_price = properties["FILLED_PRICE"];
  string local_id = client_id;

  string cut_sys_id = sys_id;
  if (sys_id.size() > 25) {
    cut_sys_id = sys_id.substr(24, 19);
  }
  vector<string> ids;
  split(client_id, "_", ids);
  if (ids.size() == 2) {
    local_id = ids[1];
  }
  vector<string> ins_exch;
  string readable_symbol = symbol;
  string exchange;
  split(symbol, ".", ins_exch);
  if (ins_exch.size() == 2) {
    readable_symbol = ins_exch[0];
    exchange = ins_exch[1];
    if (ins_exch[1] == "SH") {
      exchange = "SSE";
    } else if (ins_exch[1] == "SZ") {
      exchange = "SZE";
    }
  }

  snprintf(trade_field.InvestorID, sizeof(trade_field.InvestorID), "%s",
           account.c_str());
  snprintf(trade_field.OrderRef, sizeof(trade_field.OrderRef), "%s",
           local_id.c_str());
  snprintf(trade_field.OrderSysID, sizeof(trade_field.OrderSysID), "%s",
           cut_sys_id.c_str());
  snprintf(trade_field.InstrumentID, sizeof(trade_field.InstrumentID), "%s",
           readable_symbol.c_str());
  snprintf(trade_field.ExchangeID, sizeof(trade_field.ExchangeID), "%s",
           exchange.c_str());

  if (direction == "BUY") {
    trade_field.Direction = SECURITY_FTDC_D_Buy;
    trade_field.OffsetFlag = SECURITY_FTDC_OF_Open;
  } else if (direction == "COLLATERALBUY") {
    trade_field.Direction = SECURITY_FTDC_D_BuyCollateral;
    trade_field.OffsetFlag = SECURITY_FTDC_OF_Open;
  } else if (direction == "SELL") {
    trade_field.Direction = SECURITY_FTDC_D_Sell;
    trade_field.OffsetFlag = SECURITY_FTDC_OF_CloseYesterday;
  } else if (direction == "COLLATERALSELL") {
    trade_field.Direction = SECURITY_FTDC_D_SellCollateral;
    trade_field.OffsetFlag = SECURITY_FTDC_OF_Open;
  } else if (direction == "BORROWTOBUY") {
    trade_field.Direction = SECURITY_FTDC_D_MarginTrade;
    trade_field.OffsetFlag = SECURITY_FTDC_OF_Open;
  } else if (direction == "BORROWTOSELL") {
    trade_field.Direction = SECURITY_FTDC_D_ShortSell;
    trade_field.OffsetFlag = SECURITY_FTDC_OF_Open;
  } else if (direction == "BUYTOPAY") {
    trade_field.Direction = SECURITY_FTDC_D_RepayStock;
    trade_field.OffsetFlag = SECURITY_FTDC_OF_Open;
  } else if (direction == "SELLTOPAY") {
    trade_field.Direction = SECURITY_FTDC_D_RepayMargin;
    trade_field.OffsetFlag = SECURITY_FTDC_OF_Open;
  } else if (direction == "PAYBYCASH") {
    trade_field.Direction = SECURITY_FTDC_D_DirectRepayMargin;
    trade_field.OffsetFlag = SECURITY_FTDC_OF_Open;
  } else if (direction == "PAYBYSTOCK") {
    trade_field.Direction = SECURITY_FTDC_D_DirectRepayStock;
    trade_field.OffsetFlag = SECURITY_FTDC_OF_Open;
  }

  if (fill_price == "0.0") {
    printf("Invalid Trade Return!\n");
    return trade_field;
  }
  
  has_order = order_pool_.has_order(trade_field.OrderSysID);
  if (!has_order) {
    int local_id = strtol(trade_field.OrderRef, NULL, 10)/100;
    // cout << "decode add_pair: " << order_field.OrderSysID << " localID:" << local_id << endl;
    order_pool_.add_pair(trade_field.OrderSysID, local_id);
  }

  InputOrder *input_order = order_pool_.get(cut_sys_id);
  if (input_order != NULL) {
    trade_field.Volume = atoi(fill_qty.c_str());
    double f_prc = atof(fill_price.c_str());
    snprintf(trade_field.Price, sizeof(trade_field.Price), "%lf", f_prc);
    input_order->add_trade(trade_field.Volume, f_prc);
    return trade_field;
  } else {
    printf("InputOrder [SysID-%s] not found!\n", cut_sys_id.c_str());
    snprintf(trade_field.Price, sizeof(trade_field.Price), "%s",
             fill_price.c_str());
    trade_field.Volume = atoi(fill_qty.c_str());
  }

  return trade_field;
}

CSecurityFtdcInputOrderField ImplBOFtdcTraderApi::ToInputOrderField(
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
  vector<string> ins_exch;
  string readable_symbol = symbol;
  string exchange;
  split(symbol, ".", ins_exch);
  if (ins_exch.size() == 2) {
    readable_symbol = ins_exch[0];
    exchange = ins_exch[1];
    if (ins_exch[1] == "SH") {
      exchange = "SSE";
    } else if (ins_exch[1] == "SZ") {
      exchange = "SZE";
    }
  }

  snprintf(order_field.InvestorID, sizeof(order_field.InvestorID), "%s",
           account.c_str());
  snprintf(order_field.OrderRef, sizeof(order_field.OrderRef), "%s",
           local_id.c_str());
  snprintf(order_field.InstrumentID, sizeof(order_field.InstrumentID), "%s",
           readable_symbol.c_str());
  snprintf(order_field.ExchangeID, sizeof(order_field.ExchangeID), "%s",
           readable_symbol.c_str());
  order_field.VolumeTotalOriginal = atoi(quantity.c_str());
  snprintf(order_field.LimitPrice, sizeof(order_field.LimitPrice), "%s",
           limit_price.c_str());
  if (direction == "BUY") {
    order_field.Direction = SECURITY_FTDC_D_Buy;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_Open;
  } else if (direction == "COLLATERALBUY") {
    order_field.Direction = SECURITY_FTDC_D_BuyCollateral;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_Open;
  } else if (direction == "SELL") {
    order_field.Direction = SECURITY_FTDC_D_Sell;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_CloseYesterday;
  } else if (direction == "COLLATERALSELL") {
    order_field.Direction = SECURITY_FTDC_D_SellCollateral;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_Open;
  } else if (direction == "BORROWTOBUY") {
    order_field.Direction = SECURITY_FTDC_D_MarginTrade;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_Open;
  } else if (direction == "BORROWTOSELL") {
    order_field.Direction = SECURITY_FTDC_D_ShortSell;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_Open;
  } else if (direction == "BUYTOPAY") {
    order_field.Direction = SECURITY_FTDC_D_RepayStock;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_Open;
  } else if (direction == "SELLTOPAY") {
    order_field.Direction = SECURITY_FTDC_D_RepayMargin;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_Open;
  } else if (direction == "PAYBYCASH") {
    order_field.Direction = SECURITY_FTDC_D_DirectRepayMargin;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_Open;
  } else if (direction == "PAYBYSTOCK") {
    order_field.Direction = SECURITY_FTDC_D_DirectRepayStock;
    order_field.CombOffsetFlag[0] = SECURITY_FTDC_OF_Open;
  }

  return order_field;
}

CSecurityFtdcRspInfoField ImplBOFtdcTraderApi::ToRspField(
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

const char* time_now() {
  static char timestamp_str[32];
  time_t rawtime;
  struct tm *timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  
  struct timeval tv;
  gettimeofday(&tv, NULL);
  
  snprintf(timestamp_str, sizeof(timestamp_str), 
           "%d%02d%02d-%02d:%02d:%02d.%03ld", 1900 + timeinfo->tm_year,
           1 + timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_hour,
           timeinfo->tm_min, timeinfo->tm_sec, tv.tv_usec / 1000);
  return timestamp_str;
}
