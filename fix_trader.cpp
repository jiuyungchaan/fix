
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
  cout << "IN: " << message << endl;
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

    cout << "OUT: " << message << endl;
}

void FixTrader::toAdmin(FIX::Message& message, const FIX::SessionID&) {
  FIX::MsgType msg_type;
  message.getHeader().getField(msg_type);
  if (msg_type == FIX::MsgType_Logon) {
    char sz_user_name[32] = "testuser";
    char sz_password[32] = "testpassword";
    char sz_input_type[32] = "testinputtype";
    char sz_reset_seq_num_flag[5] = "A";

    // char sz_user_name[32] = {0};
    // char sz_password[32] = {0};
    // char sz_input_type[32] = {0};
    // char sz_reset_seq_num_flag[5] = {0};

    // GetPrivateProfileString("testdata", "UserName", "", sz_user_name, 
    //                         sizeof(sz_user_name) - 1, gConfigFileName);
    // GetPrivateProfileString("testdata", "Password", "", sz_password, 
    //                         sizeof(sz_password) - 1, gConfigFileName);
    // GetPrivateProfileString("testdata", "InputType", "", sz_input_type, 
    //                         sizeof(sz_input_type) - 1, gConfigFileName);
    // GetPrivateProfileString("testdata", "ResetSeqNumFlag", "Y",
    //                         sz_reset_seq_num_flag,
    //                         sizeof(sz_reset_seq_num_flag) - 1,
    //                         gConfigFileName);

    char sz_value[1024] = {0};
    snprintf(sz_value, sizeof(sz_value), "%s:%s:%s:", sz_input_type,
             sz_user_name, sz_password);
    message.setField(FIX::FIELD::RawData, sz_value);
    message.setField(FIX::FIELD::ResetSeqNumFlag, sz_reset_seq_num_flag);
    message.setField(FIX::FIELD::EncryptMethod, "0");
    string message_string = message.toString();
    cout << "Send Logon Message:\n" << message_string << endl;
  }
  if (msg_type == FIX::MsgType_Logout) {
    string message_string = message.toString();
    cout << "Send Logout Message:\n" << message_string << endl;
  }
}

CME_FIX_NAMESPACE::NewOrderSingle FixTrader::queryNewOrderSingle() {
  FIX::OrdType ord_type;
  CME_FIX_NAMESPACE::NewOrderSingle new_order_single()
}

