
#include "fix_trader.h"
#include "utils.h"

#include <quickfix/Session.h>

#include <string>
#include <iostream>

using namespace std;

char *gConfigFileName = const_cast<char*>("./test.cfg");

void FixTrader::onCreate(const FIX::SessionID& sessionID) {
  cout << "Session created : " << session_id_ << endl;
  session_id_ = sessionID;
}

void FixTrader::onLogon(const FIX::SessionID& sessionID) {
  cout << endl << "Logon - " << sessionID << endl;
  OnFrontConnected();
}

void FixTrader::onLogout(const FIX::SessionID& sessionID) {
  cout << endl << "Logout - " << sessionID << endl;
  OnFrontDisconnected(0);
}

void FixTrader::fromApp(const FIX::Message& message, 
                        const FIX::SessionID& sessionID) 
  throw (FIX::FieldNotFound, FIX::IncorrectDataFormat,
         FIX::IncorrectTagValue, FIX::UnsupportedMessageType) {
  // FIX::MsgType msg_type;
  // FIX::PosReqType pos_req_type;
  // message.getHeader().getField(msg_type);
  // string message_string = message.toString();
  // if (msg_type == "8") {
  //   cout << "Order Response is received:\n" << message_string << endl;
  // }
  // if (msg_type == "UAP") {
  //   cout << "Capital Query Response is received\n" << message_string << endl;
  // }
  
  /// new version quickfix demo
  /// what crack does?
  crack(message, sessionID);
  cout << "FROM APP XML: " << message.toXML() << endl;
  // cout << "FROM APP: " << message << endl;
}

void FixTrader::toApp(FIX::Message& message, const FIX::SessionID& sessionID)
  throw (FIX::DoNotSend) {
    /// old version quickfix demo
    // FIX::MsgType msg_type;
    // message.getHeader().getField(msg_type);
    // string message_string = message.toString();
    // if (msg_type == "D") {
    //   cout << "Send Order Message:\n" << message_string << endl;
    // }
    // if (msg_type == "F") {
    //   cout << "Send Cancel Message:\n" << message_string << endl;
    // }
    // if (msg_type == "UAN") {
    //   cout << "Send Capital Query Message:\n" << message_string << endl;
    // }
    // if (msg_type == "H") {
    //   cout << "Send Order Query Message:\n" << message_string << endl;
    // }

    /// new version quickfix demo
    try {
      FIX::PossDupFlag possDupFlag;
      message.getHeader().getField(possDupFlag);
      if (possDupFlag) {
        throw FIX::DoNotSend();
      }
    } catch (FIX::FieldNotFound&) {}

    cout << "TO APP XML: " << message.toXML() << endl;
    // cout << "TO APP: " << message << endl;
}

void FixTrader::fromAdmin(const FIX::Message& message,
                          const FIX::SessionID& sessionID) 
    throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, 
          FIX::IncorrectTagValue, FIX::RejectLogon) {
  crack(message, sessionID);
  cout << "FROM ADMIN XML: " << message.toXML() << endl;
  // cout << "FROM ADMIN: " << message << endl;
}

void FixTrader::toAdmin(FIX::Message& message, const FIX::SessionID&) {
  FIX::MsgType msg_type;
  message.getHeader().getField(msg_type);
  if (msg_type == FIX::MsgType_Logon) {
    FillTagsToAdmin(message);
    ReqUserLogon(message);
  } else if (msg_type == FIX::MsgType_Heartbeat) {
    FillTagsToAdmin(message);
  } else if (msg_type == FIX::MsgType_ResendRequest) {
    FillTagsToAdmin(message);
  }
  else if (msg_type == FIX::MsgType_Logout) {
    ReqUserLogout(message);
  }

  cout << "TO ADMIN XML: " << message.toXML() << endl;
  // cout << "TO ADMIN: " << message << endl;
}

