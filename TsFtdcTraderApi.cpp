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
#include <netinet/in.h>
#include <pthread.h>

#include <string>
#include <map>
#include <fstream>
#include <cctype>
#include <algorithm>
#include <vector>


void split(const std::string& str, const std::string& del, std::vector<string>& v);

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
    CThostFtdcInputOrderField basic_order;
    char order_flow_id[64];

    InputOrder();
    InputOrder(CThostFtdcInputOrderField *pInputOrder);
  };

  class OrderPool {
   public:
    static const int MAX_ORDER_SIZE = 900000;

    OrderPool();
    InputOrder *get(int order_id);
    InputOrder *get(std::string sys_id);
    InputOrder *add(CThostFtdcInputOrderField *pInputOrder);
    void add_pair(std::string sys_id, int local_id);

   private:
    InputOrder *order_pool_[MAX_ORDER_SIZE];
    std::map<std::string, int> sys_to_local_;
  };

  static void recv_thread(void *obj);
  static void callback_onlogin(void *obj);
  static void callback_onlogout(void *obj);

  void decode(const char *message);

  CThostFtdcOrderField ToOrderField();
  CThostFtdcTradeField ToTradeField();
  CThostFtdcInputOrderField ToInputOrderField();
  CThostFtdcInputOrderActionField ToActionField();
  CThostFtdcRspInfoField ToRspField();

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

ImplTsFtdcTraderApi::InputOrder::InputOrder() : order_flow_id{0} {
  memset(&basic_order, 0, sizeof(basic_order));
}

ImplTsFtdcTraderApi::InputOrder::InputOrder(
      CThostFtdcInputOrderField *pInputOrder) : order_flow_id{0} {
  memcpy(&basic_order, pInputOrder, sizeof(basic_order));
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
    front_addr_{0}, user_id_{0}, password_{0} {
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

void ImplTsFtdcTraderApi::recv_thread(void *obj) {
  ImplTsFtdcTraderApi *self = (ImplTsFtdcTraderApi *)obj;
  int fd = self->server_fd_;
  int recv_len;
  char buffer[512];
  while(recv_len = recv(fd, buffer, 512, 0) > 0) {
    self->decode(buffer);
  }
  printf("Failed to receive data! Exit thread...\n");
}

void ImplTsFtdcTraderApi::decode(const char *message) {

}

void ImplTsFtdcTraderApi::Init() {
  // connect server
  if (strcmp(front_addr_, "") == 0) {
    printf("ERROR: Front Address is not registered!\n");
    return ;
  }
  vector<string> vals;
  split(front_addr_, "//", vals);
  if (vals.size() == 2) { 
    vector<string> addr;
    split(vals[1], ":", addr);
    if (addr.size() == 2) {
      int port = atoi(addr[1].c_str());
      if (server_fd_ = socket(AF_INET, SOCK_STREAM, 0) < 0) {
        printf("Failed to create socket: %d-%s\n", strerror(errno), errno);
        exit(0);
      }
      char ip_addr[32];
      snprintf(ip_addr, sizeof(ip_addr), "%s", addr[0].c_str());
      memset(&server_addr_, 0, sizeof(server_addr_));
      server_addr_.sin_family = AF_INET;
      server_addr_.sin_port = htons(port);
      if (inet_pton(AF_INET, ip_addr, &server_addr_.sin_addr) <= 0) {
        printf("Failed to parse ip address: %s-%d-%s\n", ip_addr, 
               strerror(errno), errno);
        trader_spi_->OnFrontDisconnected(1001);
        return;
      }
      if (connect(server_fd_, (struct sockaddr*)&server_addr_, 
                  sizeof(server_addr_)) < 0) {
        printf("Failed to connect: [%s:%d]-%d-%s\n", ip_addr, port, 
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


void ImplTsFtdcTraderApi::RegisterSpi(CFixFtdcTraderSpi *pSpi) {
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

void ImplTsFtdcTraderApi::callback_onlogin(void *obj) {
  sleep(1);
  ImplTsFtdcTraderApi *self = (ImplTsFtdcTraderApi *)obj;
  CThostFtdcRspUserLoginField login_field;
  memset(&login_field, 0, sizeof(login_field));
  self->trader_spi_->OnRspUserLogin(&login_field, NULL, 0, true);  
}

void ImplTsFtdcTraderApi::callback_onlogout(void *obj) {
  sleep(1);
  ImplTsFtdcTraderApi *self = (ImplTsFtdcTraderApi *)obj;
  CThostFtdcUserLogoutField logout_field;
  memset(&logout_field, 0, sizeof(logout_field));
  CThostFtdcRspInfoField info_field;
  memset(&info_field, 0, sizeof(info_field));
  trader_spi_->OnRspUserLogout(&logout_field, &info_field, 0, true);  
}

int ImplTsFtdcTraderApi::ReqUserLogin(
      CThostFtdcReqUserLoginField *pReqUserLoginField, int nRequestID) {
  snprintf(user_id_, sizeof(user_id_), "%s", pReqUserLoginField->UserID);
  snprintf(password_, sizeof(password_), "%s", pReqUserLoginField->Password);
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
  char action[16], symbol[16], type[16], duration[16], message[512];
  
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

  snprintf(duration, sizeof(duration), "DAY");

  snprintf(message, sizeof(message), "COMMAND=SENDORDER;ACCOUNT=%s;ACTION=%s;" \
           "SYMBOL=%s;QUANTITY=%d;TYPE=LIMIT;LIMITPRICE=%lf;DURATION=%s;" \
           "CLIENTID=%s",
           user_id_, action, symbol, pInputOrder->VolumeTotalOriginal,
           pInputOrder->LimitPrice, duration, pInputOrder->OrderRef);
  int ret = send(server_fd_, message, strlen(message), 0);
  if (ret < 0) {
    printf("Failed to send message - %d\n", ret);
  }
  return ret;
}

int ImplTsFtdcTraderApi::ReqOrderAction(
      CThostFtdcInputOrderActionField *pInputOrderAction, int nRequestID) {
  char message[512];
  snprintf(message, sizeof(message), "COMMAND=CANCELORDER;CLIENTID=%s",
           pInputOrderAction->OrderRef);
  int ret = send(server_fd_, message, strlen(message), 0);
  if (ret < 0) {
    printf("Failed to send message - %d\n", ret);
  }
  return ret;
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
