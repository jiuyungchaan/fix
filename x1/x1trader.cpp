#include "x1trader.h"

#include <stdio.h>

X1Trader::X1Trader() {
  // TODO'
  request_id_ = 0;
  logon_ = false;
  api_ = x1ftdcapi::CX1FtdcTraderApi::CreateCX1FtdcTraderApi();
}

X1Trader::~X1Trader() {
  // TODO
  // api_->Release();
}

void X1Trader::Logon(char *pszFrontAddr) {
  int ret = api_->Init(pszFrontAddr, this, 0, 1);
  if (ret == -1) {
    printf("Failed to initialize api! %d\n", ret);
  }
}

void X1Trader::OnFrontConnected() {
  printf("OnFrontConnected\n");
  if (logon_) {
    return;
  }
  api_->SubscribePrivateTopic(X1_PrivateFlow_Req_Quick, 0);
  CX1FtdcReqUserLoginField login;
  memset(&login, 0, sizeof(login));
  login.RequestID = ++request_id_;
  snprintf(login.AccountID, sizeof(login.AccountID), "%s", user_id_.c_str());
  snprintf(login.Password, sizeof(login.Password), "%s", password_.c_str());
  // login.CompanyID = 1;
  int ret = api_->ReqUserLogin(&login);
  if (ret != 0) {
    printf("Failed to login! %d\n", ret);
  } else {
    logon_ = true;
    printf("ReqUserLogin\n");
  }
}

void X1Trader::OnFrontDisconnected() {
  printf("OnFrontDisconnected\n");
}

void X1Trader::OnRspUserLogin(struct CX1FtdcRspUserLoginField* pUserLoginInfoRtn, 
        struct CX1FtdcRspErrorField* pErrorInfo) {
  printf("OnRspUserLogin: RequestID:%ld AccountID:%s LoginResult:%d InitLocalOrderID:%ld SessionID:%ld "
         "ErrorID:%d ErrorMsg:%s DCEtime:%s SHFETime:%s CFFEXTime:%s CZCETime:%s INETime:%s\n",
         pUserLoginInfoRtn->RequestID, pUserLoginInfoRtn->AccountID, pUserLoginInfoRtn->LoginResult,
         pUserLoginInfoRtn->InitLocalOrderID, pUserLoginInfoRtn->SessionID, pUserLoginInfoRtn->ErrorID,
         pUserLoginInfoRtn->ErrorMsg, pUserLoginInfoRtn->DCEtime, pUserLoginInfoRtn->SHFETime,
         pUserLoginInfoRtn->CFFEXTime, pUserLoginInfoRtn->CZCETime, pUserLoginInfoRtn->INETime);
  order_pool_.reset_sequence(pUserLoginInfoRtn->InitLocalOrderID);
  if (pErrorInfo) {
    printf("RspErrorField: RequestID:%ld SessionID:%ld AccountID:%s ErrorID:%d ErrorMsg:%s "
           "X1OrderID:%ld LocalOrderID:%ld InstrumentID:%s\n",
           pErrorInfo->RequestID, pErrorInfo->SessionID,
           pErrorInfo->AccountID, pErrorInfo->ErrorID, pErrorInfo->ErrorMsg,
           pErrorInfo->X1OrderID, pErrorInfo->LocalOrderID, pErrorInfo->InstrumentID);
  }
}

void X1Trader::OnRspUserLogout(struct CX1FtdcRspUserLogoutInfoField* pUserLogoutInfoRtn,
        struct CX1FtdcRspErrorField* pErrorInfo) {
  printf("OnRspUserLogout: RequestID:%ld AccountID:%s LogoutResult:%d ErrorID:%d ErrorMsg:%s\n",
         pUserLogoutInfoRtn->RequestID, pUserLogoutInfoRtn->AccountID, pUserLogoutInfoRtn->LogoutResult,
         pUserLogoutInfoRtn->ErrorID, pUserLogoutInfoRtn->ErrorMsg);
  if (pErrorInfo) {
    printf("RspErrorField: RequestID:%ld SessionID:%ld AccountID:%s ErrorID:%d ErrorMsg:%s "
           "X1OrderID:%ld LocalOrderID:%ld InstrumentID:%s\n",
           pErrorInfo->RequestID, pErrorInfo->SessionID,
           pErrorInfo->AccountID, pErrorInfo->ErrorID, pErrorInfo->ErrorMsg,
           pErrorInfo->X1OrderID, pErrorInfo->LocalOrderID, pErrorInfo->InstrumentID);
  }
}

