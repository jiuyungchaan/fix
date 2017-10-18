
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "BOFtdcTraderApi.h"
//#include "BOFtdcQueryApi.h"
#include "order.h"
#include "utils.h"

class FixTrader : public BOFtdcTraderSpi {
 public:
  FixTrader();
  FixTrader(const std::string &account, const std::string &password, const std::string &front_addr);
  virtual ~FixTrader();

  void Init();
  void ReqOrderInsert(Order *order);
  void ReqOrderAction(int order_id);
  void ReqQryInvestorPosition();
  void ReqUserLogout();

  virtual void OnFrontConnected();
  virtual void OnFrontDisconnected(int nReason);
  virtual void OnRspUserLogin(CSecurityFtdcRspUserLoginField *pRspUserLogin, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
  virtual void OnRspUserLogout(CSecurityFtdcUserLogoutField *pUserLogout, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
  virtual void OnRspFetchAuthRandCode(CSecurityFtdcAuthRandCodeField *pAuthRandCode, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
  virtual void OnRspOrderInsert(CSecurityFtdcInputOrderField *pInputOrder, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
  virtual void OnRspOrderAction(CSecurityFtdcInputOrderActionField *pInputOrderAction, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
  // virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
  // virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};
  virtual void OnRspError(CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
  virtual void OnRtnOrder(CSecurityFtdcOrderField *pOrder);
  virtual void OnRtnTrade(CSecurityFtdcTradeField *pTrade);

 private:
  BOFtdcTraderApi *trader_api_;
  std::string user_id_;
  std::string password_;
  std::string front_addr_;
  OrderPool order_pool_;
};

/*
class FixQuery : public COXFtdcQuerySpi {
 public:
  FixQuery();
  FixQuery(const std::string &account, const std::string &password, const std::string &front_addr);
  virtual ~FixQuery();

  void Init();
  // void ReqOrderInsert(Order *order);
  // void ReqOrderAction(int order_id);
  void ReqQryInvestorPosition();
  void ReqQryCreditStockAssignInfo();
  void ReqQryTradingAccount();
  void ReqUserLogout();

  virtual void OnFrontConnected();
  virtual void OnFrontDisconnected(int nReason);
  virtual void OnRspUserLogin(CSecurityFtdcRspUserLoginField *pRspUserLogin, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
  virtual void OnRspUserLogout(CSecurityFtdcUserLogoutField *pUserLogout, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
  virtual void OnRspFetchAuthRandCode(CSecurityFtdcAuthRandCodeField *pAuthRandCode, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
  // virtual void OnRspOrderInsert(CSecurityFtdcInputOrderField *pInputOrder, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
  // virtual void OnRspOrderAction(CSecurityFtdcInputOrderActionField *pInputOrderAction, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
  virtual void OnRspQryInvestorPosition(CSecurityFtdcInvestorPositionField *pInvestorPosition, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
  virtual void OnRspQryTradingAccount(CSecurityFtdcTradingAccountField *pTradingAccount, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
  virtual void OnRspQryCreditStockAssignInfo(CSecurityFtdcCreditStockAssignInfoField *pCreditStockAssignInfo, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
  // virtual void OnRspError(CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
  // virtual void OnRtnOrder(CSecurityFtdcOrderField *pOrder);
  // virtual void OnRtnTrade(CSecurityFtdcTradeField *pTrade);

 private:
  COXFtdcQueryApi *trader_api_;
  std::string user_id_;
  std::string password_;
  std::string front_addr_;
  OrderPool order_pool_;
};
*/

using namespace std;

FixTrader::FixTrader() {
  // TODO
  user_id_ = "3T7004N";
  password_ = "4PVSK";
  user_id_ = "W80004N";
  password_ = "JY8FR";
  user_id_ = "OBCJDDN";
  password_ = "dense";
}

FixTrader::FixTrader(const string &account, const string &password, const string &front_addr) {
  user_id_ = account;
  password_ = password;
  front_addr_ = front_addr;
}

FixTrader::~FixTrader() {
  // TODO
}

void FixTrader::Init() {
  // string config_file = user_id_ + ".cfg";
  trader_api_ = BOFtdcTraderApi::CreateFtdcTraderApi();
  trader_api_->RegisterSpi(this);
  trader_api_->RegisterFront(const_cast<char*>(front_addr_.c_str()));
  trader_api_->Init();
}

void FixTrader::OnFrontConnected() {
  cout << "OnFrontConnected" << endl;
  CSecurityFtdcReqUserLoginField login;
  memset(&login, 0, sizeof(login));

  strcpy(login.UserID, user_id_.c_str());
  strcpy(login.Password, password_.c_str());

  trader_api_->ReqUserLogin(&login, 0);
}

void FixTrader::OnFrontDisconnected(int nReason) {
  cout << "OnFrontDisconnected " << nReason << endl;
}

void FixTrader::OnRspUserLogin(CSecurityFtdcRspUserLoginField *pRspUserLogin,
      CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
  cout << "OnRspUserLogin: " << pRspUserLogin->MaxOrderRef << endl;
  int last_order_id = strtol(pRspUserLogin->MaxOrderRef, NULL, 10) / 100;
  order_pool_.reset_sequence(last_order_id);
}

void FixTrader::OnRspUserLogout(CSecurityFtdcUserLogoutField *pUserLogout,
      CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
  cout << "OnRspUserLogout" << endl;
}

void FixTrader::OnRspFetchAuthRandCode(CSecurityFtdcAuthRandCodeField *pAuthRandCode,
      CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
  cout << "OnRspFetchAuthRandCode:" << pAuthRandCode->RandCode << endl;
}

void FixTrader::OnRspOrderInsert(CSecurityFtdcInputOrderField *pInputOrder,
      CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
  cout << "OnRspOrderInsert:" << time_now_ms() << " - " << pInputOrder->OrderRef << endl;
}

void FixTrader::OnRspOrderAction(CSecurityFtdcInputOrderActionField *pInputOrderAction, 
      CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
  cout << "OnRspOrderAction" << endl;
}

// void FixTrader::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition,
//       CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
//   cout << "OnRspQryInvestorPosition:" << pInvestorPosition->InstrumentID
//        << " " << pInvestorPosition->PosiDirection 
//        << " " << pInvestorPosition->YdPosition
//        << " " << pInvestorPosition->Position
//        << " " << pInvestorPosition->TradingDay << endl;
// }

void FixTrader::OnRspError(CSecurityFtdcRspInfoField *pRspInfo,
      int nRequestID, bool bIsLast) {
  cout << "OnRspError" << endl;
}

void FixTrader::OnRtnOrder(CSecurityFtdcOrderField *pOrder) {
  cout << "OnRtnOrder:" << time_now_ms() << "-" << pOrder->OrderRef << endl;
  int order_id = strtol(pOrder->OrderRef, NULL, 10) / 100;
  Order *order = order_pool_.get(order_id);
  if (order == NULL) {
    cout << "Order-" << order_id << " not found!" << endl;
    return;
  }
  if (pOrder->OrderStatus == SECURITY_FTDC_OST_NoTradeQueueing) {
    snprintf(order->sys_order_id, sizeof(order->sys_order_id),
             "%s", pOrder->OrderSysID);
    order_pool_.add_pair(order->sys_order_id, order_id);
  }
}

void FixTrader::OnRtnTrade(CSecurityFtdcTradeField *pTrade) {
  cout << "OnRtnTrade- " << time_now_ms() << "-"
      << pTrade->OrderRef << " "
      << pTrade->Price << " " << pTrade->Volume << endl;
}

void FixTrader::ReqOrderInsert(Order *order) {
  int order_id = order_pool_.add(order);
  CSecurityFtdcInputOrderField req;
  memset(&req, 0, sizeof(req));

  snprintf(req.OrderRef, sizeof(req.OrderRef), "%d", order_id*100);
  snprintf(req.InstrumentID, sizeof(req.InstrumentID),
           "%s", order->instrument_id);
  if (order->instrument_id[0] == '6') {
    snprintf(req.ExchangeID, sizeof(req.ExchangeID), "SH");
  } else {
    snprintf(req.ExchangeID, sizeof(req.ExchangeID), "SZ");
  }
  if (order->direction == kDirectionBuy) {
    req.Direction = SECURITY_FTDC_D_Buy;
  } else if (order->direction == kDirectionSell){
    req.Direction  = SECURITY_FTDC_D_Sell;
  } else if (order->direction == kDirectionCollateralBuy){
    req.Direction  = SECURITY_FTDC_D_BuyCollateral;
  } else if (order->direction == kDirectionCollateralSell){
    req.Direction  = SECURITY_FTDC_D_SellCollateral;
  } else if (order->direction == kDirectionBorrowToBuy){
    req.Direction  = SECURITY_FTDC_D_MarginTrade;
  } else if (order->direction == kDirectionBorrowToSell){
    req.Direction  = SECURITY_FTDC_D_ShortSell;
  } else if (order->direction == kDirectionBuyToPay){
    req.Direction  = SECURITY_FTDC_D_RepayStock;
  } else if (order->direction == kDirectionSellToPay){
    req.Direction  = SECURITY_FTDC_D_RepayMargin;
  }
  if (order->offset == kOffsetOpen) {
    req.CombOffsetFlag[0] = SECURITY_FTDC_OF_Open;
  } else if (order->offset == kOffsetClose){
    req.CombOffsetFlag[0] = SECURITY_FTDC_OF_Close;
  } else if (order->offset == kOffsetCloseToday){
    req.CombOffsetFlag[0] = SECURITY_FTDC_OF_CloseToday;
  } else if (order->offset == kOffsetCloseYesterday){
    req.CombOffsetFlag[0] = SECURITY_FTDC_OF_CloseYesterday;
  } else if (order->offset == kOffsetForceClose){
    req.CombOffsetFlag[0] = SECURITY_FTDC_OF_ForceOff;
  }
  if (order->order_type != kOrderTypeLimit) {
    req.OrderPriceType = SECURITY_FTDC_OPT_AnyPrice;
  } else {
    req.OrderPriceType = SECURITY_FTDC_OPT_LimitPrice;
  }

  strcpy(req.InvestorID, order->account);
  req.VolumeTotalOriginal = order->volume;
  snprintf(req.LimitPrice, sizeof(req.LimitPrice), "%lf",
           order->limit_price);

  cout << "ReqOrderInsert:" << time_now_ms() << "-" << req.OrderRef << endl;
  trader_api_->ReqOrderInsert(&req, 0);
/*
  snprintf(req.InstrumentID, sizeof(req.InstrumentID),
           "%s", "002232");
  trader_api_->ReqOrderInsert(&req, 0);
  snprintf(req.InstrumentID, sizeof(req.InstrumentID),
           "%s", "002142");
  trader_api_->ReqOrderInsert(&req, 0);
  snprintf(req.InstrumentID, sizeof(req.InstrumentID),
           "%s", "000816");
  trader_api_->ReqOrderInsert(&req, 0);// */
  cout << "After ReqOrderInsert:" << time_now_ms() << "-" << req.OrderRef << endl;
}

void FixTrader::ReqOrderAction(int order_id) {
  Order *action_order = new Order();
  int action_order_id = order_pool_.add(action_order);
  Order *order = order_pool_.get(order_id);
  if (order == NULL) {
    cout << "Order-" << order_id << " not found!" << endl;
    return;
  }

  CSecurityFtdcInputOrderActionField req;
  memset(&req, 0, sizeof(req));

  req.OrderActionRef = action_order_id*100;
  snprintf(req.OrderRef, sizeof(req.OrderRef), "%d", order_id*100);
  snprintf(req.InvestorID, sizeof(req.InvestorID), "%s", order->account);
  // snprintf(req.OrderSysID, sizeof(req.OrderSysID), "%s", order->sys_order_id);
  snprintf(req.InstrumentID, sizeof(req.InstrumentID), "%s",
           order->instrument_id);  
  // if (order->direction == kDirectionBuy) {
  //   req.ExchangeID[0] = THOST_FTDC_D_Buy;
  // } else {
  //   req.ExchangeID[0] = THOST_FTDC_D_Sell;
  // }
  trader_api_->ReqOrderAction(&req, 0);

  cout << "ReqOrderAction" << endl;
}

// void FixTrader::ReqQryInvestorPosition() {
//   CThostFtdcQryInvestorPositionField req;
//   memset(&req, 0, sizeof(req));
//   trader_api_->ReqQryInvestorPosition(&req, 0);
// }

void FixTrader::ReqUserLogout() {
  trader_api_->ReqUserLogout(NULL, 0);
}


/*
FixQuery::FixQuery() {
  // TODO
  user_id_ = "3T7004N";
  password_ = "4PVSK";
  user_id_ = "W80004N";
  password_ = "JY8FR";
  user_id_ = "OBCJDDN";
  password_ = "dense";
}

FixQuery::FixQuery(const string &account, const string &password, const string &front_addr) {
  user_id_ = account;
  password_ = password;
  front_addr_ = front_addr;
}

FixQuery::~FixQuery() {
  // TODO
}

void FixQuery::Init() {
  // string config_file = user_id_ + ".cfg";
  trader_api_ = COXFtdcQueryApi::CreateFtdcQueryApi();
  trader_api_->RegisterSpi(this);
  trader_api_->RegisterFront(const_cast<char*>(front_addr_.c_str()));
  trader_api_->Init();
}

void FixQuery::OnFrontConnected() {
  cout << "OnFrontConnected" << endl;
  CSecurityFtdcReqUserLoginField login;
  memset(&login, 0, sizeof(login));

  strcpy(login.UserID, user_id_.c_str());
  strcpy(login.Password, password_.c_str());

  trader_api_->ReqUserLogin(&login, 0);
}

void FixQuery::OnFrontDisconnected(int nReason) {
  cout << "OnFrontDisconnected " << nReason << endl;
}

void FixQuery::OnRspUserLogin(CSecurityFtdcRspUserLoginField *pRspUserLogin,
      CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
  cout << "OnRspUserLogin: " << pRspUserLogin->MaxOrderRef << endl;
  int last_order_id = strtol(pRspUserLogin->MaxOrderRef, NULL, 10) / 100;
  order_pool_.reset_sequence(last_order_id);
}

void FixQuery::OnRspUserLogout(CSecurityFtdcUserLogoutField *pUserLogout,
      CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
  cout << "OnRspUserLogout" << endl;
}

void FixQuery::OnRspFetchAuthRandCode(CSecurityFtdcAuthRandCodeField *pAuthRandCode,
      CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
  cout << "OnRspFetchAuthRandCode:" << pAuthRandCode->RandCode << endl;
}

void FixQuery::OnRspQryInvestorPosition(CSecurityFtdcInvestorPositionField *pInvestorPosition,
      CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
  cout << "OnRspQryInvestorPosition:" << pInvestorPosition->InstrumentID
       << " " << pInvestorPosition->PosiDirection 
       << " " << pInvestorPosition->YdPosition
       << " " << pInvestorPosition->Position
       << " " << pInvestorPosition->TradingDay << endl;
}

void FixQuery::OnRspQryTradingAccount(CSecurityFtdcTradingAccountField *pTradingAccount,
      CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
  cout << "OnRspQryTradingAccount:" << pTradingAccount->Balance
       << " " << pTradingAccount->Available
       << " " << pTradingAccount->StockValue << endl;
}

void FixQuery::OnRspQryCreditStockAssignInfo(CSecurityFtdcCreditStockAssignInfoField *pCreditStockAssignInfo,
      CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
  cout << "OnRspQryCreditStockAssignInfo:" << pCreditStockAssignInfo->InvestorID
       << " " << pCreditStockAssignInfo->InstrumentID
       << "." << pCreditStockAssignInfo->ExchangeID
       << " TotalMarginStock:" << pCreditStockAssignInfo->LimitVolume
       << " AvailMarginStock:" << pCreditStockAssignInfo->LeftVolume << endl;
}

void FixQuery::ReqQryInvestorPosition() {
  CSecurityFtdcQryInvestorPositionField req;
  memset(&req, 0, sizeof(req));
  trader_api_->ReqQryInvestorPosition(&req, 0);
}

void FixQuery::ReqQryTradingAccount() {
  CSecurityFtdcQryTradingAccountField req;
  memset(&req, 0, sizeof(req));
  trader_api_->ReqQryTradingAccount(&req, 0);
}

void FixQuery::ReqQryCreditStockAssignInfo() {
  CSecurityFtdcQryCreditStockAssignInfoField req;
  memset(&req, 0, sizeof(req));
  trader_api_->ReqQryCreditStockAssignInfo(&req, 0);
}

void FixQuery::ReqUserLogout() {
  trader_api_->ReqUserLogout(NULL, 0);
}
*/

char ParseToOrderType(string order_type) {
  char ret = kOrderTypeLimit;
  if (strcasecmp(order_type.c_str(), "limit") == 0) {
    ret = kOrderTypeLimit;
  } else if (strcasecmp(order_type.c_str(), "market") == 0) {
    ret = kOrderTypeMarket;
  } else if (strcasecmp(order_type.c_str(), "market_limit") == 0) {
    ret = kOrderTypeMarketLimit;
  } else if (strcasecmp(order_type.c_str(), "stop") == 0) {
    ret = kOrderTypeStop;
  } else if (strcasecmp(order_type.c_str(), "stop_limit") == 0) {
    ret = kOrderTypeStopLimit;
  }
  return ret;
}

char ParseToDirection(string direction) {
  char ret = kDirectionBuy;
  if (strcasecmp(direction.c_str(), "buy") == 0) {
    ret = kDirectionBuy;
  } else if (strcasecmp(direction.c_str(), "sell") == 0){
    ret = kDirectionSell;
  } else if (strcasecmp(direction.c_str(), "collateralbuy") == 0){
    ret = kDirectionCollateralBuy;
  } else if (strcasecmp(direction.c_str(), "collateralsell") == 0){
    ret = kDirectionCollateralSell;
  } else if (strcasecmp(direction.c_str(), "borrowtobuy") == 0){
    ret = kDirectionBorrowToBuy;
  } else if (strcasecmp(direction.c_str(), "borrowtosell") == 0){
    ret = kDirectionBorrowToSell;
  } else if (strcasecmp(direction.c_str(), "buytopay") == 0){
    ret = kDirectionBuyToPay;
  } else if (strcasecmp(direction.c_str(), "selltopay") == 0){
    ret = kDirectionSellToPay;
  }
  return ret;
}

char ParseToTimeInForce(string time_in_force) {
  char ret = kTimeInForceDay;
  if (strcasecmp(time_in_force.c_str(), "day") == 0) {
    ret = kTimeInForceDay;
  } else if (strcasecmp(time_in_force.c_str(), "gtc") == 0) {
    ret = kTimeInForceGTC;
  } else if (strcasecmp(time_in_force.c_str(), "fak") == 0) {
    ret = kTimeInForceFAK;
  } else if (strcasecmp(time_in_force.c_str(), "gtd") == 0) {
    ret = kTimeInForceGTD;
  }
  return ret;
}

char ParseToOffset(string offset) {
  char ret = kOffsetOpen;
  if (strcasecmp(offset.c_str(), "close") == 0) {
    ret = kOffsetClose;
  } else if (strcasecmp(offset.c_str(), "closetoday") == 0) {
    ret = kOffsetCloseToday;
  } else if (strcasecmp(offset.c_str(), "closeyesterday") == 0) {
    ret = kOffsetCloseYesterday;
  } else if (strcasecmp(offset.c_str(), "forceclose") == 0) {
    ret = kOffsetForceClose;
  }
  return ret;
}


int main(int argc, char **argv) {
  int arg_pos = 1;
  string config_file_name = "./test.cfg";
  string cmd_file_name = "";
  // string account = "410082065696";
  // string account = "410009987078";
  string account = "410009980106";
  string password = "JY8FR";
  string front_addr = "tcp://112.74.17.91:5555";
  while (arg_pos < argc) {
    if (strcmp(argv[arg_pos], "-f") == 0) {
      front_addr = argv[++arg_pos];
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
  // FixTrader fix_trader("410082065696", password, front_addr);
  FixTrader fix_trader(account, password, front_addr);
  //FixQuery fix_query(account, password, front_addr);
  fix_trader.Init();
  //fix_query.Init();

  sleep(10);

  fstream cmd_file;
  cmd_file.open(cmd_file_name, fstream::in | fstream::app);
  while(!cmd_file.eof()) {
    char line[512];
    cmd_file.getline(line, 512);
    if (line[0] == '#') {
      cout << "\033[32mComment:[" << line << "]\033[0m" << endl;
      continue;
    } else if (strcmp(line, "") == 0) {
      cout << "Passing blank line..." << endl;
      continue;
    }
    cout << "\033[31mCommand:[" << line << "]\033[0m" << endl;
    char cmd;
    while(true) {
      cout << "\033[33mInput command-[E-Execute P-Pass Q-Quit] :\033[0m";
      scanf("%c", &cmd);
      getchar();
      if (cmd == 'E' || cmd == 'e' || cmd == 'P' || cmd == 'p' ||
          cmd == 'Q' || cmd == 'q') {
        break;
      } else {
        cout << "Invalid command : " << cmd << endl;
      }
    }
    if (cmd == 'E' || cmd == 'e') {
      istringstream line_stream(line);
      string request;
      line_stream >> request;
      if (strcasecmp(request.c_str(), "insert") == 0) {
        string symbol, instrument, price, stop_price, volume, direction, 
               offset, order_type, time_in_force;
        line_stream >> symbol >> instrument >> price >> stop_price >> volume 
                    >> direction >> offset >> order_type >> time_in_force;
        Order *order = new Order();
        snprintf(order->account, sizeof(order->account), "%s", account.c_str());
        snprintf(order->symbol, sizeof(order->symbol), "%s", symbol.c_str());
        snprintf(order->instrument_id, sizeof(order->instrument_id), "%s", instrument.c_str());
        order->limit_price = atof(price.c_str());
        order->stop_price = atof(stop_price.c_str());
        order->volume = atoi(volume.c_str());
        order->direction = ParseToDirection(direction);
        order->order_type = ParseToOrderType(order_type);
        order->offset = ParseToOffset(offset);
        order->time_in_force = ParseToTimeInForce(time_in_force);
        fix_trader.ReqOrderInsert(order);
      } else if (strcasecmp(request.c_str(), "cancel") == 0) {
        string order_id;
        line_stream >> order_id;
        if (order_id == "-1") {
          string symbol, instrument, side, local_id, sys_id;
          cout << "\033[33mInput order info to cancel:\033[0m" << endl;
          cin >> symbol >> instrument >> side >> local_id >> sys_id;
          getchar();
          // fix_trader.ReqOrderAction(symbol, instrument, side, local_id, sys_id,
          //                           account);
        } else {
          Order *order = new Order();
          order->orig_order_id = atoi(order_id.c_str());
          fix_trader.ReqOrderAction(order->orig_order_id);
        }
      } else if (strcasecmp(request.c_str(), "position") == 0) {
        //fix_query.ReqQryInvestorPosition();
      } else if (strcasecmp(request.c_str(), "account") == 0) {
        //fix_query.ReqQryTradingAccount();
      } else if (strcasecmp(request.c_str(), "marginposition") == 0) {
        //fix_query.ReqQryCreditStockAssignInfo();
      }
      sleep(3);
    } else if (cmd == 'P' || cmd == 'p') {
      continue;
    } else if (cmd == 'Q' || cmd == 'q') {
      cout << "Quit ..." << endl;
      break;
    }
  }

  char q;
  cout << "Confirm to quit ? y/n :";
  scanf("%c", &q);
  getchar();

  // string account = "3T7004N";
  // string symbol = "GE";
  // string instrument = "000060";
  // string price = "10.2";
  // string volume = "12200";
  // string direction = "buy";
  // string order_type = "limit";
  // string time_in_force = "day";
  // Order *order = new Order();
  // snprintf(order->account, sizeof(order->account), "%s", account.c_str());
  // snprintf(order->symbol, sizeof(order->symbol), "%s", symbol.c_str());
  // snprintf(order->instrument_id, sizeof(order->instrument_id), "%s",
  //          instrument.c_str());
  // order->limit_price = atof(price.c_str());
  // // order->stop_price = atof(stop_price.c_str());
  // order->volume = atoi(volume.c_str());
  // order->direction = kDirectionSell;
  // order->order_type = kOrderTypeLimit;
  // order->offset = kOffsetOpen;
  // order->time_in_force = kTimeInForceDay;
  // fix_trader.ReqOrderInsert(order);

  // sleep(13);

  // fix_trader.ReqOrderAction(1);

  sleep(10);

  // int order_id;
  // cout << "Please input the order-ID to cancel: ";
  // cin >> order_id;
  // if (order_id != -1) {
  //   fix_trader.ReqOrderAction(order_id);
  // }
  // sleep(5);

  fix_trader.ReqUserLogout();
  //fix_query.ReqUserLogout();
  sleep(5);
  return 0;
}
