
#include <iostream>
#include <string>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "../api/include/FixFtdcTraderApi.h"
#include "order.h"

class FixTrader : public CFixFtdcTraderSpi {
 public:
  FixTrader();
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
}

FixTrader::~FixTrader() {
  // TODO
}

void FixTrader::Init() {
  trader_api_ = CFixFtdcTraderApi::CreateFtdcTraderApi("./test.cfg");
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
  cout << "OnRspUserLogin" << endl;
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
}

void FixTrader::OnRtnTrade(CThostFtdcTradeField *pTrade) {
  cout << "OnRtnTrade" << endl;
}

void FixTrader::ReqOrderInsert(Order *order) {
  int order_id = order_pool_.add(order);
  CThostFtdcInputOrderField req;
  memset(&req, 0, sizeof(req));

  snprintf(req.OrderRef, sizeof(req.OrderRef), "%d", order_id);
  snprintf(req.InstrumentID, sizeof(req.InstrumentID),
           "%s", order->instrument_id);
  if (order->direction == kDirectionBuy) {
    req.Direction = THOST_FTDC_D_Buy;
  } else {
    req.Direction  = THOST_FTDC_D_Sell;
  }

  strcpy(req.InvestorID, order->account);
  req.VolumeTotalOriginal = order->volume;

  trader_api_->ReqOrderInsert(&req, 0);
}

void FixTrader::ReqOrderAction(int order_id) {
  cout << "ReqOrderAction" << endl;
}

void FixTrader::ReqUserLogout() {
  trader_api_->ReqUserLogout(NULL, 0);
}

int main() {
  FixTrader fix_trader;
  fix_trader.Init();

  string account = "3T7004N";
  string symbol = "GE";
  string instrument = "GEZ8";
  string price = "3866";
  string volume = "4";
  string direction = "buy";
  string order_type = "market";
  string time_in_force = "day";
  Order *order = new Order();
  snprintf(order->account, sizeof(order->account), "%s", account.c_str());
  snprintf(order->symbol, sizeof(order->symbol), "%s", symbol.c_str());
  snprintf(order->instrument_id, sizeof(order->instrument_id), "%s", instrument.c_str());
  order->limit_price = atof(price.c_str());
  // order->stop_price = atof(stop_price.c_str());
  order->volume = atoi(volume.c_str());
  order->direction = kDirectionBuy;
  order->order_type = kOrderTypeMarket;
  order->time_in_force = kTimeInForceDay;
  fix_trader.ReqOrderInsert(order);

  sleep(5);
  fix_trader.ReqUserLogout();
  sleep(5);
  return 0;
}
