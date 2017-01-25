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

#include <string>
#include <map>
#include <fstream>
#include <cctype>
#include <algorithm>


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

  CThostFtdcOrderField ToOrderField();
  CThostFtdcTradeField ToTradeField();
  CThostFtdcInputOrderField ToInputOrderField();
  CThostFtdcInputOrderActionField ToActionField();
  CThostFtdcRspInfoField ToRspField();

  CTsFtdcTraderSpi *trader_spi_;
  char front_addr_[64];
  char user_id_[64];
  char user_passwd_[64];

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
ImplTsFtdcTraderApi::ImplTsFtdcTraderApi(const char *pszFlowPath) {
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

void ImplTsFtdcTraderApi::Init() {
  // connect server
  
  trader_spi_->OnFrontConnected();
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

