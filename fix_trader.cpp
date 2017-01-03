
#include "fix_trader.h"
#include "utils.h"

#include <quickfix/Session.h>

#include <string>
#include <iostream>

using namespace std;

char *gConfigFileName = const_cast<char*>("./test.cfg");

FixTrader::FixTrader() : last_msg_seq_num_(0) {
  log_file_.open("./fix.log", std::fstream::out | std::fstream::app);
  // audit_trail_.Init("AuditTrail.csv");
  // seq_serial_.SetPrefix("3T7004N");  // use account as prefix
  // seq_serial_.Init();
  // order_pool_.reset_sequence(seq_serial_.GetCurOrderID());
}

void FixTrader::Init(const string &config_file_name) {
  settings_ = new FIX::SessionSettings(config_file_name);
  store_factory_ = new FIX::FileStoreFactory(*settings_);
  log_factory_ = new FIX::ScreenLogFactory(*settings_);
  initiator_ = new FIX::SocketInitiator(*this, *store_factory_,
        *settings_, *log_factory_);
  initiator_->start();
}

void FixTrader::Logout() {
  initiator_->stop();
}

void FixTrader::onCreate(const FIX::SessionID& sessionID) {
  cout << "Session created : " << sessionID << endl;
  session_id_ = sessionID;
  string str_sender_comp_id = sessionID.getSenderCompID().getValue();
  string str_target_comp_id = sessionID.getTargetCompID().getValue();
  string acc_session_id = str_sender_comp_id.substr(0, 3);
  string firm_id = str_sender_comp_id.substr(3, 3);
  snprintf(acc_session_id_, sizeof(acc_session_id_), "%s", acc_session_id.c_str());
  snprintf(firm_id_, sizeof(firm_id_), "%s", firm_id.c_str());

  const FIX::Dictionary& dict = settings_->get(sessionID);
  string password = dict.getString("PASSWD");
  string target_sub_id = dict.getString("TARGETSUBID");
  string sender_sub_id = dict.getString("SENDERSUBID");
  string sender_loc_id = dict.getString("SENDERLOCATIONID");
  string self_match_prev_id = dict.getString("SELFMATCHPREVENTIONID");
  snprintf(password_, sizeof(password_), "%s", password.c_str());
  snprintf(target_sub_id_, sizeof(target_sub_id_), "%s", target_sub_id.c_str());
  snprintf(sender_sub_id_, sizeof(sender_sub_id_), "%s", sender_sub_id.c_str());
  snprintf(sender_loc_id_, sizeof(sender_loc_id_), "%s", sender_loc_id.c_str());
  snprintf(self_match_prev_id_, sizeof(self_match_prev_id_), "%s",
           self_match_prev_id.c_str());

  int date = date_now();
  string ec_id = dict.getString("ECID");
  string market_code = dict.getString("MARKETCODE");
  string platform_code = dict.getString("PLATFORMCODE");
  string session_id = str_sender_comp_id;
  transform(session_id.begin(), session_id.end(), 
            session_id.begin(), ::tolower);
  string client_name = dict.getString("CLIENTNAME");
  string add_detail = dict.getString("ADDITIONALOPTIONALDETAIL");

  char audit_file_name[256];
  snprintf(audit_file_name, sizeof(audit_file_name), 
           "%d_%s_%s_%s_%s_%s_%s_audittrail.globex.csv",
           date, ec_id.c_str(), market_code.c_str(), platform_code.c_str(),
           session_id.c_str(), client_name.c_str(), add_detail.c_str());

  audit_trail_.Init(audit_file_name);
  seq_serial_.SetPrefix(str_sender_comp_id.c_str());
  seq_serial_.Init();
  order_pool_.reset_sequence(seq_serial_.GetCurOrderID());
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
  throw () {
  // throw (FIX::FieldNotFound, FIX::IncorrectDataFormat,
  //        FIX::IncorrectTagValue, FIX::UnsupportedMessageType) {
  log_file_ << "[" << time_now() << "]FROM APP XML: " << message.toXML() << endl;

  FIX::MsgType msg_type;
  FIX::MsgSeqNum last_msg_seq_num;
  message.getHeader().getField(last_msg_seq_num);
  last_msg_seq_num_ = last_msg_seq_num.getValue();
  // FIX::PosReqType pos_req_type;
  message.getHeader().getField(msg_type);
  if (msg_type == FIX::MsgType_XMLnonFIX) {
    onXmlNonFix(message, sessionID);
    return;
  } else if (msg_type == FIX::MsgType_OrderMassActionReport) {
    onMassActionReport(message, sessionID);
    return;
  }
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
  // cout << "FROM APP: " << message << endl;
}

void FixTrader::toApp(FIX::Message& message, const FIX::SessionID& sessionID)
  throw () {
  // throw (FIX::DoNotSend) {
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

    ////////////////////////////////
    /// new version quickfix demo //
    ////////////////////////////////
    // static int pos = 0;
    try {
      FIX::PossDupFlag possDupFlag;
      message.getHeader().getField(possDupFlag);
      if (possDupFlag) {
          throw FIX::DoNotSend();
      }
    } catch (FIX::FieldNotFound&) {}

    FillHeader(message);
    log_file_ << "[" << time_now() << "]TO APP XML: " << message.toXML() << endl;
    // cout << "TO APP: " << message << endl;
}

void FixTrader::fromAdmin(const FIX::Message& message,
                          const FIX::SessionID& sessionID)
  throw() {

    // throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, 
    //       FIX::IncorrectTagValue, FIX::RejectLogon) {
  FIX::MsgType msg_type;
  message.getHeader().getField(msg_type);
  FIX::MsgSeqNum last_msg_seq_num;
  message.getHeader().getField(last_msg_seq_num);
  last_msg_seq_num_ = last_msg_seq_num.getValue();
  if (msg_type == "5") {
    // FIX::LastMsgSeqNumProcessed last_msg_seq_num;
    // message.getHeader().getField(last_msg_seq_num);
    // int start_seq_num = last_msg_seq_num.getValue() + 1;
    // // last_msg_seq_num_.setValue(start_seq_num);
    // last_msg_seq_num_ = start_seq_num;
    // cout << start_seq_num << "::" << last_msg_seq_num_;
  }
  log_file_ << "[" << time_now() << "]FROM ADMIN XML: " << message.toXML() << endl;
  crack(message, sessionID);
  // cout << "FROM ADMIN: " << message << endl;
}

void FixTrader::toAdmin(FIX::Message& message, const FIX::SessionID&) {
  FIX::MsgType msg_type;
  message.getHeader().getField(msg_type);
  FillHeader(message);
  if (msg_type == FIX::MsgType_Logon) {
    ReqUserLogon(message);
  } else if (msg_type == FIX::MsgType_Heartbeat) {
    SendHeartbeat(message);
  } else if (msg_type == FIX::MsgType_ResendRequest) {
    ReqUserResend(message);
  } else if (msg_type == FIX::MsgType_Reject) {
    cout << "Reject in toAdmin ?" << endl;
  } else if (msg_type == FIX::MsgType_Logout) {
    ReqUserLogout(message);
  } else if (msg_type == FIX::MsgType_SequenceReset) {
    SendResetSequence(message);
  }

  log_file_ << "[" << time_now() << "]TO ADMIN XML: " << message.toXML() << endl;
  // cout << "TO ADMIN: " << message << endl;
}

void FixTrader::onMessage(const CME_FIX_NAMESPACE::Logon& logon,
                          const FIX::SessionID& sessionID) {
  cout << "OnMessage Logon: " << logon.toXML() << endl;
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
  PrintExecutionReport(report);
}

void FixTrader::onMessage(const CME_FIX_NAMESPACE::OrderCancelReject& report,
                          const FIX::SessionID& sessionID) {
  FIX::OrdStatus ord_status;
  report.getField(ord_status);
  cout << "onMessage<OrderCancelReject>- " << ord_status << endl;
  // OnError();
  // OnRtnOrder();
}

void FixTrader::onMessage(const CME_FIX_NAMESPACE::TestRequest& request,
                          const FIX::SessionID& sessionID) {
  FIX::TestReqID test_req_id;
  request.getField(test_req_id);
  cout << "Test Request ID received: " << test_req_id << endl;

  // CME_FIX_NAMESPACE::Heartbeat heartbeat;
  // heartbeat.set(test_req_id);
  // cout << "Send heartbeat in response to Test Request:" << test_req_id << endl;
  // FIX::Session::sendToTarget(heartbeat, session_id_);
}

void FixTrader::onMessage(const CME_FIX_NAMESPACE::SequenceReset& reset,
                          const FIX::SessionID& sessionID) {
  FIX::NewSeqNo new_seq_no;
  reset.getField(new_seq_no);
  FIX::GapFillFlag gap_fill_flag;
  reset.getField(gap_fill_flag);
  cout << "New Sequence Number received: " << new_seq_no
       << " GapFillFlag: " << gap_fill_flag << endl;

  // CME_FIX_NAMESPACE::Heartbeat heartbeat;
  // heartbeat.set(test_req_id);
  // cout << "Send heartbeat in response to Test Request:" << test_req_id << endl;
  // FIX::Session::sendToTarget(heartbeat, session_id_);
}

void FixTrader::onMessage(const CME_FIX_NAMESPACE::Heartbeat& heartbeat,
                          const FIX::SessionID& sessionID) {
  cout << "Heartbeat received: " << heartbeat.toXML() << endl;
}

void FixTrader::onMessage(const CME_FIX_NAMESPACE::Reject& reject,
                          const FIX::SessionID& sessionID) {
  cout << "Reject received: " << reject.toXML() << endl;
}

void FixTrader::onMessage(const CME_FIX_NAMESPACE::ResendRequest& request,
                          const FIX::SessionID& sessionID) {
  // FIX::BeginSeqNo begin_seq_no;
  // FIX::EndSeqNo end_seq_no;
  // request.getField(begin_seq_no);
  // request.getField(end_seq_no);
  // int begin_no = begin_seq_no.getValue();
  // int end_no = end_seq_no.getValue();


  cout << "Resend Request received: " << request.toXML() << endl;
}

void FixTrader::onMassActionReport(const FIX::Message& message,
                                   const FIX::SessionID& sessionID) {
  FIX::ClOrdID cl_order_id;
  message.getField(cl_order_id);
  string report_id = message.getField(1369);
  string action_type = message.getField(1373);
  string action_scope = message.getField(1374);
  string action_response = message.getField(1375);
  string affected_orders = message.getField(533);
  string last_fragment = message.getField(893);
  cout << "OnMassActionReport: ReportID-" << report_id
       << " ActionType-" << action_type 
       << " ActionScope-" << action_scope
       << " ActionResponse-" << action_response
       << " TotalAffectedOrders-" << affected_orders
       << " LastFragment-" << last_fragment
       << endl;
  int total_affected = atoi(affected_orders.c_str());
  if (total_affected > 0) {
    int no_affected = atoi(message.getField(534).c_str());
    FIX::OrigClOrdID orig_cl_ord_id;
    FIX::CxlQty cxl_qty;
    FIX::SecurityDesc security_desc;
    message.getField(orig_cl_ord_id);
    message.getField(cxl_qty);
    message.getField(security_desc);
    string affected_ord_id = message.getField(535);
    cout << "Cancelled: NoAffectedOrders-" << no_affected
         << " OrigClOrdID-" << orig_cl_ord_id
         << " CxlQty-" << cxl_qty
         << " SecurityDesc-" << security_desc
         << " AffectedOrderID-" << affected_ord_id
         << endl;
  }
}

void FixTrader::onXmlNonFix(const FIX::Message& message,
                            const FIX::SessionID& sessionID) {
  string xml_data = message.getHeader().getField(213);
  // strip header and tailor of <RTRF> </RTRF>
  string message_data = xml_data.substr(6, xml_data.size() - 6 - 7);
  for (size_t i = 0; i < message_data.size(); i++) {
    if (message_data[i] == '\002') {
      message_data[i] = '\001';
    }
  }
  cout << "Recover message data: " << message_data << endl;
  FIX::Message report(message_data, false);
  FIX::MsgType msg_type;
  report.getHeader().getField(msg_type);
  if (msg_type == FIX::MsgType_ExecutionReport) {
    CME_FIX_NAMESPACE::ExecutionReport exe_report(report);
    onMessage(exe_report, sessionID);
  } else if (msg_type == FIX::MsgType_OrderMassActionReport) {
    onMassActionReport(report, sessionID);
  }
  cout << "onXmlNonFix DONE" << endl;
}

void FixTrader::OnFrontConnected() {
  cout << "Front Connected" << endl;
}

void FixTrader::OnFrontDisconnected(int nReason) {
  cout << "Front Disconnected" << nReason << endl;
}

void FixTrader::ReqUserLogon(FIX::Message& message) {
  cout << "ReqUserLogon" << endl;
  //char sz_password[32] = "4PVSK";
  // int last_msg_seq_num = last_msg_seq_num_.getValue();
  // cout << "last msg seq num: " << last_msg_seq_num << endl;
  // if (last_msg_seq_num_ != 0) {
  //   FIX::MsgSeqNum msg_seq_num(last_msg_seq_num_);
  //   message.getHeader().setField(msg_seq_num);
  // }
  // char sz_password[32] = "JY8FR";
  // char sz_password[32] = "lfick";
  // char sz_reset_seq_num_flag[5] = "N";

  // char raw_data[1024] = {0};
  // char raw_data_len[16] = {0};
  char system_name[32] = "CME_CFI";
  char system_version[32] = "1.0";
  char system_vendor[32] = "Cash Algo";
  // snprintf(raw_data, sizeof(raw_data), "%s", sz_password);
  // snprintf(raw_data_len, sizeof(raw_data_len), "%d", strlen(raw_data));
  // int raw_data_len = strlen(raw_data);
  
  /*
  message.setField(FIX::FIELD::RawData, raw_data);
  message.setField(FIX::FIELD::RawDataLength, raw_data_len);
  message.setField(FIX::FIELD::ResetSeqNumFlag, sz_reset_seq_num_flag);
  // message.setField(FIX::FIELD::EncryptMethod, "0");
  message.setField(FIX::FIELD::EncryptMethod, "0");  // string or int? type-safety?
  not type-safety */ 
  message.setField(1603, system_name);  // customed fields
  message.setField(1604, system_version);
  message.setField(1605, system_vendor);
  FIX::RawData raw_data(password_);
  FIX::RawDataLength raw_data_len(strlen(password_));
  FIX::ResetSeqNumFlag reset_seq_num_flag(false);
  FIX::EncryptMethod encrypt_method(0);

  message.setField(raw_data);
  message.setField(raw_data_len);
  message.setField(reset_seq_num_flag);
  message.setField(encrypt_method);

  string message_string = message.toString();
  cout << "Send Logon Message:\n" << message_string << endl;
}

void FixTrader::SendHeartbeat(FIX::Message& message) {
  // FillHeader(message);
}

void FixTrader::SendResetSequence(FIX::Message& message) {
  // FillHeader(message);
  FIX::GapFillFlag gap_fill_flag(true);
  message.setField(gap_fill_flag);
}

void FixTrader::ReqUserResend(FIX::Message& message) {
  // FIX::BeginSeqNo begin_seq_no;
  // FIX::EndSeqNo end_seq_no;
  // message.getField(begin_seq_no);
  // message.getField(end_seq_no);
  // int begin_no = begin_seq_no.getValue();
  // int end_no = end_seq_no.getValue();
  // if (end_no == 2500) {
  //   return;
  // }
  // if (last_msg_seq_num_ - begin_no > 2500) {
  //   cout << "More than 2500 messages to resend!" << endl;
    // FIX::BeginSeqNo begin_seq_no(100);
    // FIX::EndSeqNo end_seq_no(2500);
    // message.setField(end_seq_no);
    // message.setField(begin_seq_no);
  // }
}

void FixTrader::ReqUserLogon() {
  CME_FIX_NAMESPACE::Logon logon;
  // cout << "Input sequence number manually? y/n ";
  // char cmd;
  // scanf("%c", &cmd);
  // getchar();
  // if (cmd == 'y' || cmd == 'Y') {
  //   getchar();
  //   FIX::TestReqID test_req_id(req_id);
  //   heartbeat.set(test_req_id);
  // } else {
  //   // No need to input test request ID
  // }

  FIX::Session::sendToTarget(logon, session_id_);
}

void FixTrader::ReqUserLogout(FIX::Message& message) {
    string message_string = message.toString();
    cout << "Send Logout Message:\n" << message_string << endl;
}

void FixTrader::SendHeartbeat() {
  CME_FIX_NAMESPACE::Heartbeat heartbeat;
  cout << "Input test request ID manually ? y/n";
  char cmd;
  scanf("%c", &cmd);
  getchar();
  if (cmd == 'y' || cmd == 'Y') {
    char req_id[16];
    cout << "Input test request ID:" << endl;
    scanf("%s", req_id);
    getchar();
    FIX::TestReqID test_req_id(req_id);
    heartbeat.set(test_req_id);
  } else {
    // No need to input test request ID
  }
  cout << "Send heartbeat manually:" << endl;
  FIX::Session::sendToTarget(heartbeat, session_id_);
}

void FixTrader::SendResendRequest() {
  int begin_no, end_no;
  printf("Input begin sequence no and end sequence no:");
  scanf("%d %d", &begin_no, &end_no);
  getchar();

  FIX::BeginSeqNo begin_seq_no(begin_no);
  FIX::EndSeqNo end_seq_no(end_no);
  CME_FIX_NAMESPACE::ResendRequest resend_request(begin_seq_no, end_seq_no);

  cout << "SendResendRequest:" << begin_no << ":" << end_no << endl;
  FIX::Session::sendToTarget(resend_request, session_id_);
}

void FixTrader::SendTestRequest() {
  cout << "Input test request ID: ";
  char req_id[16];
  scanf("%s", req_id);
  getchar();
  FIX::TestReqID test_req_id(req_id);
  CME_FIX_NAMESPACE::TestRequest test_request;
  test_request.set(test_req_id);

  cout << "SendTestRequest:" << test_req_id << endl;
  FIX::Session::sendToTarget(test_request, session_id_);
}

void FixTrader::SendResetSequence() {
  CME_FIX_NAMESPACE::SequenceReset sequence_reset;
  int seq_no;
  cout << "Input new sequence number: ";
  scanf("%d", &seq_no);
  getchar();
  FIX::NewSeqNo new_seq_no(seq_no);
  sequence_reset.set(new_seq_no);

  char flag;
  cout << "Input Gap Fill Flag: y/n ";
  scanf("%c", &flag);
  getchar();
  if (flag == 'y' || flag == 'Y') {
    FIX::GapFillFlag gap_fill_flag(true);
    sequence_reset.set(gap_fill_flag);
  } else {
    FIX::GapFillFlag gap_fill_flag(false);
    sequence_reset.set(gap_fill_flag);
  }

  cout << "SendSequenceReset:" << new_seq_no 
       << " GapFillFlag:" << flag << endl;
  FIX::Session::sendToTarget(sequence_reset, session_id_);
}

void FixTrader::ReqUserLogout() {
  CME_FIX_NAMESPACE::Logout logout;
  FIX::Session::sendToTarget(logout, session_id_);
}

void FixTrader::ReqOrderInsert(Order *order) {
  order->order_id = order_pool_.add(order);
  seq_serial_.DumpOrderID(order->order_id);
  string order_flow_id = seq_serial_.GenFlowIDStr(order->order_id);
  snprintf(order->order_flow_id, sizeof(order->order_flow_id), "%s", 
           order_flow_id.c_str());
  // string trans_id = seq_serial_.GenTransIDStr();
  char cl_order_id_str[16];
  snprintf(cl_order_id_str, sizeof(cl_order_id_str), "%d", order->order_id);
  FIX::HandlInst handl_inst('1');
  FIX::ClOrdID cl_order_id(cl_order_id_str);
  FIX::Symbol symbol(order->symbol);
  // FIX::Symbol symbol("GE");
  FIX::Side side;
  if (order->direction == kDirectionBuy) {
    side = FIX::Side_BUY;
  } else {
    side = FIX::Side_SELL;
  }
  FIX::OrdType order_type = FIX::OrdType_LIMIT;
  if (order->order_type == kOrderTypeLimit) {
    order_type = FIX::OrdType_LIMIT;
  } else if (order->order_type == kOrderTypeMarket) {
    order_type = FIX::OrdType_MARKET;
  } else if (order->order_type == kOrderTypeStopLimit) {
    order_type = FIX::OrdType_STOP_LIMIT;
  } else if (order->order_type == kOrderTypeStop) {
    order_type = FIX::OrdType_STOP;
  }
  string timestamp(time_now());
  FIX::TransactTime transact_time(timestamp.c_str());
  CME_FIX_NAMESPACE::NewOrderSingle new_order(cl_order_id, handl_inst, symbol,
                                            side, transact_time, order_type);
  FIX::Price price(order->limit_price);
  FIX::StopPx stop_px(order->stop_price);
  FIX::Account account(order->account);
  FIX::OrderQty order_qty(order->volume);
  FIX::TimeInForce time_in_force;
  if (order->time_in_force == kTimeInForceDay) {
    time_in_force = FIX::TimeInForce_DAY;
  } else if (order->time_in_force == kTimeInForceGTC) {
    time_in_force = FIX::TimeInForce_GOOD_TILL_CANCEL;
  } else if (order->time_in_force == kTimeInForceGTD) {
    time_in_force = FIX::TimeInForce_GOOD_TILL_DATE;
  } else if (order->time_in_force == kTimeInForceFAK) {
    time_in_force = FIX::TimeInForce_IMMEDIATE_OR_CANCEL;
  }

  if (order_type != FIX::OrdType_MARKET &&
      order_type != 'K') {
    new_order.set(price);
  }
  if (order_type == FIX::OrdType_STOP ||
      order_type == FIX::OrdType_STOP_LIMIT) {
    new_order.set(stop_px);
  }
  new_order.set(account);
  new_order.set(order_qty);
  new_order.set(time_in_force);

  // 1028-ManualOrderIndicator : Y=manual N=antomated
  new_order.setField(1028, "N");
  // new_order.setField(1028, "n");

  // SecurityDesc is the InstrumentID 
  FIX::SecurityDesc security_desc(order->instrument_id);
  new_order.set(security_desc);

  // SecurityType : FUT=Future
  //                OPT=Option
  // FIX::SecurityType security_type("FUT");
  // new_order.set(security_type);
  new_order.setField(FIX::FIELD::SecurityType, "FUT");

  // CustomerOrFirm : 0=Customer
  //                  1=Firm
  // FIX::CustomerOrFirm customer_or_firm = FIX::CustomerOrFirm_FIRM;
  // new_order.set(customer_or_firm);
  FIX::CustomerOrFirm customer_or_firm(1);
  new_order.set(customer_or_firm);
  // new_order.setField(FIX::FIELD::CustomerOrFirm, "1");

  // 9702-CtiCode : 1=CTI1 2=CTI2 3=CTI3 4=CTI4
  new_order.setField(9702, "2");

  new_order.setField(7928, self_match_prev_id_);  // SelfMatchPreventionID
  new_order.setField(8000, "O");  // SelfMatchPreventionInstruction

  // test the type of getValue()
  double p = price.getValue();
  string a = account.getValue();
  int v = order_qty.getValue();
  char h = handl_inst.getValue();
  // string t = transact_time.getValue();
  // 
  // FIX::SenderCompID sender_comp_id;
  // new_order.getHeader().getField(sender_comp_id);
  // string str_sender_comp_id = sender_comp_id.getValue();
  // string session_id = str_sender_comp_id.substr(0, 3);
  // string session_id = "3T7";
  // string firm_id = "004";
  // string sender_sub_id = "G";

  AuditLog audit_log;
  audit_log.WriteElement("sending_timestamps", timestamp);
  audit_log.WriteElement("message_direction", "TO CME");
  audit_log.WriteElement("operator_id", target_sub_id_);
  audit_log.WriteElement("self_match_prevention_id", self_match_prev_id_);
  audit_log.WriteElement("account_number", account.getValue());
  audit_log.WriteElement("session_id", acc_session_id_);
  audit_log.WriteElement("executing_firm_id", firm_id_);
  audit_log.WriteElement("manual_order_identifier", "N");  // field-1028
  audit_log.WriteElement("message_type", FIX::MsgType_NewOrderSingle);
  audit_log.WriteElement("customer_type_indicator", "2");  // CtiCode-9702
  audit_log.WriteElement("origin", customer_or_firm.getValue());
  // audit_log.WriteElement("message_link_id", trans_id);
  audit_log.WriteElement("order_flow_id", order_flow_id);
  audit_log.WriteElement("instrument_description", security_desc.getValue());
  // market segment id can be found in InstrumentDefinition in market data
  audit_log.WriteElement("market_segment_id", sender_sub_id_);
  audit_log.WriteElement("client_order_id", cl_order_id.getValue());
  audit_log.WriteElement("buy_sell_indicator", side.getValue());
  audit_log.WriteElement("quantity", (int)order_qty.getValue());
  audit_log.WriteElement("limit_price", price.getValue());
  audit_log.WriteElement("order_type", order_type.getValue());
  audit_log.WriteElement("order_qualifier", time_in_force);
  // audit_log.WriteElement("ifm_flag", "N");
  // audit_log.WriteElement("display_quantity", order_qty.getValue());
  // audit_log.WriteElement("minimum_quantity", 0);
  audit_log.WriteElement("country_of_origin", sender_loc_id_);
  // audit_log.WriteElement("")
  audit_trail_.WriteLog(audit_log);

  cout << "ReqOrderInsert:" << order->order_id << endl;
  FIX::Session::sendToTarget(new_order, session_id_);
}

void FixTrader::ReqOrderAction(Order *order) {
  order->order_id = order_pool_.add(order);
  Order *orig_order = order_pool_.get(order->orig_order_id);
  seq_serial_.DumpOrderID(order->order_id);
  // string trans_id = seq_serial_.GenTransIDStr();
  string order_flow_id = orig_order->order_flow_id;
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
  FIX::Symbol symbol(orig_order->symbol);
  // FIX::Symbol symbol("GE");
  string timestamp(time_now());
  FIX::TransactTime transact_time(timestamp.c_str());

  CME_FIX_NAMESPACE::OrderCancelRequest cancel_order(orig_cl_order_id,
                                                     cl_order_id,
                                                     symbol,
                                                     side,
                                                     transact_time);

  FIX::Account account(orig_order->account);
  FIX::OrderID order_id(orig_order->sys_order_id);
  cancel_order.set(account);
  cancel_order.set(order_id);

  // 1028-ManualOrderIndicator : Y=manual N=antomated
  // cancel_order.setField(1028, "n");
  cancel_order.setField(1028, "N");

  // SecurityDesc : Future Example: GEZ8
  //                Option Example: CEZ9 C9375
  // Is SecurityDesc a type? 
  FIX::SecurityDesc security_desc(orig_order->instrument_id);
  cout << "ReqOrderAction tag-107 " << orig_order->instrument_id << endl;
  cancel_order.set(security_desc);
  // cancel_order.setField(FIX::FIELD::SecurityDesc, "GEZ8");

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

  // FIX::SenderCompID sender_comp_id;
  // cancel_order.getHeader().getField(sender_comp_id);
  // string str_sender_comp_id = sender_comp_id.getValue();
  // string session_id = str_sender_comp_id.substr(0, 3);
  // string session_id = "3T7";
  // string firm_id = "004";
  // string sender_sub_id = "G";

  AuditLog audit_log;
  audit_log.WriteElement("sending_timestamps", timestamp);
  audit_log.WriteElement("message_direction", "TO CME");
  audit_log.WriteElement("operator_id", target_sub_id_);
  // audit_log.WriteElement("self_match_prevention_id", "CASHALGO_CFI");
  audit_log.WriteElement("account_number", account.getValue());
  audit_log.WriteElement("session_id", acc_session_id_);
  audit_log.WriteElement("executing_firm_id", firm_id_);
  audit_log.WriteElement("manual_order_identifier", "N");  // field-1028
  
  audit_log.WriteElement("message_type", FIX::MsgType_OrderCancelRequest);
  audit_log.WriteElement("customer_type_indicator", "2");  // CtiCode-9702
  // audit_log.WriteElement("origin", "Firm");
  // audit_log.WriteElement("message_link_id", trans_id);
  audit_log.WriteElement("order_flow_id", order_flow_id);
  audit_log.WriteElement("instrument_description", security_desc.getValue());
  // market segment id can be found in InstrumentDefinition in market data
  audit_log.WriteElement("market_segment_id", sender_sub_id_);
  audit_log.WriteElement("client_order_id", cl_order_id.getValue());
  audit_log.WriteElement("cme_globex_order_id", order_id.getValue());
  audit_log.WriteElement("buy_sell_indicator", side.getValue());
  // audit_log.WriteElement("ifm_flag", "N");
  // audit_log.WriteElement("display_quantity", order_qty.getValue());
  // audit_log.WriteElement("minimum_quantity", 0);
  audit_log.WriteElement("country_of_origin", sender_loc_id_);
  // audit_log.WriteElement("")
  audit_trail_.WriteLog(audit_log);

  cout << "ReqOrderAction:" << orig_order_id_str << endl;
  FIX::Session::sendToTarget(cancel_order, session_id_);
}


void FixTrader::ReqOrderAction(string str_symbol, string instrument_id,
        string str_side, string local_id, string sys_id, string str_account) {
  Order *order = new Order();
  order->order_id = order_pool_.add(order);
  seq_serial_.DumpOrderID(order->order_id);
  // string trans_id = seq_serial_.GenTransIDStr();
  string order_flow_id = seq_serial_.GenFlowIDStr(atoi(local_id.c_str()));
  // Order *orig_order = order_pool_.get(order->orig_order_id);
  char cl_order_id_str[16], orig_order_id_str[16];
  snprintf(cl_order_id_str, sizeof(cl_order_id_str), "%d", order->order_id);
  snprintf(orig_order_id_str, sizeof(orig_order_id_str), "%s", local_id.c_str());
  FIX::ClOrdID cl_order_id(cl_order_id_str);
  FIX::OrigClOrdID orig_cl_order_id(orig_order_id_str);
  FIX::Side side;
  if (strcasecmp(str_side.c_str(), "buy") == 0) {
    side = FIX::Side_BUY;
  } else {
    side = FIX::Side_SELL;
  }
  FIX::Symbol symbol(str_symbol.c_str());
  // FIX::Symbol symbol("GE");
  string timestamp(time_now());
  FIX::TransactTime transact_time(timestamp.c_str());

  CME_FIX_NAMESPACE::OrderCancelRequest cancel_order(orig_cl_order_id,
                                                     cl_order_id,
                                                     symbol,
                                                     side,
                                                     transact_time);

  FIX::Account account(str_account.c_str());
  FIX::OrderID order_id(sys_id.c_str());
  cancel_order.set(account);
  cancel_order.set(order_id);

  // 1028-ManualOrderIndicator : Y=manual N=antomated
  // cancel_order.setField(1028, "n");
  cancel_order.setField(1028, "N");

  // SecurityDesc : Future Example: GEZ8
  //                Option Example: CEZ9 C9375
  // Is SecurityDesc a type? 
  FIX::SecurityDesc security_desc(instrument_id.c_str());
  cout << "ReqOrderAction tag-107 " << instrument_id << endl;
  cancel_order.set(security_desc);
  // cancel_order.setField(FIX::FIELD::SecurityDesc, "GEZ8");

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

  // FIX::SenderCompID sender_comp_id;
  // cancel_order.getHeader().getField(sender_comp_id);
  // string str_sender_comp_id = sender_comp_id.getValue();
  // string session_id = str_sender_comp_id.substr(0, 3);
  // string session_id = "3T7";
  // string firm_id = "004";
  // string sender_sub_id = "G";

  AuditLog audit_log;
  audit_log.WriteElement("sending_timestamps", timestamp);
  audit_log.WriteElement("message_direction", "TO CME");
  audit_log.WriteElement("operator_id", target_sub_id_);
  // audit_log.WriteElement("self_match_prevention_id", "CASHALGO_CFI");
  audit_log.WriteElement("account_number", account.getValue());
  audit_log.WriteElement("session_id", acc_session_id_);
  audit_log.WriteElement("executing_firm_id", firm_id_);
  audit_log.WriteElement("manual_order_identifier", "N");  // field-1028
  
  audit_log.WriteElement("message_type", FIX::MsgType_OrderCancelRequest);
  // audit_log.WriteElement("customer_type_indicator", "2");  // CtiCode-9702
  // audit_log.WriteElement("origin", "Firm");
  // audit_log.WriteElement("message_link_id", trans_id);
  audit_log.WriteElement("order_flow_id", order_flow_id);
  audit_log.WriteElement("instrument_description", security_desc.getValue());
  // market segment id can be found in InstrumentDefinition in market data
  audit_log.WriteElement("market_segment_id", sender_sub_id_);
  audit_log.WriteElement("client_order_id", cl_order_id.getValue());
  audit_log.WriteElement("cme_globex_order_id", order_id.getValue());
  audit_log.WriteElement("buy_sell_indicator", side.getValue());
  // audit_log.WriteElement("ifm_flag", "N");
  // audit_log.WriteElement("display_quantity", order_qty.getValue());
  // audit_log.WriteElement("minimum_quantity", 0);
  audit_log.WriteElement("country_of_origin", sender_loc_id_);
  // audit_log.WriteElement("")
  audit_trail_.WriteLog(audit_log);

  cout << "ReqOrderAction With Info:" << symbol << " "
       << instrument_id << " " << side << " " << local_id
       << " " << sys_id << endl;
  FIX::Session::sendToTarget(cancel_order, session_id_);
}

void FixTrader::ReqMassOrderAction(Order *order) {
  FIX::Message mass_order_cancel;
  order->order_id = order_pool_.add(order);
  seq_serial_.DumpOrderID(order->order_id);
  char cl_order_id_str[16];
  snprintf(cl_order_id_str, sizeof(cl_order_id_str), "%d", order->order_id);
  FIX::ClOrdID cl_order_id(cl_order_id_str);
  FIX::SecurityDesc security_desc(order->instrument_id);
  string timestamp(time_now());
  FIX::TransactTime transact_time(timestamp.c_str());

  mass_order_cancel.setField(cl_order_id);
  mass_order_cancel.setField(security_desc);
  mass_order_cancel.setField(transact_time);
  int mass_action_type = 3;
  int mass_action_scope = 1;
  char action_type[8], action_scope[8], manual_order_indicator[4];
  snprintf(action_type, sizeof(action_type), "%d", mass_action_type);
  snprintf(action_scope, sizeof(action_scope), "%d", mass_action_scope);
  snprintf(action_scope, sizeof(action_scope), "N");

  mass_order_cancel.setField(1373, action_type);
  mass_order_cancel.setField(1374, action_scope);
  mass_order_cancel.setField(1028, manual_order_indicator);

  cout << "ReqMassOrderAction: " << order->instrument_id << endl;
  FIX::Session::sendToTarget(mass_order_cancel, session_id_);
}

void FixTrader::ReqOrderReplace(Order *order) {
  order->order_id = order_pool_.add(order);
  Order *orig_order = order_pool_.get(order->orig_order_id);
  seq_serial_.DumpOrderID(order->order_id);
  // string trans_id = seq_serial_.GenTransIDStr();
  string order_flow_id = orig_order->order_flow_id;
  char cl_order_id_str[16], orig_order_id_str[16];
  snprintf(cl_order_id_str, sizeof(cl_order_id_str), "%d", order->order_id);
  snprintf(orig_order_id_str, sizeof(orig_order_id_str), "%d",
           order->orig_order_id);
  FIX::ClOrdID cl_order_id(cl_order_id_str);
  FIX::OrigClOrdID orig_cl_order_id(orig_order_id_str);
  FIX::HandlInst handl_inst('1');
  FIX::Side side;
  if (orig_order->direction == kDirectionBuy) {
    side = FIX::Side_BUY;
  } else {
    side = FIX::Side_SELL;
  }
  FIX::OrdType order_type = FIX::OrdType_LIMIT;
  if (order->order_type == kOrderTypeLimit) {
    order_type = FIX::OrdType_LIMIT;
  } else if (order->order_type == kOrderTypeMarket) {
    order_type = FIX::OrdType_MARKET;
  }
  FIX::Symbol symbol(orig_order->symbol);
  // FIX::Symbol symbol("GE");
  string timestamp(time_now());
  FIX::TransactTime transact_time(timestamp.c_str());

  CME_FIX_NAMESPACE::OrderCancelReplaceRequest replace_order(
          orig_cl_order_id, cl_order_id, handl_inst, symbol,
          side, transact_time, order_type);

  FIX::Account account(orig_order->account);
  FIX::OrderID order_id(orig_order->sys_order_id);
  replace_order.set(account);
  replace_order.set(order_id);

  FIX::Price price(order->limit_price);
  replace_order.set(price);

  FIX::OrderQty order_qty(order->volume);
  replace_order.set(order_qty);

  FIX::CustomerOrFirm customer_or_firm(1);
  replace_order.set(customer_or_firm);

  // 1028-ManualOrderIndicator : Y=manual N=antomated
  // replace_order.setField(1028, "n");
  replace_order.setField(1028, "N");

  // 1028-ManualOrderIndicator : Y=manual N=antomated
  // replace_order.setField(1028, "n");
  replace_order.setField(9702, "2");

  FIX::SecurityDesc security_desc(orig_order->instrument_id);
  replace_order.set(security_desc);

  // SecurityType : FUT=Future
  //                OPT=Option
  // FIX::SecurityType security_type("FUT");
  // new_order.set(security_type);
  replace_order.setField(FIX::FIELD::SecurityType, "FUT");

  // 9717-CorrelationClOrdID
  // This tag should contain the same value as the tag-11 ClOrdID 
  // of the original New Order message and is used to correlate iLink
  // messages associated with a single order chain
  // ClOrdID or OrigClOrdID ?
  // replace_order.setField(9717, cl_order_id_str);
  replace_order.setField(9717, orig_order_id_str);

  // FIX::SenderCompID sender_comp_id;
  // replace_order.getHeader().getField(sender_comp_id);
  // string str_sender_comp_id = sender_comp_id.getValue();
  // string session_id = str_sender_comp_id.substr(0, 3);
  // string session_id = "3T7";
  // string firm_id = "004";
  // string sender_sub_id = "G";

  AuditLog audit_log;
  audit_log.WriteElement("sending_timestamps", timestamp);
  audit_log.WriteElement("message_direction", "TO CME");
  audit_log.WriteElement("operator_id", target_sub_id_);
  // audit_log.WriteElement("self_match_prevention_id", "CASHALGO_CFI");
  audit_log.WriteElement("account_number", account.getValue());
  audit_log.WriteElement("session_id", acc_session_id_);
  audit_log.WriteElement("executing_firm_id", firm_id_);
  audit_log.WriteElement("manual_order_identifier", "N");  // field-1028
  audit_log.WriteElement("message_type", FIX::MsgType_OrderCancelReplaceRequest);
  audit_log.WriteElement("customer_type_indicator", "2");  // CtiCode-9702
  audit_log.WriteElement("origin", customer_or_firm.getValue());
  // audit_log.WriteElement("message_link_id", trans_id);
  audit_log.WriteElement("order_flow_id", order_flow_id);
  audit_log.WriteElement("instrument_description", security_desc.getValue());
  // market segment id can be found in InstrumentDefinition in market data
  audit_log.WriteElement("market_segment_id", sender_sub_id_);
  audit_log.WriteElement("client_order_id", cl_order_id.getValue());
  audit_log.WriteElement("cme_globex_order_id", order_id.getValue());
  audit_log.WriteElement("buy_sell_indicator", side.getValue());
  audit_log.WriteElement("quantity", (int)order_qty.getValue());
  audit_log.WriteElement("limit_price", price.getValue());
  audit_log.WriteElement("order_type", order_type.getValue());
  audit_log.WriteElement("ifm_flag", "N");
  // audit_log.WriteElement("display_quantity", order_qty.getValue());
  // audit_log.WriteElement("minimum_quantity", 0);
  audit_log.WriteElement("country_of_origin", sender_loc_id_);
  // audit_log.WriteElement("")
  audit_trail_.WriteLog(audit_log);

  printf("ReqOrderReplace:%s volume:[%d->%d] price:[%lf->%lf]\n",
          orig_order_id_str, orig_order->volume, order->volume,
          orig_order->limit_price, orig_order->limit_price);
  FIX::Session::sendToTarget(replace_order, session_id_);
}

void FixTrader::run() {
  // TODO
  int count = 1;
  cout << "run" << endl;
  double price = 9860.0;
  int order_id = 0;
  while(true) {
    sleep(1);
    if (count++ % 20 == 0) {
      Order order;
      snprintf(order.instrument_id, sizeof(order.instrument_id), "GEZ8");
      snprintf(order.account, sizeof(order.account), "W80004");
      order.volume = 2;
      order.limit_price = price;
      ReqOrderInsert(&order);
      sleep(3);
      price += 2.0;

      Order cancel_order;
      cancel_order.orig_order_id = order.order_id;
      ReqOrderAction(&cancel_order);
    }
  }
}

void FixTrader::OnRspOrderInsert(OrderAck *order_ack) {
  Order *order = order_pool_.get(order_ack->order_id);
  if (order != NULL) {
    strcpy(order->sys_order_id, order_ack->sys_order_id);
    order_pool_.add_pair(order_ack->sys_order_id, order_ack->order_id);
    printf("OnRspOrderInsert: %d-%s\n", order_ack->order_id,
            order_ack->sys_order_id);
  } else {
    printf("OnRspOrderInsert Error: Order %d-%s not found\n",
            order_ack->order_id, order_ack->sys_order_id);
  }
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
  FIX::ClOrdID cl_order_id;
  report.getField(cl_order_id);
  string cl_ord_id = cl_order_id.getValue();
  order_ack.order_id = atol(cl_ord_id.c_str());
  FIX::OrderID order_id;
  report.getField(order_id);
  string sys_order_id = order_id.getValue();
  strcpy(order_ack.sys_order_id, sys_order_id.c_str());
  return order_ack;
}

Deal FixTrader::ToDeal(const CME_FIX_NAMESPACE::ExecutionReport& report) {
  Deal deal;

  return deal;
}

void FixTrader::PrintExecutionReport(
      const CME_FIX_NAMESPACE::ExecutionReport& report) {
  FIX::OrdStatus ord_status;
  report.getField(ord_status);

  AuditLog audit_log;

  if (ord_status == FIX::OrdStatus_NEW) {
    cout << "[Execution Report Acknowledgment]:" << endl;
  } else if (ord_status == FIX::OrdStatus_PARTIALLY_FILLED) {
    FIX::LastPx last_px;
    FIX::LastQty last_qty;
    FIX::AggressorIndicator aggressor_indicator;
    report.getField(last_px);
    report.getField(last_qty);
    report.getField(aggressor_indicator);
    cout << "[Execution Report PartiallyFill]:" << endl;
    cout << "LastPx:[" << last_px << "]" << endl;
    cout << "LastQty:[" << last_qty << "]" << endl;
    audit_log.WriteElement("fill_price", last_px.getValue());
    audit_log.WriteElement("fill_quantity", (int)last_qty.getValue());
    if (aggressor_indicator.getValue()) {
      audit_log.WriteElement("aggressor_flag", "Y");
    } else {
      audit_log.WriteElement("aggressor_flag", "N");
    }
  } else if (ord_status == FIX::OrdStatus_FILLED) {
    FIX::LastPx last_px;
    FIX::LastQty last_qty;
    FIX::AggressorIndicator aggressor_indicator;
    report.getField(last_px);
    report.getField(last_qty);
    report.getField(aggressor_indicator);
    cout << "[Execution Report AllFill]:" << endl;
    cout << "LastPx:[" << last_px << "]" << endl;
    cout << "LastQty:[" << last_qty << "]" << endl;
    audit_log.WriteElement("fill_price", last_px.getValue());
    audit_log.WriteElement("fill_quantity", (int)last_qty.getValue());
    if (aggressor_indicator.getValue()) {
      audit_log.WriteElement("aggressor_flag", "Y");
    } else {
      audit_log.WriteElement("aggressor_flag", "N");
    }
  } else if (ord_status == FIX::OrdStatus_CANCELED) {
    cout << "[Execution Report Cancellation]:" << endl;
  } else if (ord_status == FIX::OrdStatus_REPLACED) {
    cout << "[Execution Report Modification]:" << endl;
  } else if (ord_status == FIX::OrdStatus_REJECTED) {
    cout << "[Execution Report Reject]:" << endl;
    FIX::OrdRejReason ord_rej_reason;
    report.getField(ord_rej_reason);
    audit_log.WriteElement("reject_reason", ord_rej_reason.getValue());
  } else if (ord_status == FIX::OrdStatus_EXPIRED) {
    cout << "[Execution Report Elimination]:" << endl;
  } else if (ord_status == 'H') {
    cout << "[Execution Report Trade Cancellation]:" << endl;
  } else {
    cout << "[Unknown Type Execution Report]:" << endl;
  }
  PrintBasicExecutionReport(report);

  // string trans_id = seq_serial_.GenTransIDStr();

  FIX::TargetCompID target_comp_id;
  FIX::TargetSubID target_sub_id;
  FIX::SenderSubID sender_sub_id;
  FIX::Account account;
  FIX::ClOrdID cl_ord_id;
  FIX::CumQty cum_qty;
  FIX::ExecID exec_id;
  FIX::OrderID order_id;
  FIX::OrderQty order_qty;
  FIX::LeavesQty leaves_qty;
  FIX::OrdType ord_type;
  FIX::Price price;
  FIX::Side side;
  FIX::TimeInForce time_in_force;
  FIX::TransactTime transact_time;
  FIX::SecurityDesc security_desc;
  
  report.getHeader().getField(target_comp_id);
  report.getHeader().getField(target_sub_id);
  report.getHeader().getField(sender_sub_id);
  report.getField(account);
  report.getField(cl_ord_id);
  report.getField(cum_qty);
  report.getField(exec_id);
  report.getField(order_id);
  report.getField(order_qty);
  report.getField(leaves_qty);
  report.getField(ord_type);
  report.getField(price);
  report.getField(side);
  report.getField(time_in_force);
  report.getField(transact_time);
  report.getField(security_desc);

  string str_target_comp_id = target_comp_id.getValue();
  string str_target_sub_id = target_sub_id.getValue();
  // string str_sender_sub_id = sender_sub_id.getValue();
  string session_id = str_target_comp_id.substr(0, 3);
  string firm_id = str_target_comp_id.substr(3, 3);
  string manual_order_identifier = report.getField(1028);

  if (ord_status == FIX::OrdStatus_NEW ||
      ord_status == FIX::OrdStatus_PARTIALLY_FILLED ||
      ord_status == FIX::OrdStatus_FILLED) {
    // if (report.getFieldIfSet(7928)) {
    if (report.isSetField(7928)) {
      string self_match_prevention_id = report.getField(7928);
      audit_log.WriteElement("self_match_prevention_id", 
          self_match_prevention_id);
    }
  }


  string message_type = string(FIX::MsgType_ExecutionReport) + "/" +
                        ord_status.getValue();
  Order *order = order_pool_.get(order_id.getValue());
  string order_flow_id = "";
  if (order != NULL) {
    order_flow_id = order->order_flow_id;
  }

  int year, month, day, hour, min, second, milsec;
  transact_time.getValue().getYMD(year, month, day);
  transact_time.getValue().getHMS(hour, min, second, milsec);
  char timestamp[32];
  snprintf(timestamp, sizeof(timestamp), "%d%02d%02d-%02d:%02d:%02d.%03d",
           year, month, day, hour, min, second, milsec);
  audit_log.WriteElement("receiving_timestamps", timestamp);
  audit_log.WriteElement("message_direction", "FROM CME");
  audit_log.WriteElement("operator_id", str_target_sub_id);
  audit_log.WriteElement("account_number", account.getValue());
  audit_log.WriteElement("session_id", session_id);
  audit_log.WriteElement("executing_firm_id", firm_id);
  audit_log.WriteElement("message_type", message_type);
  // audit_log.WriteElement("customer_type_indicator", "2");  // CtiCode-9702
  // audit_log.WriteElement("origin", "Firm");
  audit_log.WriteElement("cme_globex_message_id", exec_id.getValue());
  // audit_log.WriteElement("message_link_id", trans_id);
  audit_log.WriteElement("order_flow_id", order_flow_id);
  audit_log.WriteElement("instrument_description", security_desc.getValue());
  // market segment id can be found in InstrumentDefinition in market data
  audit_log.WriteElement("market_segment_id", str_target_sub_id);
  audit_log.WriteElement("client_order_id", cl_ord_id.getValue());
  audit_log.WriteElement("cme_globex_order_id", order_id.getValue());
  audit_log.WriteElement("buy_sell_indicator", side.getValue());
  audit_log.WriteElement("quantity", (int)order_qty.getValue());
  audit_log.WriteElement("limit_price", price.getValue());
  audit_log.WriteElement("order_type", ord_type.getValue());
  audit_log.WriteElement("order_qualifier", time_in_force);
  // audit_log.WriteElement("ifm_flag", "N");
  // audit_log.WriteElement("display_quantity", order_qty.getValue());
  // audit_log.WriteElement("minimum_quantity", 0);
  // audit_log.WriteElement("country_of_origin", "HK");
  audit_log.WriteElement("cumulative_quantity", (int)cum_qty.getValue());
  audit_log.WriteElement("remaining_quantity", (int)leaves_qty.getValue());
  audit_log.WriteElement("manual_order_identifier", manual_order_identifier);
  // audit_log.WriteElement("")
  audit_trail_.WriteLog(audit_log);
}

void FixTrader::PrintBasicExecutionReport(
      const CME_FIX_NAMESPACE::ExecutionReport& report) {
  FIX::Account account;
  FIX::AvgPx avg_px;
  FIX::ClOrdID cl_ord_id;
  FIX::CumQty cum_qty;
  FIX::ExecID exec_id;
  FIX::OrderID order_id;
  FIX::OrderQty order_qty;
  FIX::OrdStatus ord_status;
  FIX::OrdType ord_type;
  FIX::OrigClOrdID orig_cl_order_id;
  FIX::Price price;
  FIX::SecurityID security_id;
  FIX::Side side;
  FIX::Symbol symbol;
  FIX::TimeInForce time_in_force;
  FIX::TransactTime transact_time;
  FIX::SecurityDesc security_desc;
  FIX::ExecType exec_type;
  FIX::SecurityType security_type;
  
  report.getField(account);
  report.getField(avg_px);
  report.getField(cl_ord_id);
  report.getField(cum_qty);
  report.getField(exec_id);
  report.getField(order_id);
  report.getField(order_qty);
  report.getField(ord_status);
  report.getField(ord_type);
  report.getField(orig_cl_order_id);
  report.getField(price);
  report.getField(security_id);
  report.getField(side);
  report.getField(symbol);
  report.getField(time_in_force);
  report.getField(transact_time);
  report.getField(security_desc);
  report.getField(exec_type);
  report.getField(security_type);

  string str_ord_status = ToString(ord_status);
  string str_ord_type = ToString(ord_type);
  string str_side = ToString(side);
  string str_time_in_force = ToString(time_in_force);
  string str_exec_type = ToString(exec_type);

  cout << "[ExecutionReport Basic Information]:" << endl;
  cout << "Account:[" << account << "]" << endl;
  cout << "AvgPx:[" << avg_px << "]" << endl;
  cout << "ClOrdID:[" << cl_ord_id << "]" << endl;
  cout << "CumQty:[" << cum_qty << "]" << endl;
  cout << "ExecID:[" << exec_id << "]" << endl;
  cout << "OrderID:[" << order_id << "]" << endl;
  cout << "OrderQty:[" << order_qty << "]" << endl;
  cout << "OrdStatus:[" << str_ord_status << "]" << endl;
  cout << "OrdType:[" << str_ord_type << "]" << endl;
  cout << "OrigClOrdID:[" << orig_cl_order_id << "]" << endl;
  cout << "Price:[" << price << "]" << endl;
  cout << "SecurityID:[" << security_id << "]" << endl;
  cout << "Side:[" << str_side << "]" << endl;
  cout << "Symbol:[" << symbol << "]" << endl;
  cout << "TimeInForce:[" << str_time_in_force << "]" << endl;
  cout << "TransactTime:[" << transact_time << "]" << endl;
  cout << "SecurityDesc:[" << security_desc << "]" << endl;
  cout << "ExecType:[" << str_exec_type << "]" << endl;
  cout << "SecurityType:[" << security_type << "]" << endl;
}

void FixTrader::PrintOrderCancelReject(
      const CME_FIX_NAMESPACE::OrderCancelReject& report) {
  FIX::TargetCompID target_comp_id;
  FIX::TargetSubID target_sub_id;
  FIX::SenderSubID sender_sub_id;
  FIX::Account account;
  FIX::ClOrdID cl_ord_id;
  FIX::ExecID exec_id;
  FIX::OrderID order_id;
  FIX::OrderQty order_qty;
  FIX::OrdStatus ord_status;
  FIX::OrigClOrdID orig_cl_order_id;
  FIX::SecurityID security_id;
  FIX::Text text;
  FIX::TransactTime transact_time;
  FIX::CxlRejReason cxl_rej_reason;
  FIX::SecurityDesc security_desc;
  FIX::CxlRejResponseTo cxl_rej_response_to;
  
  report.getHeader().getField(target_comp_id);
  report.getHeader().getField(target_sub_id);
  report.getHeader().getField(sender_sub_id);
  report.getField(account);
  report.getField(cl_ord_id);
  report.getField(exec_id);
  report.getField(order_id);
  report.getField(ord_status);
  report.getField(orig_cl_order_id);
  report.getField(security_id);
  report.getField(text);
  report.getField(transact_time);
  report.getField(cxl_rej_reason);
  report.getField(security_desc);
  report.getField(cxl_rej_response_to);
  string manual_order_identifier = report.getField(1028);
  // string self_match_prevention_id = report.getField(7928);

  AuditLog audit_log;

  string str_target_sub_id = target_sub_id.getValue();
  string str_sender_sub_id = sender_sub_id.getValue();
  string str_target_comp_id = target_comp_id.getValue();
  string session_id = str_target_comp_id.substr(0, 3);
  string firm_id = str_target_comp_id.substr(3, 3);

  Order *order = order_pool_.get(order_id.getValue());
  string order_flow_id = "";
  if (order != NULL) {
    order_flow_id = order->order_flow_id;
  }

  string message_type = string(FIX::MsgType_ExecutionReport) + "/" +
                        cxl_rej_response_to.getValue();

  int year, month, day, hour, min, second, milsec;
  transact_time.getValue().getYMD(year, month, day);
  transact_time.getValue().getHMS(hour, min, second, milsec);
  char timestamp[32];
  snprintf(timestamp, sizeof(timestamp), "%d%02d%02d-%02d:%02d:%02d.%03d",
           year, month, day, hour, min, second, milsec);
  audit_log.WriteElement("receiving_timestamps", timestamp);
  audit_log.WriteElement("message_direction", "FROM CME");
  audit_log.WriteElement("operator_id", str_target_sub_id);
  // audit_log.WriteElement("self_match_prevention_id", self_match_prevention_id);
  audit_log.WriteElement("account_number", account.getValue());
  audit_log.WriteElement("session_id", session_id);
  audit_log.WriteElement("executing_firm_id", firm_id);
  audit_log.WriteElement("message_type", message_type);
  // audit_log.WriteElement("customer_type_indicator", "2");  // CtiCode-9702
  // audit_log.WriteElement("origin", "Firm");
  audit_log.WriteElement("cme_globex_message_id", exec_id.getValue());
  // audit_log.WriteElement("message_link_id", trans_id);
  audit_log.WriteElement("order_flow_id", order_flow_id);
  audit_log.WriteElement("instrument_description", security_desc.getValue());
  // market segment id can be found in InstrumentDefinition in market data
  audit_log.WriteElement("market_segment_id", str_target_sub_id);
  audit_log.WriteElement("client_order_id", cl_ord_id.getValue());
  audit_log.WriteElement("cme_globex_order_id", order_id.getValue());
  audit_log.WriteElement("reject_reason", cxl_rej_reason.getValue());
  audit_log.WriteElement("manual_order_identifier", manual_order_identifier);
  
  // audit_log.WriteElement("ifm_flag", "N");
  // audit_log.WriteElement("display_quantity", order_qty.getValue());
  // audit_log.WriteElement("minimum_quantity", 0);
  // audit_log.WriteElement("country_of_origin", "HK");
  // audit_log.WriteElement("")
  audit_trail_.WriteLog(audit_log);

  string str_ord_status = ToString(ord_status);

  cout << "[Order Cancel Reject]:" << endl;
  cout << "Account:[" << account << "]" << endl;
  cout << "ClOrdID:[" << cl_ord_id << "]" << endl;
  cout << "ExecID:[" << exec_id << "]" << endl;
  cout << "OrderID:[" << order_id << "]" << endl;
  cout << "OrdStatus:[" << str_ord_status << "]" << endl;
  cout << "OrigClOrdID:[" << orig_cl_order_id << "]" << endl;
  cout << "SecurityID:[" << security_id << "]" << endl;
  cout << "Text:[" << text << "]" << endl;
  cout << "TransactTime:[" << transact_time << "]" << endl;
  cout << "CxlRejReason:[" << cxl_rej_response_to << "]" << endl;
  cout << "SecurityDesc:[" << security_desc << "]" << endl;
  cout << "CxlRejResponseTo:[" << cxl_rej_response_to << "]" << endl;
}

void FixTrader::FillHeader(FIX::Message& message) {
  // message.getHeader().setField(FIX::SenderSubID("ANYTHING"));
  message.getHeader().setField(FIX::SenderSubID(sender_sub_id_));
  message.getHeader().setField(FIX::SenderLocationID(sender_loc_id_));
  message.getHeader().setField(FIX::TargetSubID(target_sub_id_));
}

string FixTrader::ToString(FIX::OrdStatus ord_status) {
  char val_ord_status[8] = {0};
  val_ord_status[0] = ord_status.getValue();
  string str_ord_status = val_ord_status;
  if (ord_status == FIX::OrdStatus_NEW) {
    str_ord_status = "NEW";
  } else if (ord_status == FIX::OrdStatus_PARTIALLY_FILLED) {
    str_ord_status = "PARTIALLY_FILLED";
  } else if (ord_status == FIX::OrdStatus_FILLED) {
    str_ord_status = "FILLED";
  } else if (ord_status == FIX::OrdStatus_DONE_FOR_DAY) {
    str_ord_status = "DONE_FOR_DAY";
  } else if (ord_status == FIX::OrdStatus_CANCELED) {
    str_ord_status = "CANCELED";
  } else if (ord_status == FIX::OrdStatus_REPLACED) {
    str_ord_status = "REPLACED";
  } else if (ord_status == FIX::OrdStatus_PENDING_CANCEL) {
    str_ord_status = "PENGING_CANCEL";
  } else if (ord_status == FIX::OrdStatus_STOPPED) {
    str_ord_status = "STOPPED";
  } else if (ord_status == FIX::OrdStatus_REJECTED) {
    str_ord_status = "REJECTED";
  } else if (ord_status == FIX::OrdStatus_EXPIRED) {
    str_ord_status = "EXPIRED";
  }
  return str_ord_status;
}

string FixTrader::ToString(FIX::OrdType ord_type) {
  char val_ord_type[8] = {0};
  val_ord_type[0] = ord_type.getValue();
  string str_ord_type = val_ord_type;
  if (ord_type == FIX::OrdType_LIMIT) {
    str_ord_type = "LIMIT";
  } else if (ord_type == FIX::OrdType_MARKET) {
    str_ord_type = "MARKET";
  } else if (ord_type == FIX::OrdType_STOP) {
    str_ord_type = "STOP";
  } else if (ord_type == FIX::OrdType_STOP_LIMIT) {
    str_ord_type = "STOP_LIMIT";
  } else if (ord_type == 'K') {
    str_ord_type = "MARKET_LIMIT";
  }
  return str_ord_type;
}

string FixTrader::ToString(FIX::Side side) {
  char val_side[8] = {0};
  val_side[0] = side.getValue();
  string str_side = val_side;
  if (side == FIX::Side_BUY) {
    str_side = "BUY";
  } else if (side == FIX::Side_SELL) {
    str_side = "SELL";
  }
  return str_side;
}

string FixTrader::ToString(FIX::TimeInForce time_in_force) {  
  char val_time_in_force[8] = {0};
  val_time_in_force[0] = time_in_force.getValue();
  string str_time_in_force = val_time_in_force;
  if (time_in_force == FIX::TimeInForce_DAY) {
    str_time_in_force = "DAY";
  } else if (time_in_force == FIX::TimeInForce_GOOD_TILL_CANCEL) {
    str_time_in_force = "GTC";
  } else if (time_in_force == FIX::TimeInForce_IMMEDIATE_OR_CANCEL) {
    str_time_in_force = "FAK";
  } else if (time_in_force == FIX::TimeInForce_FILL_OR_KILL) {
    str_time_in_force = "FOK";
  } else if (time_in_force == FIX::TimeInForce_GOOD_TILL_DATE) {
    str_time_in_force = "GTD";
  }
  return str_time_in_force;
}

string FixTrader::ToString(FIX::ExecType exec_type) {
  char val_exec_type[8] = {0};
  val_exec_type[0] = exec_type.getValue();
  string str_exec_type = val_exec_type;
  if (exec_type == FIX::ExecType_NEW) {
    str_exec_type = "NEW";
  } else if (exec_type == FIX::ExecType_PARTIAL_FILL) {
    str_exec_type = "PARTIAL_FILL";
  } else if (exec_type == FIX::ExecType_FILL) {
    str_exec_type = "FILL";
  } else if (exec_type == FIX::ExecType_DONE_FOR_DAY) {
    str_exec_type = "DONE_FOR_DAY";
  } else if (exec_type == FIX::ExecType_CANCELED) {
    str_exec_type = "CANCELED";
  } else if (exec_type == FIX::ExecType_REPLACE) {
    str_exec_type = "REPLACE";
  } else if (exec_type == FIX::ExecType_PENDING_CANCEL) {
    str_exec_type = "PENGING_CANCEL";
  } else if (exec_type == FIX::ExecType_STOPPED) {
    str_exec_type = "STOPPED";
  } else if (exec_type == FIX::ExecType_REJECTED) {
    str_exec_type = "REJECTED";
  } else if (exec_type == FIX::ExecType_EXPIRED) {
    str_exec_type = "ELIMINATION_ACK";
  } else if (exec_type == 'H') {
    str_exec_type = "TRADE_CANCEL_ACK";
  }
  return str_exec_type;
}