void FixTrader::onMessage(const CME_FIX_NAMESPACE::ExecutionReport& report, 
                          const FIX::SessionID& sessionID) {
  FIX::ExecType exec_type;
  report.getField(exec_type);
  cout << "onMessage<ExecutionReport>- " << exec_type << endl;
  switch (exec_type) {
    case FIX::ExecType_NEW: {
      OrderAck order_ack = ToOrderAck(report);
      OnRspOrderInsert(&order_ack);  // OnRspOrderInsert
    }
    break;
    case  FIX::ExecType_PARTIAL_FILL: {
      Deal deal = ToDeal(report);
      OnRtnTrade(&deal);   // OnRtnTrade
    }
    break;
    case FIX::ExecType_FILL: {
      Deal deal = ToDeal(report);
      OrderAck order_ack = ToOrderAck(report);
      OnRtnTrade(&deal);
      OnRtnOrder(&order_ack);
      // OnRtnOrder if necessary
    }
    break;
    case FIX::ExecType_CANCELED: {
      OrderAck order_ack = ToOrderAck(report);
      OnRtnOrder(&order_ack);  // OnRtnOrder
    }
    break;
    case FIX::ExecType_REPLACE:
      // OnRtnOrder of modified
      break;
    case FIX::ExecType_REJECTED:
      // OnError()
      break;
    default:
      break;
  }  // switch ord_status
}

void FixTrader::onMessage(const CME_FIX_NAMESPACE::OrderCancelReject& report,
                          const FIX::SessionID& sessionID) {
  FIX::OrdStatus ord_status;
  report.getField(ord_status);
  cout << "onMessage<OrderCancelReject>- " << ord_status << endl;
  // OnError();
  // OnRtnOrder();
}

void FixTrader::OnFrontConnected() {
  cout << "Front Connected" << endl;
}

void FixTrader::OnFrontDisconnected(int nReason) {
  cout << "Front Disconnected" << nReason << endl;
}

void FixTrader::ReqUserLogon(FIX::Message& message) {
    message.getHeader().setField(FIX::SenderSubID("Anything"));
    message.getHeader().setField(FIX::SenderLocationID("HK"));
    message.getHeader().setField(FIX::TargetSubID("G"));

    //char sz_password[32] = "4PVSK";
    char sz_password[32] = "JY8FR";
    char sz_reset_seq_num_flag[5] = "N";

    char raw_data[1024] = {0};
    char raw_data_len[16] = {0};
    char system_name[32] = "CME_CFI";
    char system_version[32] = "1.0";
    char system_vendor[32] = "Cash Algo";
    snprintf(raw_data, sizeof(raw_data), "%s", sz_password);
    snprintf(raw_data_len, sizeof(raw_data_len), "%d", strlen(raw_data));
    // int raw_data_len = strlen(raw_data);
    message.setField(FIX::FIELD::RawData, raw_data);
    message.setField(FIX::FIELD::RawDataLength, raw_data_len);
    message.setField(FIX::FIELD::ResetSeqNumFlag, sz_reset_seq_num_flag);
    // message.setField(FIX::FIELD::EncryptMethod, "0");
    message.setField(FIX::FIELD::EncryptMethod, "0");  // string or int? type-safety?
    // message.setField(1603, system_name);  // customed fields
    // message.setField(1604, system_version);
    // message.setField(1605, system_vendor);
    string message_string = message.toString();
    cout << "Send Logon Message:\n" << message_string << endl;
}

void FixTrader::ReqUserLogout(FIX::Message& message) {
    string message_string = message.toString();
    cout << "Send Logout Message:\n" << message_string << endl;
}

void FixTrader::ReqOrderInsert(Order *order) {
  order->order_id = order_pool_.add(order);
  char cl_order_id_str[16];
  snprintf(cl_order_id_str, sizeof(cl_order_id_str), "%d", order->order_id);
  FIX::OrdType order_type = FIX::OrdType_LIMIT;
  FIX::HandlInst handl_inst('1');
  FIX::ClOrdID cl_order_id(cl_order_id_str);
  FIX::Symbol symbol(order->instrument_id);
  FIX::Side side;
  if (order->direction == kDirectionBuy) {
    side = FIX::Side_BUY;
  } else {
    side = FIX::Side_SELL;
  }
  FIX::TransactTime transact_time(time_now());
  CME_FIX_NAMESPACE::NewOrderSingle new_order(cl_order_id, handl_inst, symbol,
                                            side, transact_time, order_type);
  FIX::Price price(order->limit_price);
  FIX::Account account(order->account);
  FIX::OrderQty order_qty(order->volume);
  FIX::TimeInForce time_in_force = FIX::TimeInForce_DAY;
  new_order.set(price);
  new_order.set(account);
  new_order.set(order_qty);
  new_order.set(time_in_force);

  // 1028-ManualOrderIndicator : Y=manual N=antomated
  new_order.setField(1028, "N");

  // SecurityDesc : Future Example: GEZ8
  //                Option Example: CEZ9 C9375
  // Is SecurityDesc a type? 
  // FIX::SecurityDesc security_desc("GEZ8");
  // new_order.set(security_desc);
  new_order.setField(FIX::FIELD::SecurityDesc, "GEZ8");

  // SecurityType : FUT=Future
  //                OPT=Option
  // FIX::SecurityType security_type("FUT");
  // new_order.set(security_type);
  new_order.setField(FIX::FIELD::SecurityType, "FUT");

  // CustomerOrFirm : 0=Customer
  //                  1=Firm
  // FIX::CustomerOrFirm customer_or_firm = FIX::CustomerOrFirm_FIRM;
  // new_order.set(customer_or_firm);
  new_order.setField(FIX::FIELD::CustomerOrFirm, "1");

  // 9702-CtiCode : 1=CTI1 2=CTI2 3=CTI3 4=CTI4
  new_order.setField(9702, "2");

  // test the type of getValue()
  double p = price.getValue();
  string a = account.getValue();
  int v = order_qty.getValue();
  char h = handl_inst.getValue();
  // string t = transact_time.getValue();

  cout << "ReqOrderInsert:%d" << order->order_id << endl;
  FIX::Session::sendToTarget(new_order);
}

