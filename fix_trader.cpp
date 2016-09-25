
#include "fix_trader.h"

#include <quickfix/Session.h>

#include <string>
#include <iostream>

using namespace std;

char *gConfigFileName = const_cast<char*>("./test.cfg");

void FixTrader::onLogon(const FIX::SessionID& sessionID) {
  cout << endl << "Logon - " << sessionID << endl;
}

void FixTrader::onLogout(const FIX::SessionID& sessionID) {
  cout << endl << "Logout - " << sessionID << endl;
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
  cout << "FROM APP: " << message << endl;
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

    cout << "TO APP: " << message << endl;
}

void FixTrader::fromAdmin(FIX::Message& message,
                          const FIX::SessionID& sessionID) 
    throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, 
          FIX::IncorrectTagValue, FIX::RejectLogon) {
  crack(message, sessionID);
  cout << "FROM ADMIN: " << message << endl;
}

void FixTrader::toAdmin(FIX::Message& message, const FIX::SessionID&) {
  FIX::MsgType msg_type;
  message.getHeader().getField(msg_type);
  if (msg_type == FIX::MsgType_Logon) {
    ReqUserLogon(message);
  }
  if (msg_type == FIX::MsgType_Logout) {
    ReqUserLogout(message);
  }

  cout << "TO ADMIN: " << message << endl;
}

void FixTrader::ReqUserLogon(const FIX::Message& message) {
    char sz_user_name[32] = "testuser";
    char sz_password[32] = "testpassword";
    char sz_input_type[32] = "testinputtype";
    char sz_reset_seq_num_flag[5] = "N";

    char raw_data[1024] = {0};
    char raw_data_len[16] = {0};
    char system_name[32] = "CME_CFI";
    char system_version[32] = "1.0";
    char system_vendor[32] = "Cash Algo";
    snprintf(raw_data, sizeof(raw_data), "%s", sz_password);
    // snrpintf(raw_data_len, sizeof(raw_data_len), "%d", strlen(raw_data));
    int raw_data_len = strlen(raw_data);
    message.setField(FIX::FIELD::RawData, raw_data);
    message.setField(FIX::FIELD::RawDataLength, raw_data_len);
    message.setField(FIX::FIELD::ResetSeqNumFlag, sz_reset_seq_num_flag);
    // message.setField(FIX::FIELD::EncryptMethod, "0");
    message.setField(FIX::FIELD::EncryptMethod, 0);  // string or int? type-safety?
    message.setField(1603, system_name);  // customed fields
    message.setField(1604, system_version);
    message.setField(1605, system_vendor);
    string message_string = message.toString();
    cout << "Send Logon Message:\n" << message_string << endl;
}

void FixTrader::ReqUserLogout(const FIX::Message& message) {
    string message_string = message.toString();
    cout << "Send Logout Message:\n" << message_string << endl;
}

CME_FIX_NAMESPACE::NewOrderSingle FixTrader::queryNewOrderSingle() {
  FIX::OrdType ord_type;
  CME_FIX_NAMESPACE::NewOrderSingle new_order_single()
}