void X1Trader::OnRspInsertOrder(struct CX1FtdcRspOperOrderField* pOrderRtn,
        struct CX1FtdcRspErrorField* pErrorInfo) {
  printf("OnRspInsertOrder: LocalOrderID:%ld OrderStatus:%d RequestID:%ld Margin:%lf SessionID:%ld\n", 
         pOrderRtn->LocalOrderID, pOrderRtn->OrderStatus, pOrderRtn->RequestID, pOrderRtn->Margin,
         pOrderRtn->SessionID);
  if (pErrorInfo) {
    printf("RspErrorField: RequestID:%ld SessionID:%ld AccountID:%s ErrorID:%d ErrorMsg:%s "
           "X1OrderID:%ld LocalOrderID:%ld InstrumentID:%s\n",
           pErrorInfo->RequestID, pErrorInfo->SessionID,
           pErrorInfo->AccountID, pErrorInfo->ErrorID, pErrorInfo->ErrorMsg,
           pErrorInfo->X1OrderID, pErrorInfo->LocalOrderID, pErrorInfo->InstrumentID);
  }
}

void X1Trader::OnRspCancelOrder(struct CX1FtdcRspOperOrderField* pOrderCanceledRtn,
        struct CX1FtdcRspErrorField* pErrorInfo) {
  printf("OnRspCancelOrder: LocalOrderID:%ld OrderStatus:%d RequestID:%ld Margin:%lf SessionID:%ld X1OrderID%ld\n", 
         pOrderCanceledRtn->LocalOrderID, pOrderCanceledRtn->OrderStatus,
         pOrderCanceledRtn->RequestID, pOrderCanceledRtn->Margin,
         pOrderCanceledRtn->SessionID, pOrderCanceledRtn->X1OrderID);
  if (pErrorInfo) {
    printf("RspErrorField: RequestID:%ld SessionID:%ld AccountID:%s ErrorID:%d ErrorMsg:%s "
           "X1OrderID:%ld LocalOrderID:%ld InstrumentID:%s\n",
           pErrorInfo->RequestID, pErrorInfo->SessionID,
           pErrorInfo->AccountID, pErrorInfo->ErrorID, pErrorInfo->ErrorMsg,
           pErrorInfo->X1OrderID, pErrorInfo->LocalOrderID, pErrorInfo->InstrumentID);
  }
}

void X1Trader::OnRtnErrorMsg(struct CX1FtdcRspErrorField* pErrorInfo) {
  printf("OnRtnErrorMsg: RequestID:%ld SessionID:%ld AccountID:%s ErrorID:%d X1OrderID:%ld LocalOrderID:%ld "
         "ErrorMsg:%s InstrumentID:%s\n",
        pErrorInfo->RequestID, pErrorInfo->SessionID, pErrorInfo->AccountID, pErrorInfo->ErrorID,
        pErrorInfo->X1OrderID, pErrorInfo->LocalOrderID, pErrorInfo->ErrorMsg, pErrorInfo->InstrumentID);
}

void X1Trader::OnRtnMatchedInfo(struct CX1FtdcRspPriMatchInfoField* pRtnMatchData) {
  printf("OnRtnMatchedInfo: LocalOrderID:%ld OrderSysID:%s MatchID:%s InstrumentID:%s BuySellType:%d "
         "OpenCloseType:%d MatchedPrice:%lf OrderAmount:%ld MatchedAmount:%ld MatchedTime:%s X1OrderID:%ld "
         "MatchType:%ld Speculator:%d ExchangeID:%s SessionID:%ld\n",
         pRtnMatchData->LocalOrderID, pRtnMatchData->OrderSysID, pRtnMatchData->MatchID,
         pRtnMatchData->InstrumentID, pRtnMatchData->BuySellType, pRtnMatchData->OpenCloseType,
         pRtnMatchData->MatchedPrice, pRtnMatchData->OrderAmount, pRtnMatchData->MatchedAmount,
         pRtnMatchData->MatchedTime, pRtnMatchData->X1OrderID, pRtnMatchData->MatchType,
         pRtnMatchData->Speculator, pRtnMatchData->ExchangeID, pRtnMatchData->SessionID);
}