void FixTrader::ReqOrderAction(Order *order) {
  order->order_id = order_pool_.add(order);
  Order *orig_order = order_pool_.get(order->orig_order_id);
  char cl_order_id_str[16], orig_order_id_str[16];
  snprintf(cl_order_id_str, sizeof(cl_order_id_str), "%d", order->order_id);
  snprintf(orig_order_id_str, sizeof(orig_order_id_str), "%d",
           order->orig_order_id);
  FIX::ClOrdID cl_order_id(cl_order_id_str);
  FIX::OrigClOrdID orig_cl_order_id(orig_order_id_str);
  FIX::Side side;
  if (orig_order->direction == kDirectionBuy) {
    side = FIX::Side_BUY;
  } else {
    side = FIX::Side_SELL;
  }
  FIX::Symbol symbol(orig_order->instrument_id);
  FIX::TransactTime transact_time(time_now());

  CME_FIX_NAMESPACE::OrderCancelRequest cancel_order(orig_cl_order_id,
                                                     cl_order_id,
                                                     symbol,
                                                     side,
                                                     transact_time);

  FIX::Account account(order->account);
  FIX::OrderID order_id(order->sys_order_id);
  cancel_order.set(account);
  cancel_order.set(order_id);

  // 1028-ManualOrderIndicator : Y=manual N=antomated
  cancel_order.setField(1028, "N");

  // SecurityDesc : Future Example: GEZ8
  //                Option Example: CEZ9 C9375
  // Is SecurityDesc a type? 
  // FIX::SecurityDesc security_desc("GEZ8");
  // new_order.set(security_desc);
  cancel_order.setField(FIX::FIELD::SecurityDesc, "GEZ8");

  // SecurityType : FUT=Future
  //                OPT=Option
  // FIX::SecurityType security_type("FUT");
  // new_order.set(security_type);
  cancel_order.setField(FIX::FIELD::SecurityType, "FUT");

  // 9717-CorrelationClOrdID
  // This tag should contain the same value as the tag-11 ClOrdID 
  // of the original New Order message and is used to correlate iLink
  // messages associated with a single order chain
  // ClOrdID or OrigClOrdID ?
  // cancel_order.setField(9717, cl_order_id_str);
  cancel_order.setField(9717, orig_order_id_str);

  cout << "ReqOrderAction:" << orig_order_id_str << endl;
  FIX::Session::sendToTarget(cancel_order);
}

void FixTrader::run() {
  // TODO
  cout << "run" << endl;
}

void FixTrader::OnRspOrderInsert(OrderAck *order_ack) {
  cout << "OnRspOrderInsert" << endl;
  // TODO
}

void FixTrader::OnRtnOrder(OrderAck *order_ack) {
  cout << "OnRtnOrder" << endl;
  // TODO
}

void FixTrader::OnRtnTrade(Deal *deal) {
  cout << "OnRtnTrade" << endl;
  // TODO
}

OrderAck FixTrader::ToOrderAck(const CME_FIX_NAMESPACE::ExecutionReport& report) {
  OrderAck order_ack;
  FIX::ExecType exec_type;
  report.getField(exec_type);
  order_ack.order_status = exec_type;
  FIX::Account account;
  report.getField(account);
  string acc = account.getValue();
  // strcpy(order_ack.account, account);
  return order_ack;
}

Deal FixTrader::ToDeal(const CME_FIX_NAMESPACE::ExecutionReport& report) {
  Deal deal;

  return deal;
}

