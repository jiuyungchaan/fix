
#include <iostream>
#include <string>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "FixFtdcTraderApi.h"
#include "order.h"

class FixTrader : public CFixFtdcTraderSpi {
 public:
  FixTrader();
  FixTrader(const std::string &account, const std::string &password);
  virtual ~FixTrader();

  void Init();
  void ReqOrderInsert(Order *order);
  void ReqOrderAction(int order_id);
  void ReqUserLogout();

  virtual void OnFrontConnected();
  virtual void OnFrontDisconnected(int nReason);
  virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
  virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
  virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
  virtual void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
  virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
  virtual void OnRtnOrder(CThostFtdcOrderField *pOrder);
  virtual void OnRtnTrade(CThostFtdcTradeField *pTrade);

 private:
  CFixFtdcTraderApi *trader_api_;
  std::string user_id_;
  std::string password_;
  OrderPool order_pool_;
};

using namespace std;

FixTrader::FixTrader() {
  // TODO
  user_id_ = "3T7004N";
  password_ = "4PVSK";
  user_id_ = "W80004N";
  password_ = "JY8FR";
}

FixTrader::FixTrader(const string &account, const string &password) {
  user_id_ = account;
  password_ = password;
}

FixTrader::~FixTrader() {
  // TODO
}

void FixTrader::Init() {
  string config_file = user_id_ + ".cfg";
  trader_api_ = CFixFtdcTraderApi::CreateFtdcTraderApi(config_file.c_str());
  trader_api_->RegisterSpi(this);
  trader_api_->RegisterFront(const_cast<char *>("NULL"));
  trader_api_->Init();
}

void FixTrader::OnFrontConnected() {
  cout << "OnFrontConnected" << endl;
  CThostFtdcReqUserLoginField login;
  memset(&login, 0, sizeof(login));

  strcpy(login.UserID, user_id_.c_str());
  strcpy(login.Password, password_.c_str());

  trader_api_->ReqUserLogin(&login, 0);
}

void FixTrader::OnFrontDisconnected(int nReason) {
  cout << "OnFrontDisconnected " << nReason << endl;
}

void FixTrader::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
      CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
  cout << "OnRspUserLogin: " << pRspUserLogin->MaxOrderRef << endl;
  int last_order_id = strtol(pRspUserLogin->MaxOrderRef, NULL, 10) / 100;
  order_pool_.reset_sequence(last_order_id);
}

void FixTrader::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout,
      CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
  cout << "OnRspUserLogout" << endl;
}

void FixTrader::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder,
      CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
  cout << "OnRspOrderInsert" << endl;
}

void FixTrader::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, 
      CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
  cout << "OnRspOrderAction" << endl;
}

void FixTrader::OnRspError(CThostFtdcRspInfoField *pRspInfo,
      int nRequestID, bool bIsLast) {
  cout << "OnRspError" << endl;
}

void FixTrader::OnRtnOrder(CThostFtdcOrderField *pOrder) {
  cout << "OnRtnOrder" << endl;
  int order_id = strtol(pOrder->OrderRef, NULL, 10) / 100;
  Order *order = order_pool_.get(order_id);
  if (order == NULL) {
    cout << "Order-" << order_id << " not found!" << endl;
    return;
  }
  if (pOrder->OrderStatus == THOST_FTDC_OST_NoTradeQueueing) {
    snprintf(order->sys_order_id, sizeof(order->sys_order_id),
             "%s", pOrder->OrderSysID);
    order_pool_.add_pair(order->sys_order_id, order_id);
  }
}

void FixTrader::OnRtnTrade(CThostFtdcTradeField *pTrade) {
  cout << "OnRtnTrade" << endl;
}