void X1Trader::OnRtnOrder(struct CX1FtdcRspPriOrderField* pRtnOrderData) {
  printf("OnRtnOrder: LocalOrderID:%ld X1OrderID:%ld OrderSysID:%s OrderStatus:%d SessionID:%ld "
         "InstrumentID:%s ExchangeID:%s BuySellType:%d OpenCloseType:%d InstrumentType:%d "
         "Speculator:%d InsertPrice:%lf CancelAmount:%ld OrderAmount:%ld ErrorID:%d StatusMsg:%s\n",
         pRtnOrderData->LocalOrderID, pRtnOrderData->X1OrderID, pRtnOrderData->OrderSysID, 
         pRtnOrderData->OrderStatus, pRtnOrderData->SessionID, pRtnOrderData->InstrumentID,
         pRtnOrderData->ExchangeID, pRtnOrderData->BuySellType, pRtnOrderData->OpenCloseType, 
         pRtnOrderData->InstrumentType, pRtnOrderData->Speculator, pRtnOrderData->InsertPrice,
         pRtnOrderData->CancelAmount, pRtnOrderData->OrderAmount, pRtnOrderData->ErrorID,
         pRtnOrderData->StatusMsg);
  Order *order = order_pool_.get(pRtnOrderData->LocalOrderID);
  snprintf(order->sys_order_id, sizeof(order->sys_order_id), "%s", pRtnOrderData->OrderSysID);
  order_pool_.add_pair(pRtnOrderData->OrderSysID, pRtnOrderData->LocalOrderID);
}

void X1Trader::OnRtnCancelOrder(struct CX1FtdcRspPriCancelOrderField* pCancelOrderData) {
  printf("OnRtnCancelOrder: LocalOrderID:%ld OrderSysID:%s CancelAmount:%ld X1OrderID:%ld "
         "CancelTime:%s SessionID:%ld OrderStatus:%d ErrorID:%d StatusMsg:%s\n",
         pCancelOrderData->LocalOrderID, pCancelOrderData->OrderSysID, pCancelOrderData->CancelAmount,
         pCancelOrderData->X1OrderID, pCancelOrderData->CanceledTime, pCancelOrderData->SessionID,
         pCancelOrderData->OrderStatus, pCancelOrderData->ErrorID, pCancelOrderData->StatusMsg);

}

void X1Trader::OnRspQryPosition(struct CX1FtdcRspPositionField* pPositionInfoRtn,
        struct CX1FtdcRspErrorField* pErrorInfo, bool bIsLast) {
  printf("OnRspQryPosition: ExchangeID:%s InstrumentID:%s BuySellType:%d OpenAvgPrice:%lf "
         "PositionAvgPrice:%lf PositionAmount:%ld TotalAvaiAmount:%ld TodayAvaiAmount:%ld "
         "LastAvaiAmount:%ld TodayAmount:%ld LastAmount:%ld TradingAmount:%ld DatePositionPNL:%lf "
         "DateClosePNL:%lf Premium:%lf PNL:%lf Margin:%lf Speculator:%d ClientID:%s "
         "PreSettlementPrice:%lf InstrumentType:%d YesterdayTradingAmount:%ld OptionValue:%lf\n",
         pPositionInfoRtn->ExchangeID, pPositionInfoRtn->InstrumentID, pPositionInfoRtn->BuySellType,
         pPositionInfoRtn->OpenAvgPrice, pPositionInfoRtn->PositionAvgPrice, 
         pPositionInfoRtn->PositionAmount, pPositionInfoRtn->TotalAvaiAmount,
         pPositionInfoRtn->TodayAvaiAmount, pPositionInfoRtn->LastAvaiAmount,
         pPositionInfoRtn->TodayAmount, pPositionInfoRtn->LastAmount, 
         pPositionInfoRtn->TradingAmount, pPositionInfoRtn->DatePositionProfitLoss,
         pPositionInfoRtn->DateCloseProfitLoss, pPositionInfoRtn->Premium, pPositionInfoRtn->ProfitLoss,
         pPositionInfoRtn->Margin, pPositionInfoRtn->Speculator, pPositionInfoRtn->ClientID,
         pPositionInfoRtn->PreSettlementPrice, pPositionInfoRtn->InstrumentType,
         pPositionInfoRtn->YesterdayTradingAmount, pPositionInfoRtn->OptionValue);
  if (pErrorInfo) {
    printf("RspErrorField: RequestID:%ld SessionID:%ld AccountID:%s ErrorID:%d ErrorMsg:%s "
           "X1OrderID:%ld LocalOrderID:%ld InstrumentID:%s\n",
           pErrorInfo->RequestID, pErrorInfo->SessionID,
           pErrorInfo->AccountID, pErrorInfo->ErrorID, pErrorInfo->ErrorMsg,
           pErrorInfo->X1OrderID, pErrorInfo->LocalOrderID, pErrorInfo->InstrumentID);
  }
}


