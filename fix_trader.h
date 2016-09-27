
#ifndef __FIX_TRADER_H__
#define __FIX_TRADER_H__


#include <quickfix/Application.h>
#include <quickfix/MessageCracker.h>
#include <quickfix/Values.h>
#include <quickfix/Mutex.h>

#include <quickfix/fix40/NewOrderSingle.h>
#include <quickfix/fix40/ExecutionReport.h>
#include <quickfix/fix40/OrderCancelRequest.h>
#include <quickfix/fix40/OrderCancelReject.h>
#include <quickfix/fix40/OrderCancelReplaceRequest.h>

#include <quickfix/fix41/NewOrderSingle.h>
#include <quickfix/fix41/ExecutionReport.h>
#include <quickfix/fix41/OrderCancelRequest.h>
#include <quickfix/fix41/OrderCancelReject.h>
#include <quickfix/fix41/OrderCancelReplaceRequest.h>

#include <quickfix/fix42/NewOrderSingle.h>
#include <quickfix/fix42/ExecutionReport.h>
#include <quickfix/fix42/OrderCancelRequest.h>
#include <quickfix/fix42/OrderCancelReject.h>
#include <quickfix/fix42/OrderCancelReplaceRequest.h>

#include <quickfix/fix43/NewOrderSingle.h>
#include <quickfix/fix43/ExecutionReport.h>
#include <quickfix/fix43/OrderCancelRequest.h>
#include <quickfix/fix43/OrderCancelReject.h>
#include <quickfix/fix43/OrderCancelReplaceRequest.h>
#include <quickfix/fix43/MarketDataRequest.h>

#include <quickfix/fix44/NewOrderSingle.h>
#include <quickfix/fix44/ExecutionReport.h>
#include <quickfix/fix44/OrderCancelRequest.h>
#include <quickfix/fix44/OrderCancelReject.h>
#include <quickfix/fix44/OrderCancelReplaceRequest.h>
#include <quickfix/fix44/MarketDataRequest.h>

#include <quickfix/fix50/NewOrderSingle.h>
#include <quickfix/fix50/ExecutionReport.h>
#include <quickfix/fix50/OrderCancelRequest.h>
#include <quickfix/fix50/OrderCancelReject.h>
#include <quickfix/fix50/OrderCancelReplaceRequest.h>
#include <quickfix/fix50/MarketDataRequest.h>

#include <queue>

#include "order.h"

#ifdef CME_FIX_40
#define CME_FIX_NAMESPACE FIX40
#elif defined CME_FIX_41
#define CME_FIX_NAMESPACE FIX41
#elif defined CME_FIX_42
#define CME_FIX_NAMESPACE FIX42
#elif defined CME_FIX_43
#define CME_FIX_NAMESPACE FIX43
#elif defined CME_FIX_44
#define CME_FIX_NAMESPACE FIX44
#elif defined CME_FIX_50
#define CME_FIX_NAMESPACE FIX50
#else
#define CME_FIX_NAMESPACE FIX42
#endif

class FixTrader : public FIX::Application, public FIX::MessageCracker {
 public:
  void run();
  void ReqUserLogon(FIX::Message& message);
  void ReqUserLogout(FIX::Message& message);
  void ReqOrderInsert(Order *order);
  void ReqOrderAction(Order *order);

 private:
  void onCreate(const FIX::SessionID& sessionID);
  void onLogon(const FIX::SessionID& sessionID);
  void onLogout(const FIX::SessionID& sessionID);
  void toAdmin(FIX::Message& message, const FIX::SessionID& sessionID);
  void toApp(FIX::Message& message, const FIX::SessionID& sessionID)
    throw(FIX::DoNotSend);
  void fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID)
    throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, 
          FIX::IncorrectTagValue, FIX::RejectLogon);
  void fromApp(const FIX::Message& message, const FIX::SessionID& sessionID)
    throw(FIX::FieldNotFound, FIX::IncorrectDataFormat,
          FIX::IncorrectTagValue, FIX::UnsupportedMessageType);

  void onMessage(const CME_FIX_NAMESPACE::ExecutionReport& report, 
                 const FIX::SessionID& sessionID);
  void onMessage(const CME_FIX_NAMESPACE::OrderCancelReject& report,
                 const FIX::SessionID& sessionID);

  // CME_FIX_NAMESPACE::NewOrderSingle queryNewOrderSingle(Order &order);
  // CME_FIX_NAMESPACE::OrderCancelRequest queryOrderCancelRequest(Order &order);
  // CME_FIX_NAMESPACE::OrderCancelReplaceRequest queryCancelReplaceRequest();
  /// 
  void queryHeader(FIX::Header& header);

  virtual void OnFrontConnected();
  virtual void OnFrontDisconnected(int nReason);
  // virtual void OnRspOrderInsert();
  // virtual void OnRtnOrder();
  // virtual void OnRtnTrade();
  
  FIX::SessionID session_id_;
  OrderPool order_pool_;
};

#endif  // __FIX_TRADER_H__