void FixTrader::ReqOrderInsert(Order *order) {
  int order_id = order_pool_.add(order);
  CThostFtdcInputOrderField req;
  memset(&req, 0, sizeof(req));

  snprintf(req.OrderRef, sizeof(req.OrderRef), "%d", order_id*100);
  snprintf(req.InstrumentID, sizeof(req.InstrumentID),
           "%s", order->instrument_id);
  if (order->direction == kDirectionBuy) {
    req.Direction = THOST_FTDC_D_Buy;
  } else {
    req.Direction  = THOST_FTDC_D_Sell;
  }
  if (order->order_type != kOrderTypeLimit) {
    req.OrderPriceType = THOST_FTDC_OPT_AnyPrice;
  } else {
    req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
  }

  strcpy(req.InvestorID, order->account);
  req.VolumeTotalOriginal = order->volume;
  req.LimitPrice = order->limit_price;

  trader_api_->ReqOrderInsert(&req, 0);
}

void FixTrader::ReqOrderAction(int order_id) {
  Order *action_order = new Order();
  int action_order_id = order_pool_.add(action_order);
  Order *order = order_pool_.get(order_id);
  if (order == NULL) {
    cout << "Order-" << order_id << " not found!" << endl;
    return;
  }

  CThostFtdcInputOrderActionField req;
  memset(&req, 0, sizeof(req));

  req.OrderActionRef = action_order_id*100;
  snprintf(req.OrderRef, sizeof(req.OrderRef), "%d", order_id*100);
  snprintf(req.InvestorID, sizeof(req.InvestorID), "%s", order->account);
  snprintf(req.OrderSysID, sizeof(req.OrderSysID), "%s", order->sys_order_id);
  snprintf(req.InstrumentID, sizeof(req.InstrumentID), "%s",
           order->instrument_id);  
  if (order->direction == kDirectionBuy) {
    req.ExchangeID[0] = THOST_FTDC_D_Buy;
  } else {
    req.ExchangeID[0] = THOST_FTDC_D_Sell;
  }
  trader_api_->ReqOrderAction(&req, 0);

  cout << "ReqOrderAction" << endl;
}

void FixTrader::ReqUserLogout() {
  trader_api_->ReqUserLogout(NULL, 0);
}

int main(int argc, char **argv) {
  int arg_pos = 1;
  string config_file_name = "./test.cfg";
  string cmd_file_name = "";
  string account = "W80004N";
  string password = "JY8FR";
  while (arg_pos < argc) {
    if (strcmp(argv[arg_pos], "-f") == 0) {
      config_file_name = argv[++arg_pos];
    }
    if (strcmp(argv[arg_pos], "-c") == 0) {
      cmd_file_name = argv[++arg_pos];
    }
    if (strcmp(argv[arg_pos], "-a") == 0) {
      account = argv[++arg_pos];
    }
    if (strcmp(argv[arg_pos], "-p") == 0) {
      password = argv[++arg_pos];
    }
    ++arg_pos;
  }
  FixTrader fix_trader(account, password);
  fix_trader.Init();

  sleep(10);

  // string account = "3T7004N";
  string symbol = "GE";
  string instrument = "GEZ8";
  string price = "9776";
  string volume = "4";
  string direction = "buy";
  string order_type = "limit";
  string time_in_force = "day";
  Order *order = new Order();
  snprintf(order->account, sizeof(order->account), "%s", account.c_str());
  snprintf(order->symbol, sizeof(order->symbol), "%s", symbol.c_str());
  snprintf(order->instrument_id, sizeof(order->instrument_id), "%s",
           instrument.c_str());
  order->limit_price = atof(price.c_str());
  // order->stop_price = atof(stop_price.c_str());
  order->volume = atoi(volume.c_str());
  order->direction = kDirectionBuy;
  order->order_type = kOrderTypeLimit;
  order->time_in_force = kTimeInForceDay;
  fix_trader.ReqOrderInsert(order);
  sleep(5);

  int order_id;
  cout << "Please input the order-ID to cancel: ";
  cin >> order_id;
  if (order_id != -1) {
    fix_trader.ReqOrderAction(order_id);
  }
  sleep(5);

  fix_trader.ReqUserLogout();
  sleep(5);
  return 0;
}