void X1Trader::ReqInsertOrder(Order *order) {
  int order_id = order_pool_.add(order);
  CX1FtdcInsertOrderField field;
  memset(&field, 0, sizeof(field));
  snprintf(field.AccountID, sizeof(field.AccountID), "%s", user_id_.c_str());
  field.LocalOrderID = order_id;
  snprintf(field.InstrumentID, sizeof(field.InstrumentID), "%s", order->instrument_id);
  field.InsertPrice = order->limit_price;
  field.OrderAmount = order->volume;
  if (order->direction == kDirectionBuy) {
    field.BuySellType = X1_FTDC_SPD_BUY;
  } else {
    field.BuySellType = X1_FTDC_SPD_SELL;
  }
  switch(order->offset) {
    case kOffsetOpen:
      field.OpenCloseType = X1_FTDC_SPD_OPEN;
      break;
    case kOffsetClose:
      field.OpenCloseType = X1_FTDC_SPD_CLOSE;
      break;
    case kOffsetCloseToday:
      field.OpenCloseType = X1_FTDC_SPD_CLOSETODAY;
      break;
    default:
      field.OpenCloseType = X1_FTDC_SPD_CLOSE;
      break;
  }
  field.Speculator = X1_FTDC_SPD_SPECULATOR;
  field.InsertType = X1_FTDC_BASIC_ORDER;
  switch(order->order_type) {
    case kOrderTypeLimit:
      field.OrderType = X1_FTDC_LIMITORDER;
      break;
    case kOrderTypeMarket:
      field.OrderType = X1_FTDC_MKORDER;
      break;
    default:
      field.OrderType = X1_FTDC_LIMITORDER;
      break;
  }
  switch(order->time_in_force) {
    case kTimeInForceDay:
      field.OrderProperty = X1_FTDC_SP_NON;
      break;
    case kTimeInForceFAK:
      field.OrderProperty = X1_FTDC_SP_FAK;
      break;
    case kTimeInForceFOK:
      field.OrderProperty = X1_FTDC_SP_FOK;
      break;
    default:
      field.OrderProperty = X1_FTDC_SP_NON;
      break;
  }
  field.InstrumentType = X1FTDC_INSTRUMENT_TYPE_COMM;
  field.MinMatchAmount = 0;
  field.RequestID = ++request_id_;
  int ret = api_->ReqInsertOrder(&field);
  if (ret != 0) {
    printf("Failed to insert order! %d\n", ret);
  } else {
    printf("Insert Order successfully!\n");
  }
}

void X1Trader::ReqCancelOrder(Order *order) {
  // Order *orig_order = order_pool_.get(order->sys_order_id);
  CX1FtdcCancelOrderField field;
  memset(&field, 0, sizeof(field));
  // snprintf(field.X1OrderID, sizeof(field.X1OrderID), "%s", order->sys_order_id);
  field.LocalOrderID = order->order_id;
  field.RequestID = ++request_id_;
  int ret = api_->ReqCancelOrder(&field);
  if (ret != 0) {
    printf("Failed to cancel order! %d\n", ret);
  } else {
    printf("Cancel Order successfully!\n");
  }
}

void X1Trader::ReqCancelOrder(int order_id, long x1order_id, const char *instrument_id) {
  CX1FtdcCancelOrderField field;
  memset(&field, 0, sizeof(field));
  snprintf(field.InstrumentID, sizeof(field.InstrumentID), "%s", instrument_id);
  field.LocalOrderID = order_id;
  // field.X1OrderID = x1order_id;
  field.RequestID = ++request_id_;
  int ret = api_->ReqCancelOrder(&field);
  if (ret != 0) {
    printf("Failed to cancel order! %d\n", ret);
  } else {
    printf("Cancel Order successfully!\n");
  }
}

void X1Trader::ReqCancelAllOrder() {
  // TODO
}

void X1Trader::ReqQryPosition() {
  CX1FtdcQryPositionField field;
  memset(&field, 0, sizeof(field));
  snprintf(field.AccountID, sizeof(field.AccountID), "%s", user_id_.c_str());
  field.RequestID = ++request_id_;
  int ret = api_->ReqQryPosition(&field);
  if (ret != 0) {
    printf("Failed to qry position! %d\n", ret);
  } else {
    printf("ReqQryPosition\n");
  }
}

void X1Trader::Logout() {
  CX1FtdcReqUserLogoutField logout;
  memset(&logout, 0, sizeof(0));
  logout.RequestID = ++request_id_;
  snprintf(logout.AccountID, sizeof(logout.AccountID), "%s", user_id_.c_str());
  api_->ReqUserLogout(&logout);
  printf("Logout...\n");
}

