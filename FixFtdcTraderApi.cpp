////////////////////////
///@author Kenny Chiu
///@date 20161015
///@summary Implementation of CFixFtdcTraderApi and ImplFixFtdcTraderApi
///         
///
////////////////////////

#include "FixFtdcTraderApi.h"

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

#include <quickfix/Application.h>
#include <quickfix/MessageCracker.h>
#include <quickfix/Values.h>
#include <quickfix/Mutex.h>
#include <quickfix/FileStore.h>
#include <quickfix/SocketInitiator.h>
#include <quickfix/SessionSettings.h>
#include <quickfix/Log.h>

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

#include <quickfix/fix42/Logon.h>
#include <quickfix/fix42/Reject.h>
#include <quickfix/fix42/BusinessMessageReject.h>
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


/* ===============================================
   ================= DECLARATION ================= */

// AuditLog: Map to store the audit log trail elements
class AuditLog {
 public:
  AuditLog();
  ~AuditLog();

  void ClearElement(std::string key);

  int WriteElement(std::string key, std::string value);
  int WriteElement(std::string key, const char *value);
  int WriteElement(std::string key, int value);
  int WriteElement(std::string key, double value);
  int WriteElement(std::string key, char value);

  std::string ToString();

 private:
  std::map<std::string, std::string> elements;
};

// AuditTrail: Class to write the content of AuditLog into specified file
class AuditTrail {
 public:
  AuditTrail();
  ~AuditTrail();

  int Init(const char *log_file_name);
  void WriteLog(AuditLog& log);

 private:
  std::fstream log_file_;
};

// SequenceSerialization: To serialize the OrderID and FlowOrderID
// When system up, client should read from file to load the values of the
// last OrderID and FlowOrderID and then increment the ID
class SequenceSerialization {
 public:
  SequenceSerialization();
  virtual ~SequenceSerialization();

  int Init();
  void SetPrefix(const char *prefix);

  // Order-ID is identifier of requests including NewOrderSingle,
  // OrderCancelRequest, OrderCancelReplaceRequest and so on
  void DumpOrderID(int order_id);
  int GetCurOrderID();

  // Order-Flow-ID is identifier throughout the life of order starting
  // with the new order and ending once the order has been filled or 
  // canceled/expired
  void DumpFlowID();
  int GetCurFlowID();
  const char *GenFlowIDStr(int flow_id);

  // Transaction-ID is identifier of messages including request messages
  // sent and response messages received
  // void DumpTransID();
  // int GetCurTransID();
  // const char *GenTransIDStr();

 private:
  static const int MAX_ID = 100000000;
  std::fstream dump_file_;
  bool file_good_;
  int date_;
  char prefix_[32];

  int cur_order_id_;
  int cur_flow_id_;
  int cur_trans_id_;

  // pthread_mutex_t trans_lock_;
};

class ImplFixFtdcTraderApi : public CFixFtdcTraderApi, public FIX::Application,
                             public FIX::MessageCracker {
 public:
  explicit ImplFixFtdcTraderApi(const char *configPath);

  // interfaces of CFixFtdcTraderApi
  void Init();
  int Join();
  void RegisterFront(char *pszFrontAddress);
  void RegisterSpi(CFixFtdcTraderSpi *pSpi);
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


 protected:
  virtual ~ImplFixFtdcTraderApi();

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

  // interfaces of FIX::Application
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

  // interfaces of FIX::MessageCracker
  void onMessage(const CME_FIX_NAMESPACE::Logon& logon,
                 const FIX::SessionID& sessionID);
  void onMessage(const CME_FIX_NAMESPACE::ExecutionReport& report,
                 const FIX::SessionID& sessionID);
  void onMessage(const CME_FIX_NAMESPACE::OrderCancelReject& report,
                 const FIX::SessionID& sessionID);
  void onMessage(const CME_FIX_NAMESPACE::Reject& reject,
                 const FIX::SessionID& sessionID);
  void onMessage(const CME_FIX_NAMESPACE::BusinessMessageReject& reject,
                 const FIX::SessionID& sessionID);
  void onMassActionReport(const FIX::Message& message,
                          const FIX::SessionID& sessionID);
  void onXmlNonFix(const FIX::Message& message,
                   const FIX::SessionID& sessionID);

  void LogAuditTrail(const CME_FIX_NAMESPACE::ExecutionReport& report);
  void LogAuditTrail(const CME_FIX_NAMESPACE::OrderCancelReject& report);
  void LogAuditTrail(const CME_FIX_NAMESPACE::Reject& reject);
  void LogAuditTrail(const CME_FIX_NAMESPACE::BusinessMessageReject& reject);

  // void queryHeader(FIX::Header& header);
  void FillFixHeader(FIX::Message& message);
  void FillLogonRequest(FIX::Message& message);
  void FillLogoutRequest(FIX::Message& message);
  void FillHeartbeat(FIX::Message& message);
  void FillResendRequest(FIX::Message& message);
  void FillRejectRequest(FIX::Message& message);

  CThostFtdcOrderField ToOrderField(
        const CME_FIX_NAMESPACE::ExecutionReport& report);
  CThostFtdcTradeField ToTradeField(
        const CME_FIX_NAMESPACE::ExecutionReport& report);
  CThostFtdcInputOrderField ToInputOrderField(
        const CME_FIX_NAMESPACE::ExecutionReport& report);
  CThostFtdcInputOrderActionField ToActionField(
        const CME_FIX_NAMESPACE::ExecutionReport& report);
  CThostFtdcRspInfoField ToRspField(
        const CME_FIX_NAMESPACE::ExecutionReport& report);
  CThostFtdcRspInfoField ToRspField(
        const CME_FIX_NAMESPACE::OrderCancelReject& report);

  CFixFtdcTraderSpi *trader_spi_;
  char fix_config_path_[64];
  int last_msg_seq_num_;
  char user_id_[64];
  char user_passwd_[64];
  char acc_session_id_[8];
  char firm_id_[8];
  char sender_sub_id_[32];
  char target_sub_id_[32];
  char sender_loc_id_[32];
  char self_match_prev_id_[32];

  FIX::SessionSettings *settings_;
  FIX::FileStoreFactory *store_factory_;
  FIX::ScreenLogFactory *log_factory_;
  FIX::SocketInitiator *initiator_;
  FIX::SessionID session_id_;
  AuditTrail audit_trail_;
  SequenceSerialization seq_serial_;
  OrderPool order_pool_;
  std::fstream log_file_;
};

const char *time_now();
int date_now();

/* ===============================================
   =============== IMPLEMENTATION ================ */

using namespace std;

AuditLog::AuditLog() {
  ClearElement("sending_timestamps");
  ClearElement("receiving_timestamps");
  ClearElement("message_direction");
  ClearElement("operator_id");
  ClearElement("self_match_prevention_id");
  ClearElement("account_number");
  ClearElement("session_id");
  ClearElement("executing_firm_id");
  ClearElement("manual_order_identifier");
  ClearElement("message_type");
  ClearElement("customer_type_indicator");
  ClearElement("origin");
  ClearElement("cme_globex_message_id");
  ClearElement("message_link_id");
  ClearElement("order_flow_id");
  ClearElement("spread_leg_link_id");
  ClearElement("instrument_description");
  ClearElement("market_segment_id");
  ClearElement("client_order_id");
  ClearElement("cme_globex_order_id");
  ClearElement("buy_sell_indicator");
  ClearElement("quantity");
  ClearElement("limit_price");
  ClearElement("stop_price");
  ClearElement("order_type");
  ClearElement("order_qualifier");
  ClearElement("ifm_flag");
  ClearElement("display_quantity");
  ClearElement("minimum_quantity");
  ClearElement("country_of_origin");
  ClearElement("fill_price");
  ClearElement("fill_quantity");
  ClearElement("cumulative_quantity");
  ClearElement("remaining_quantity");
  ClearElement("aggressor_flag");
  ClearElement("source_of_cancellation");
  ClearElement("reject_reason");
  ClearElement("processed_quotes");
  ClearElement("cross_id");
  ClearElement("quote_request_id");
  ClearElement("message_quote_id");
  ClearElement("quote_entry_id");
  ClearElement("bid_price");
  ClearElement("bid_size");
  ClearElement("offer_price");
  ClearElement("offer_size");
}

AuditLog::~AuditLog() {
  // TODO
}

void AuditLog::ClearElement(string key) {
  map<string, string>::iterator iter = elements.find(key);
  if (iter == elements.end()) {
    elements.insert(pair<string, string>(key, ""));
  } else {
    iter->second = "";
  }
}

int AuditLog::WriteElement(string key, string value) {
  map<string, string>::iterator iter = elements.find(key);
  if (iter == elements.end()) {
    printf("Key not found: %s\n", key.c_str());
    return 1;
  } else {
    iter->second = value;
    return 0;
  }
}

int AuditLog::WriteElement(string key, const char *value) {
  map<string, string>::iterator iter = elements.find(key);
  if (iter == elements.end()) {
    printf("Key not found: %s\n", key.c_str());
    return 1;
  } else {
    iter->second = string(value);
    return 0;
  }
}

int AuditLog::WriteElement(string key, int value) {
  map<string, string>::iterator iter = elements.find(key);
  if (iter == elements.end()) {
    printf("Key not found: %s\n", key.c_str());
    return 1;
  } else {
    char value_str[16];
    snprintf(value_str, sizeof(value_str), "%d", value);
    iter->second = string(value_str);
    return 0;
  }
}

int AuditLog::WriteElement(string key, double value) {
  map<string, string>::iterator iter = elements.find(key);
  if (iter == elements.end()) {
    printf("Key not found: %s\n", key.c_str());
    return 1;
  } else {
    char value_str[32];
    snprintf(value_str, sizeof(value_str), "%.3lf", value);
    iter->second = string(value_str);
    return 0;
  }
}

int AuditLog::WriteElement(string key, char value) {
  map<string, string>::iterator iter = elements.find(key);
  if (iter == elements.end()) {
    printf("Key not found: %s\n", key.c_str());
    return 1;
  } else {
    char value_str[16];
    snprintf(value_str, sizeof(value_str), "%c", value);
    iter->second = string(value_str);
    return 0;
  }
}

string AuditLog::ToString() {
  string buffer = elements["sending_timestamps"] + ","
                + elements["receiving_timestamps"] + ","
                + elements["message_direction"] + ","
                + elements["operator_id"] + ","
                + elements["self_match_prevention_id"] + ","
                + elements["account_number"] + ","
                + elements["session_id"] + ","
                + elements["executing_firm_id"] + ","
                + elements["manual_order_identifier"] + ","
                + elements["message_type"] + ","
                + elements["customer_type_indicator"] + ","
                + elements["origin"] + ","
                + elements["cme_globex_message_id"] + ","
                + elements["message_link_id"] + ","
                + elements["order_flow_id"] + ","
                + elements["spread_leg_link_id"] + ","
                + elements["instrument_description"] + ","
                + elements["market_segment_id"] + ","
                + elements["client_order_id"] + ","
                + elements["cme_globex_order_id"] + ","
                + elements["buy_sell_indicator"] + ","
                + elements["quantity"] + ","
                + elements["limit_price"] + ","
                + elements["stop_price"] + ","
                + elements["order_type"] + ","
                + elements["order_qualifier"] + ","
                + elements["ifm_flag"] + ","
                + elements["display_quantity"] + ","
                + elements["minimum_quantity"] + ","
                + elements["country_of_origin"] + ","
                + elements["fill_price"] + ","
                + elements["fill_quantity"] + ","
                + elements["cumulative_quantity"] + ","
                + elements["remaining_quantity"] + ","
                + elements["aggressor_flag"] + ","
                + elements["source_of_cancellation"] + ","
                + elements["reject_reason"] + ","
                + elements["processed_quotes"] + ","
                + elements["cross_id"] + ","
                + elements["quote_request_id"] + ","
                + elements["message_quote_id"] + ","
                + elements["quote_entry_id"] + ","
                + elements["bid_price"] + ","
                + elements["bid_size"] + ","
                + elements["offer_price"] + ","
                + elements["offer_size"];
  return buffer;
}

AuditTrail::AuditTrail() {
  // TODO
}

AuditTrail::~AuditTrail() {
  // TODO
}

int AuditTrail::Init(const char *log_file_name) {
  // TODO
  log_file_.open(log_file_name, fstream::out | fstream::app);
  if (log_file_.good()) {
  } else {
    printf("Can not open file!\n");
  }
  return 0;
}

void AuditTrail::WriteLog(AuditLog& log) {
  if (log_file_.good()) {
    log_file_ << log.ToString() << endl;
    log_file_.flush();
  } else {
    printf("Write File Error!\n");
  }
}


SequenceSerialization::SequenceSerialization() : prefix_{0} {
  // TODO
  // int ret = pthread_mutex_init(&trans_lock_, NULL);
  // if (ret != 0) {
  //   printf("Failed to initialize transaction mutex lock\n");
  // } else {
  //   printf("Initialize transaction mutex lock sucessfully\n");
  // }
}

SequenceSerialization::~SequenceSerialization() {
  // TODO
}

void SequenceSerialization::SetPrefix(const char *prefix) {
  snprintf(prefix_, sizeof(prefix_), "%s", prefix);
}

int SequenceSerialization::Init() {
  date_ = date_now();
  char dump_file_name[64];
  snprintf(dump_file_name, sizeof(dump_file_name), "%s_%d.seq", prefix_, date_);
  dump_file_.open(dump_file_name, fstream::in | fstream::out | fstream::app);
  if (!dump_file_.good()) {
    printf("Can't open file %s\n", dump_file_name);
    file_good_ = false;
    return 1;
  }
  printf("open file sucessfully\n");
  file_good_ = true;
  // content of file is like 'OrderID:FlowID:TransID'
  // example:               '00000012:00001002:00000333'
  /*char c = */dump_file_.get();
  if (dump_file_.eof()) {
    dump_file_.close();
    dump_file_.open(dump_file_name, fstream::in | fstream::out);
    cur_order_id_ = cur_flow_id_ = cur_trans_id_ = 0;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%08d:%08d:%08d", cur_order_id_,
        cur_flow_id_, cur_trans_id_);
    // dump_file_.clear();  // clear() to transfer read mode to write mode
    dump_file_.seekp(0);
    dump_file_.write(buffer, 26);
    dump_file_.flush();
  } else {
    // dump_file_.clear();
    dump_file_.seekg(0);
    char order_id_buf[16], flow_id_buf[16], trans_id_buf[16];
    dump_file_.read(order_id_buf, 9);  // read to ':'
    order_id_buf[8] = '\0';  // rewrite ':' to '\0'
    dump_file_.read(flow_id_buf, 9);
    flow_id_buf[8] = '\0';
    dump_file_.read(trans_id_buf, 8);
    trans_id_buf[8] = '\0';

    cur_order_id_ = atoi(order_id_buf);
    cur_flow_id_ = atoi(flow_id_buf);
    cur_trans_id_ = atoi(trans_id_buf);
    printf("order:%d flow:%d trans:%d\n", cur_order_id_, cur_flow_id_, cur_trans_id_);
    // cur_order_id_++;
    // cur_flow_id_++;
    // cur_trans_id_++;

    dump_file_.close();
    dump_file_.open(dump_file_name, fstream::in | fstream::out);
  }
  return 0;
}

void SequenceSerialization::DumpOrderID(int order_id) {
  if (!file_good_) {
    printf("Invalid file stream\n");
    return;
  }
  if (order_id != cur_order_id_ + 1) {
    printf("WARNING: OrderID Gap Detected %d-->%d\n", cur_order_id_, order_id);
  }
  cur_order_id_ = order_id;
  char order_id_buf[16];
  snprintf(order_id_buf, sizeof(order_id_buf), "%08d", cur_order_id_);
  dump_file_.seekp(0);
  dump_file_.write(order_id_buf, 8);
  dump_file_.flush();
}

int SequenceSerialization::GetCurOrderID() {
  return cur_order_id_;
}

void SequenceSerialization::DumpFlowID() {
  if (!file_good_) {
    printf("Invalid file stream\n");
    return;
  }
  char flow_id_buf[16];
  snprintf(flow_id_buf, sizeof(flow_id_buf), "%08d", cur_flow_id_);
  dump_file_.seekp(9);
  dump_file_.write(flow_id_buf, 8);
  dump_file_.flush();
}

int SequenceSerialization::GetCurFlowID() {
  return cur_flow_id_;
}

const char *SequenceSerialization::GenFlowIDStr(int flow_id) {
  static char flow_id_buf[32];
  cur_flow_id_ = flow_id;
  snprintf(flow_id_buf, sizeof(flow_id_buf), "%s%08d", prefix_, flow_id);
  DumpFlowID();
  return flow_id_buf;
}


CFixFtdcTraderApi *CFixFtdcTraderApi::CreateFtdcTraderApi(const char *configPath) {
  ImplFixFtdcTraderApi *api = new ImplFixFtdcTraderApi(configPath);
  return (CFixFtdcTraderApi *)api;
}

ImplFixFtdcTraderApi::InputOrder::InputOrder() : order_flow_id{0} {
  memset(&basic_order, 0, sizeof(basic_order));
}

ImplFixFtdcTraderApi::InputOrder::InputOrder(
      CThostFtdcInputOrderField *pInputOrder) : order_flow_id{0} {
  memcpy(&basic_order, pInputOrder, sizeof(basic_order));
}

ImplFixFtdcTraderApi::OrderPool::OrderPool() {
  memset(order_pool_, sizeof(order_pool_), 0);
}

ImplFixFtdcTraderApi::InputOrder *ImplFixFtdcTraderApi::OrderPool::get(
      int order_id) {
  if (order_id >= 0 && order_id < MAX_ORDER_SIZE) {
    return order_pool_[order_id];
  } else {
    return (ImplFixFtdcTraderApi::InputOrder *)NULL;
  }
}

ImplFixFtdcTraderApi::InputOrder *ImplFixFtdcTraderApi::OrderPool::get(
      string sys_id) {
  map<string, int>::iterator it = sys_to_local_.find(sys_id);
  if (it == sys_to_local_.end()) {
    printf("SysOrderID-%s not found!\n", sys_id.c_str());
    return (ImplFixFtdcTraderApi::InputOrder *)NULL;
  } else {
    return get(it->second);
  }
}

ImplFixFtdcTraderApi::InputOrder *ImplFixFtdcTraderApi::OrderPool::add(
      CThostFtdcInputOrderField *pInputOrder) {
  ImplFixFtdcTraderApi::InputOrder *input_order = new
      ImplFixFtdcTraderApi::InputOrder(pInputOrder);
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

void ImplFixFtdcTraderApi::OrderPool::add_pair(string sys_id, int local_id) {
  map<string, int>::iterator it = sys_to_local_.find(sys_id);
  if (it != sys_to_local_.end()) {
    printf("SysOrderID-%s already match for %d !\n", sys_id.c_str(),
           it->second);
  } else {
    sys_to_local_.insert(std::pair<std::string, int>(sys_id, local_id));
  }
}

ImplFixFtdcTraderApi::ImplFixFtdcTraderApi(const char *configPath) :
    last_msg_seq_num_(0), acc_session_id_{0}, firm_id_{0}, sender_sub_id_{0},
    target_sub_id_{0}, sender_loc_id_{0}, self_match_prev_id_{0} {
  snprintf(fix_config_path_, sizeof(fix_config_path_), "%s", configPath);
#ifndef __DEBUG__
  log_file_.open("./fix.log", fstream::out | fstream::app);
#endif
}

ImplFixFtdcTraderApi::~ImplFixFtdcTraderApi() {
  // TODO
}

void ImplFixFtdcTraderApi::Init() {
  settings_ = new FIX::SessionSettings(fix_config_path_);
  store_factory_ = new FIX::FileStoreFactory(*settings_);
  log_factory_ = new FIX::ScreenLogFactory(*settings_);
  initiator_ = new FIX::SocketInitiator(*this, *store_factory_, 
        *settings_, *log_factory_);
  trader_spi_->OnFrontConnected();
}

int ImplFixFtdcTraderApi::Join() {
  // dummy function to adapt CTP API
  return 0;
}

void ImplFixFtdcTraderApi::RegisterFront(char *pszFrontAddress) {
  // TODO
}


void ImplFixFtdcTraderApi::RegisterSpi(CFixFtdcTraderSpi *pSpi) {
  trader_spi_ = pSpi;
}

void ImplFixFtdcTraderApi::SubscribePrivateTopic(
      THOST_TE_RESUME_TYPE nResumeType) {
  // dummy function to adapt CTP API
}

void ImplFixFtdcTraderApi::SubscribePublicTopic(
      THOST_TE_RESUME_TYPE nResumeType) {
  // dummy function to adapt CTP API
}

int ImplFixFtdcTraderApi::ReqUserLogin(
      CThostFtdcReqUserLoginField *pReqUserLoginField, int nRequestID) {
  // save password and used for the real Login Request in toAdmin
  snprintf(user_id_, sizeof(user_id_), "%s", pReqUserLoginField->UserID);
  snprintf(user_passwd_, sizeof(user_passwd_), "%s", pReqUserLoginField->Password);
  // CThostFtdcRspInfoField info_field;
  // memset(&info_field, 0, sizeof(info_field));
  initiator_->start();
  return 0;
}

int ImplFixFtdcTraderApi::ReqUserLogout(
      CThostFtdcUserLogoutField *pUserLogout, int nRequestID) {
  initiator_->stop();
  return 0;
}

int ImplFixFtdcTraderApi::ReqOrderInsert(
      CThostFtdcInputOrderField *pInputOrder, int nRequestID) {
  InputOrder *input_order = order_pool_.add(pInputOrder);
  // real order-ID equal OrderRef/MAX_STRATEGY_NUM(100)
  // int order_id = atoi(pInputOrder->OrderRef) / 100;
  int order_id = atoi(pInputOrder->OrderRef);
  seq_serial_.DumpOrderID(order_id);
  int no_stid = order_id / 100;
  string order_flow_id = seq_serial_.GenFlowIDStr(no_stid);
  snprintf(input_order->order_flow_id, sizeof(input_order->order_flow_id),
         "%s", order_flow_id.c_str());
  FIX::OrdType order_type = FIX::OrdType_LIMIT;
  if (pInputOrder->OrderPriceType == THOST_FTDC_OPT_AnyPrice) {
    order_type = FIX::OrdType_MARKET;
  }
  FIX::HandlInst handl_inst('1');
  // FIX::ClOrdID cl_order_id(pInputOrder->UserOrderLocalID);
  FIX::ClOrdID cl_order_id(pInputOrder->OrderRef);
  char inst_group[8] = {0};
  strncpy(inst_group, pInputOrder->InstrumentID, 2);
  FIX::Symbol symbol(inst_group);
  FIX::Side side;
  if (pInputOrder->Direction == THOST_FTDC_D_Buy) {
    side = FIX::Side_BUY;
  } else {
    side = FIX::Side_SELL;
  }
  string timestamp(time_now());
  FIX::TransactTime transact_time(timestamp.c_str());
  CME_FIX_NAMESPACE::NewOrderSingle new_order(cl_order_id, handl_inst, symbol,
                                            side, transact_time, order_type);
  FIX::Price limit_price(pInputOrder->LimitPrice);
  FIX::Account account(pInputOrder->InvestorID);
  FIX::OrderQty order_qty(pInputOrder->VolumeTotalOriginal);
  FIX::TimeInForce time_in_force = FIX::TimeInForce_DAY;
  FIX::SecurityDesc security_desc(pInputOrder->InstrumentID);
  if (order_type == FIX::OrdType_LIMIT) {
    new_order.set(limit_price);
  }
  new_order.set(account);
  new_order.set(order_qty);
  new_order.set(time_in_force);
  new_order.set(security_desc);

  // 1028-ManualOrderIndicator : Y=manual N=antomated
  // new_order.setField(1028, "Y");
  new_order.setField(1028, "N");

  // SecurityType : FUT=Future
  //                OPT=Option
  // FIX::SecurityType security_type("FUT");
  // new_order.set(security_type);
  new_order.setField(FIX::FIELD::SecurityType, "FUT");

  // CustomerOrFirm : 0=Customer
  //                  1=Firm
  FIX::CustomerOrFirm customer_or_firm(1);
  new_order.set(customer_or_firm);
  // new_order.setField(FIX::FIELD::CustomerOrFirm, "1");

  // 9702-CtiCode : 1=CTI1 2=CTI2 3=CTI3 4=CTI4
  new_order.setField(9702, "2");

  new_order.setField(7928, self_match_prev_id_);  // SelfMatchPreventionID
  new_order.setField(8000, "O");  // SelfMatchPreventionInstruction

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
  audit_log.WriteElement("limit_price", limit_price.getValue());
  audit_log.WriteElement("order_type", order_type.getValue());
  audit_log.WriteElement("order_qualifier", time_in_force);
  // audit_log.WriteElement("ifm_flag", "N");
  // audit_log.WriteElement("display_quantity", order_qty.getValue());
  // audit_log.WriteElement("minimum_quantity", 0);
  audit_log.WriteElement("country_of_origin", sender_loc_id_);
  // audit_log.WriteElement("")
  audit_trail_.WriteLog(audit_log);

  // cout << "ReqOrderInsert" << pInputOrder->UserOrderLocalID << endl;
  cout << "ReqOrderInsert" << pInputOrder->OrderRef << endl;
  FIX::Session::sendToTarget(new_order, session_id_);

  return 0;
}

int ImplFixFtdcTraderApi::ReqOrderAction(
      CThostFtdcInputOrderActionField *pInputOrderAction, int nRequestID) {
  // real order-ID equal to OrderRef / MAX_STRATEGY_NUM(100)
  // int action_id = pInputOrderAction->OrderActionRef / 100;
  int action_id = pInputOrderAction->OrderActionRef;
  seq_serial_.DumpOrderID(action_id);
  int local_order_ref = strtol(pInputOrderAction->OrderRef, NULL, 10) / 100;
  InputOrder *input_order = order_pool_.get(local_order_ref);
  string order_flow_id;
  if (input_order != NULL) {
    order_flow_id = input_order->order_flow_id;
  } else {
    printf("Order-%s not found!\n", pInputOrderAction->OrderRef);
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%s%08d", user_id_, local_order_ref);
    order_flow_id = string(buffer);
  }
  char cl_order_id_str[32], orig_order_id_str[32];
  snprintf(cl_order_id_str, sizeof(cl_order_id_str), "%d",
           pInputOrderAction->OrderActionRef);
  snprintf(orig_order_id_str, sizeof(orig_order_id_str), "%s",
           pInputOrderAction->OrderRef);
  FIX::ClOrdID cl_order_id(cl_order_id_str);
  FIX::OrigClOrdID orig_cl_order_id(pInputOrderAction->OrderRef);
  FIX::Side side;  // side is necessary!!
  if (pInputOrderAction->ExchangeID[0] == THOST_FTDC_D_Buy) {
    side = FIX::Side_BUY;
  } else {
    side = FIX::Side_SELL;
  }
  char inst_group[8] = {0};
  strncpy(inst_group, pInputOrderAction->InstrumentID, 2);
  FIX::Symbol symbol(inst_group);
  string timestamp(time_now());
  FIX::TransactTime transact_time(timestamp.c_str());
  CME_FIX_NAMESPACE::OrderCancelRequest cancel_order(orig_cl_order_id,
                                                     cl_order_id,
                                                     symbol,
                                                     side,
                                                     transact_time);
  FIX::Account account(pInputOrderAction->InvestorID);
  FIX::OrderID order_id(pInputOrderAction->OrderSysID);
  FIX::SecurityDesc security_desc(pInputOrderAction->InstrumentID);
  cancel_order.set(account);
  cancel_order.set(order_id);
  cancel_order.set(security_desc);

  // 1028-ManualOrderIndicator : Y=manual N=antomated
  cancel_order.setField(1028, "N");

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

  cout << "ReqOrderAction:" << orig_order_id_str 
       << "|" << cl_order_id_str
       << "|" << pInputOrderAction->OrderSysID << endl;
  FIX::Session::sendToTarget(cancel_order, session_id_);

  return 0;
}

int ImplFixFtdcTraderApi::ReqMassOrderAction(
      CThostFtdcInputOrderActionField *pInputOrderAction, int nRequestID) {
  FIX::Message mass_order_cancel;
  // int action_id = pInputOrderAction->OrderActionRef / 100;
  int action_id = pInputOrderAction->OrderActionRef;
  seq_serial_.DumpOrderID(action_id);
  char cl_order_id_str[32];
  snprintf(cl_order_id_str, sizeof(cl_order_id_str), "%d",
           pInputOrderAction->OrderActionRef);
  FIX::ClOrdID cl_order_id(cl_order_id_str);
  FIX::SecurityDesc security_desc(pInputOrderAction->InstrumentID);
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

  cout << "ReqMassOrderAction: " << pInputOrderAction->InstrumentID << endl;
  FIX::Session::sendToTarget(mass_order_cancel, session_id_);
  return 0;
}

void ImplFixFtdcTraderApi::onCreate(const FIX::SessionID& sessionID) {
  session_id_ = sessionID;
  string str_sender_comp_id = sessionID.getSenderCompID().getValue();
  string str_target_comp_id = sessionID.getTargetCompID().getValue();
  string acc_session_id = str_sender_comp_id.substr(0, 3);
  string firm_id = str_sender_comp_id.substr(3, 3);
  snprintf(acc_session_id_, sizeof(acc_session_id_), "%s",
           acc_session_id.c_str());
  snprintf(firm_id_, sizeof(firm_id_), "%s", firm_id.c_str());

  const FIX::Dictionary& dict = settings_->get(sessionID);
  string target_sub_id = dict.getString("TARGETSUBID");
  string sender_sub_id = dict.getString("SENDERSUBID");
  string sender_loc_id = dict.getString("SENDERLOCATIONID");
  string self_match_prev_id = dict.getString("SELFMATCHPREVENTIONID");
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

  cout << "Session created : " << session_id_ << endl;
}

void ImplFixFtdcTraderApi::onLogon(const FIX::SessionID& sessionID) {
  cout << "Logon - " << sessionID << endl;
  // trader_spi_->OnFrontConnected();
  CThostFtdcRspUserLoginField login_field;
  memset(&login_field, 0, sizeof(login_field));
  // return Max Local Order ID should be equal to 
  // Real Order ID * MAX_STRATEGY_NUM(100)
  // int max_order_id = seq_serial_.GetCurOrderID() * 100;
  int max_order_id = seq_serial_.GetCurOrderID();
  snprintf(login_field.MaxOrderRef, sizeof(login_field.MaxOrderRef), "%d",
           max_order_id);
  trader_spi_->OnRspUserLogin(&login_field, NULL, 0, true);
}

void ImplFixFtdcTraderApi::onLogout(const FIX::SessionID& sessionID) {
  cout << "Logout - " << sessionID << endl;
  CThostFtdcUserLogoutField logout_field;
  memset(&logout_field, 0, sizeof(logout_field));
  CThostFtdcRspInfoField info_field;
  memset(&info_field, 0, sizeof(info_field));
  trader_spi_->OnRspUserLogout(&logout_field, &info_field, 0, true);
  // trader_spi_->OnFrontDisconnected(1001);
}

void ImplFixFtdcTraderApi::toAdmin(FIX::Message& message,
      const FIX::SessionID& sessionID) {
  FIX::MsgType msg_type;
  message.getHeader().getField(msg_type);
  FillFixHeader(message);
  if (msg_type == FIX::MsgType_Logon) {
    FillLogonRequest(message);
  } else if (msg_type == FIX::MsgType_Heartbeat) {
    FillHeartbeat(message);
  } else if (msg_type == FIX::MsgType_ResendRequest) {
    FillResendRequest(message);
  } else if (msg_type == FIX::MsgType_Reject) {
    FillRejectRequest(message);
  } else if (msg_type == FIX::MsgType_Logout) {
    FillLogoutRequest(message);
  }

#ifdef __DEBUG__
  cout << "[" << time_now() << "]TO ADMIN XML: " << message.toXML() << endl;
#else
  log_file_ << "[" << time_now() << "]TO ADMIN XML: " << message.toXML() << endl;
#endif
}

void ImplFixFtdcTraderApi::toApp(FIX::Message& message, 
      const FIX::SessionID& sessionID)
    throw(FIX::DoNotSend) {
  try {
    FIX::PossDupFlag possDupFlag;
    message.getHeader().getField(possDupFlag);
    if (possDupFlag) {
      throw FIX::DoNotSend();
    }
  } catch (FIX::FieldNotFound&) {}

  FillFixHeader(message);
#ifdef __DEBUG__
  cout << "[" << time_now() << "]TO APP XML: " << message.toXML() << endl;
#else
  log_file_ << "[" << time_now() << "]TO APP XML: " << message.toXML() << endl;
#endif
}

void ImplFixFtdcTraderApi::fromAdmin(const FIX::Message& message,
      const FIX::SessionID& sessionID) 
    throw(FIX::FieldNotFound, FIX::IncorrectDataFormat,
          FIX::IncorrectTagValue, FIX::RejectLogon) {
  FIX::MsgType msg_type;
  message.getHeader().getField(msg_type);
  if (msg_type == FIX::MsgType_Logout) {
    FIX::LastMsgSeqNumProcessed last_msg_seq_num;
    message.getHeader().getField(last_msg_seq_num);
    int start_seq_num = last_msg_seq_num.getValue() + 1;
    // last_msg_seq_num_.setValue(start_seq_num);
    last_msg_seq_num_ = start_seq_num;
    // cout << start_seq_num << "::" << last_msg_seq_num_;
  }
  crack(message, sessionID);
#ifdef __DEBUG__
  cout << "[" << time_now() << "]FROM ADMIN XML: " << message.toXML() << endl;
#else
  log_file_ << "[" << time_now() << "]FROM ADMIN XML: " << message.toXML() << endl;
#endif
}

void ImplFixFtdcTraderApi::fromApp(const FIX::Message& message,
      const FIX::SessionID& sessionID)
    throw(FIX::FieldNotFound, FIX::IncorrectDataFormat,
          FIX::IncorrectTagValue, FIX::UnsupportedMessageType) {
  FIX::MsgType msg_type;
  message.getHeader().getField(msg_type);
  if (msg_type == FIX::MsgType_XMLnonFIX) {
    onXmlNonFix(message, sessionID);
    return;
  } else if (msg_type == FIX::MsgType_OrderMassActionReport) {
    onMassActionReport(message, sessionID);
    return;
  }

  crack(message, sessionID);
#ifdef __DEBUG__
  cout << "[" << time_now() << "]FROM APP XML: " << message.toXML() << endl;
#else
  log_file_ << "[" << time_now() << "]FROM APP XML: " << message.toXML() << endl;
#endif
}

void ImplFixFtdcTraderApi::onXmlNonFix(const FIX::Message& message,
      const FIX::SessionID& sessionID) {
  string xml_data = message.getHeader().getField(213);
  // strip header and tailor of <RTRF> </RTRF>
  string message_data = xml_data.substr(6, xml_data.size() - 6 - 7);
  for (size_t i = 0; i < message_data.size(); i++) {
    if (message_data[i] == '\002') {
      message_data[i] = '\001';
    }
  } // decode xml data
  FIX::Message report(message_data, false);
  FIX::MsgType msg_type;
  report.getHeader().getField(msg_type);
  if (msg_type == FIX::MsgType_ExecutionReport) {
    CME_FIX_NAMESPACE::ExecutionReport exe_report(report);
    onMessage(exe_report, sessionID);
  } else if (msg_type == FIX::MsgType_OrderMassActionReport) {
    onMassActionReport(report, sessionID);
  }
}

void ImplFixFtdcTraderApi::onMassActionReport(const FIX::Message& message,
      const FIX::SessionID& sessionID) {
  FIX::ClOrdID cl_order_id;
  message.getField(cl_order_id);
  string report_id = message.getField(1369);
  string action_type = message.getField(1373);
  string action_scope = message.getField(1374);
  string action_response = message.getField(1375);
  string affected_orders = message.getField(533);
  string last_fragment = message.getField(893);
  int total_affected = atoi(message.getField(534).c_str());
  if (total_affected > 0) {
    CThostFtdcOrderField order_field;
    memset(&order_field, 0, sizeof(order_field));

    FIX::Account account;
    if (message.getFieldIfSet(account)) {
      string str_account = account.getValue();
      snprintf(order_field.InvestorID, sizeof(order_field.InvestorID),
               "%s", str_account.c_str());
    }

    FIX::OrigClOrdID orig_cl_ord_id;
    message.getField(orig_cl_ord_id);
    string str_cl_ord_id = orig_cl_ord_id.getValue();
    snprintf(order_field.OrderRef, sizeof(order_field.OrderRef),
             "%s", str_cl_ord_id.c_str());

    if (message.isSetField(535)) {
      string str_order_id = message.getField(535);
      snprintf(order_field.OrderSysID, sizeof(order_field.OrderSysID),
               "%s", str_order_id.c_str());
    }

    // FIX::OrderQty order_qty;
    // report.getField(order_qty);
    // int int_order_qty = order_qty.getValue();
    // order_field.VolumeTotalOriginal = int_order_qty;

    order_field.OrderStatus = THOST_FTDC_OST_Canceled;

    FIX::SecurityDesc security_desc;
    if (message.getFieldIfSet(security_desc)) {
      string str_security_desc = security_desc.getValue();
      snprintf(order_field.InstrumentID, sizeof(order_field.InstrumentID),
               "%s", str_security_desc.c_str());
    }

    trader_spi_->OnRtnOrder(&order_field);
  }
}

void ImplFixFtdcTraderApi::onMessage(
      const CME_FIX_NAMESPACE::Logon& logon,
      const FIX::SessionID& sessionID) {
  cout << "onMessage - Logon" << endl;
  // FIX::TargetCompID target_comp_id;
  // string str_sender_comp_id = sessionID.getSenderCompID().getValue();
  // string str_target_comp_id = sessionID.getTargetCompID().getValue();
  // string acc_session_id = str_sender_comp_id.substr(0, 3);
  // string firm_id = str_sender_comp_id.substr(3, 3);
  // snprintf(acc_session_id_, sizeof(acc_session_id_), acc_session_id.c_str());
  // snprintf(firm_id_, sizeof(firm_id_), firm_id.c_str());

  FIX::TargetSubID target_sub_id;

  logon.getHeader().getField(target_sub_id);

  string str_target_sub_id = target_sub_id.getValue();
  snprintf(sender_sub_id_, sizeof(sender_sub_id_), "%s", str_target_sub_id.c_str());
}

void ImplFixFtdcTraderApi::onMessage(
      const CME_FIX_NAMESPACE::ExecutionReport& report,
      const FIX::SessionID& sessionID) {
  FIX::ExecType exec_type;
  report.getField(exec_type);
  cout << "onMessage<ExecutionReport>- " << exec_type << endl;
  switch (exec_type) {
    case FIX::ExecType_NEW: {
      CThostFtdcOrderField order_field = ToOrderField(report);
      int local_id = strtol(order_field.OrderRef, NULL, 10)/100;
      string sys_id(order_field.OrderSysID);
      order_pool_.add_pair(sys_id, local_id);
      LogAuditTrail(report);
      trader_spi_->OnRtnOrder(&order_field);  // OnRspOrderInsert
    }
    break;
    case  FIX::ExecType_PARTIAL_FILL: {
      CThostFtdcTradeField trade_field = ToTradeField(report);
      LogAuditTrail(report);
      trader_spi_->OnRtnTrade(&trade_field);   // OnRtnTrade
    }
    break;
    case FIX::ExecType_FILL: {
      CThostFtdcTradeField trade_field = ToTradeField(report);
      CThostFtdcOrderField order_field = ToOrderField(report);
      LogAuditTrail(report);
      trader_spi_->OnRtnTrade(&trade_field);
      trader_spi_->OnRtnOrder(&order_field);
      // OnRtnOrder if necessary
    }
    break;
    case FIX::ExecType_CANCELED: {
      CThostFtdcOrderField order_field = ToOrderField(report);
      LogAuditTrail(report);
      trader_spi_->OnRtnOrder(&order_field);  // OnRtnOrder
    }
    break;
    case FIX::ExecType_REPLACE: {
      // OnRtnOrder of modified
      LogAuditTrail(report);
    }
    break;
    case FIX::ExecType_REJECTED: {
      CThostFtdcInputOrderField order_field = ToInputOrderField(report);
      CThostFtdcRspInfoField rsp_field = ToRspField(report);
      LogAuditTrail(report);
      trader_spi_->OnRspOrderInsert(&order_field, &rsp_field, 0, true);      
    }
      break;
    default: {
      LogAuditTrail(report);
    }
      break;
  }  // switch exec_type

}

void ImplFixFtdcTraderApi::onMessage(
      const CME_FIX_NAMESPACE::OrderCancelReject& report,
      const FIX::SessionID& sessionID) {
  CThostFtdcInputOrderActionField action_field = ToActionField(report);
  CThostFtdcRspInfoField rsp_field = ToRspField(report);
  LogAuditTrail(report);
  trader_spi_->OnRspOrderAction(&action_field, &rsp_field, 0, true);
}

void ImplFixFtdcTraderApi::onMessage(
      const CME_FIX_NAMESPACE::Reject& reject,
      const FIX::SessionID& sessionID) {
  LogAuditTrail(reject);
}

void ImplFixFtdcTraderApi::onMessage(
      const CME_FIX_NAMESPACE::BusinessMessageReject& reject,
      const FIX::SessionID& sessionID) {
  LogAuditTrail(reject);
}

void ImplFixFtdcTraderApi::LogAuditTrail(
      const CME_FIX_NAMESPACE::ExecutionReport& report) {
  FIX::OrdStatus ord_status;
  report.getField(ord_status);

  AuditLog audit_log;

  if (ord_status == FIX::OrdStatus_PARTIALLY_FILLED ||
             ord_status == FIX::OrdStatus_FILLED) {
    FIX::LastPx last_px;
    FIX::LastQty last_qty;
    FIX::AggressorIndicator aggressor_indicator;
    report.getField(last_px);
    report.getField(last_qty);
    report.getField(aggressor_indicator);
    audit_log.WriteElement("fill_price", last_px.getValue());
    audit_log.WriteElement("fill_quantity", (int)last_qty.getValue());
    if (aggressor_indicator.getValue()) {
      audit_log.WriteElement("aggressor_flag", "Y");
    } else {
      audit_log.WriteElement("aggressor_flag", "N");
    }
  } else if (ord_status == FIX::OrdStatus_REJECTED) {
    FIX::OrdRejReason ord_rej_reason;
    report.getField(ord_rej_reason);
    audit_log.WriteElement("reject_reason", ord_rej_reason.getValue());
  }

  FIX::MsgType msg_type;
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
  
  report.getHeader().getField(msg_type);
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
    // string self_match_prevention_id = report.getField(7928);
    if (report.isSetField(7928)) {
      string self_match_prevention_id = report.getField(7928);
      audit_log.WriteElement("self_match_prevention_id", 
          self_match_prevention_id);
    }
  }


  string message_type = string(msg_type.getValue()) + "/" +
                        ord_status.getValue();
  InputOrder *input_order = order_pool_.get(order_id.getValue());
  string order_flow_id = "";
  if (input_order != NULL) {
    order_flow_id = input_order->order_flow_id;
  } else {
    printf("InputOrder [SysID-%s] not found!\n", order_id.getValue().c_str());
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
  // audit_log.WriteElement("country_of_origin", sender_loc_id_);
  audit_log.WriteElement("cumulative_quantity", (int)cum_qty.getValue());
  audit_log.WriteElement("remaining_quantity", (int)leaves_qty.getValue());
  audit_log.WriteElement("manual_order_identifier", manual_order_identifier);
  // audit_log.WriteElement("")
  audit_trail_.WriteLog(audit_log);

}

void ImplFixFtdcTraderApi::LogAuditTrail(
      const CME_FIX_NAMESPACE::OrderCancelReject& report) {
  FIX::MsgType msg_type;
  FIX::TargetCompID target_comp_id;
  FIX::TargetSubID target_sub_id;
  FIX::SenderSubID sender_sub_id;
  FIX::ClOrdID cl_ord_id;
  FIX::ExecID exec_id;
  FIX::OrderID order_id;
  FIX::OrdStatus ord_status;
  FIX::OrigClOrdID orig_cl_order_id;
  FIX::TransactTime transact_time;
  FIX::CxlRejReason cxl_rej_reason;
  FIX::CxlRejResponseTo cxl_rej_response_to;
  
  report.getHeader().getField(msg_type);
  report.getHeader().getField(target_comp_id);
  report.getHeader().getField(target_sub_id);
  report.getHeader().getField(sender_sub_id);
  report.getField(cl_ord_id);
  report.getField(exec_id);
  report.getField(order_id);
  report.getField(ord_status);
  report.getField(orig_cl_order_id);
  report.getField(transact_time);
  report.getField(cxl_rej_reason);
  report.getField(cxl_rej_response_to);
  string manual_order_identifier = report.getField(1028);
  // string self_match_prevention_id = report.getField(7928);

  AuditLog audit_log;

  string str_target_sub_id = target_sub_id.getValue();
  string str_sender_sub_id = sender_sub_id.getValue();
  string str_target_comp_id = target_comp_id.getValue();
  string session_id = str_target_comp_id.substr(0, 3);
  string firm_id = str_target_comp_id.substr(3, 3);

  InputOrder *input_order = order_pool_.get(order_id.getValue());
  string order_flow_id = "";
  if (input_order != NULL) {
    order_flow_id = input_order->order_flow_id;
  } else {
    printf("InputOrder [SysID-%s] not found!\n", order_id.getValue().c_str());
  }

  string message_type = string(msg_type.getValue()) + "/" +
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
  audit_log.WriteElement("account_number", user_id_);
  audit_log.WriteElement("session_id", session_id);
  audit_log.WriteElement("executing_firm_id", firm_id);
  audit_log.WriteElement("message_type", message_type);
  // audit_log.WriteElement("customer_type_indicator", "2");  // CtiCode-9702
  // audit_log.WriteElement("origin", "Firm");
  audit_log.WriteElement("cme_globex_message_id", exec_id.getValue());
  // audit_log.WriteElement("message_link_id", trans_id);
  audit_log.WriteElement("order_flow_id", order_flow_id);
  // audit_log.WriteElement("instrument_description", security_desc.getValue());
  // market segment id can be found in InstrumentDefinition in market data
  audit_log.WriteElement("market_segment_id", str_sender_sub_id);
  audit_log.WriteElement("client_order_id", cl_ord_id.getValue());
  audit_log.WriteElement("cme_globex_order_id", order_id.getValue());
  audit_log.WriteElement("reject_reason", cxl_rej_reason.getValue());
  
  // audit_log.WriteElement("ifm_flag", "N");
  // audit_log.WriteElement("display_quantity", order_qty.getValue());
  // audit_log.WriteElement("minimum_quantity", 0);
  // audit_log.WriteElement("country_of_origin", sender_loc_id_);
  audit_log.WriteElement("manual_order_identifier", manual_order_identifier);
  // audit_log.WriteElement("")
  audit_trail_.WriteLog(audit_log);
}

void ImplFixFtdcTraderApi::LogAuditTrail(
      const CME_FIX_NAMESPACE::Reject& reject) {
  FIX::MsgType msg_type;
  FIX::TargetCompID target_comp_id;
  FIX::TargetSubID target_sub_id;
  FIX::SenderSubID sender_sub_id;
  FIX::RefSeqNum ref_seq_num;
  // FIX::Text text;
    
  reject.getHeader().getField(msg_type);
  reject.getHeader().getField(target_comp_id);
  reject.getHeader().getField(target_sub_id);
  reject.getHeader().getField(sender_sub_id);
  reject.getField(ref_seq_num);
  // reject.getField(text);
  string manual_order_identifier = reject.getField(1028);

  AuditLog audit_log;

  string str_target_sub_id = target_sub_id.getValue();
  string str_sender_sub_id = sender_sub_id.getValue();
  string str_target_comp_id = target_comp_id.getValue();
  string session_id = str_target_comp_id.substr(0, 3);
  string firm_id = str_target_comp_id.substr(3, 3);
  string message_type = string(msg_type.getValue());

  string timestamp = time_now();
  audit_log.WriteElement("receiving_timestamps", timestamp);
  audit_log.WriteElement("message_direction", "FROM CME");
  audit_log.WriteElement("operator_id", str_target_sub_id);
  audit_log.WriteElement("session_id", session_id);
  audit_log.WriteElement("executing_firm_id", firm_id);
  audit_log.WriteElement("message_type", message_type);
  audit_log.WriteElement("market_segment_id", str_sender_sub_id);
  audit_log.WriteElement("reject_reason", ref_seq_num.getValue());
  
  audit_log.WriteElement("manual_order_identifier", manual_order_identifier);
  audit_trail_.WriteLog(audit_log);
}

void ImplFixFtdcTraderApi::LogAuditTrail(
      const CME_FIX_NAMESPACE::BusinessMessageReject& reject) {
  FIX::MsgType msg_type;
  FIX::TargetCompID target_comp_id;
  FIX::TargetSubID target_sub_id;
  FIX::SenderSubID sender_sub_id;
  FIX::RefSeqNum ref_seq_num;
  // FIX::Text text;
    
  reject.getHeader().getField(msg_type);
  reject.getHeader().getField(target_comp_id);
  reject.getHeader().getField(target_sub_id);
  reject.getHeader().getField(sender_sub_id);
  reject.getField(ref_seq_num);
  // reject.getField(text);
  string manual_order_identifier = reject.getField(1028);

  AuditLog audit_log;

  string str_target_sub_id = target_sub_id.getValue();
  string str_sender_sub_id = sender_sub_id.getValue();
  string str_target_comp_id = target_comp_id.getValue();
  string session_id = str_target_comp_id.substr(0, 3);
  string firm_id = str_target_comp_id.substr(3, 3);
  string message_type = string(msg_type.getValue());

  string timestamp = time_now();
  audit_log.WriteElement("receiving_timestamps", timestamp);
  audit_log.WriteElement("message_direction", "FROM CME");
  audit_log.WriteElement("operator_id", str_target_sub_id);
  audit_log.WriteElement("session_id", session_id);
  audit_log.WriteElement("executing_firm_id", firm_id);
  audit_log.WriteElement("message_type", message_type);
  audit_log.WriteElement("market_segment_id", str_sender_sub_id);
  audit_log.WriteElement("reject_reason", ref_seq_num.getValue());
  
  audit_log.WriteElement("manual_order_identifier", manual_order_identifier);
  audit_trail_.WriteLog(audit_log);
}

void ImplFixFtdcTraderApi::FillFixHeader(FIX::Message& message) {
  message.getHeader().setField(FIX::SenderSubID(sender_sub_id_));
  message.getHeader().setField(FIX::SenderLocationID(sender_loc_id_));
  message.getHeader().setField(FIX::TargetSubID(target_sub_id_));
}

void ImplFixFtdcTraderApi::FillLogonRequest(FIX::Message& message) {
  // char raw_data[1024] = {0};
  // char raw_data_len[16] = {0};
  // snprintf(raw_data, sizeof(raw_data), "%s", pReqUserLoginField->Password);
  // snprintf(raw_data_len, sizeof(raw_data_len), "%s", strlen(raw_data));

  char system_name[32] = "CME_CFI";
  char system_version[32] = "1.0";
  char system_vendor[32] = "Cash Algo";
  FIX::RawData raw_data(user_passwd_);
  FIX::RawDataLength raw_data_len(strlen(user_passwd_));
  FIX::ResetSeqNumFlag reset_seq_num_flag(false);
  FIX::EncryptMethod encrypt_method(0);
  message.setField(raw_data);
  message.setField(raw_data_len);
  message.setField(reset_seq_num_flag);
  message.setField(encrypt_method);
  message.setField(1603, system_name);  // customed fields
  message.setField(1604, system_version);
  message.setField(1605, system_vendor);

  string message_string = message.toString();
  cout << "Send Logon Message:\n" << message_string << endl;
}

void ImplFixFtdcTraderApi::FillLogoutRequest(FIX::Message& message) {
  string message_string = message.toString();
  cout << "Send Logout Message:\n" << message_string << endl;
}

void ImplFixFtdcTraderApi::FillHeartbeat(FIX::Message& message) {
  // TODO
}

void ImplFixFtdcTraderApi::FillResendRequest(FIX::Message& message) {
  // TODO
}

void ImplFixFtdcTraderApi::FillRejectRequest(FIX::Message& message) {
  // TODO
}

CThostFtdcOrderField ImplFixFtdcTraderApi::ToOrderField(
      const CME_FIX_NAMESPACE::ExecutionReport& report) {
  CThostFtdcOrderField order_field;
  memset(&order_field, 0, sizeof(order_field));

  FIX::Account account;
  report.getField(account);
  string str_account = account.getValue();
  snprintf(order_field.InvestorID, sizeof(order_field.InvestorID), "%s",
           str_account.c_str());

  FIX::ClOrdID cl_ord_id;
  report.getField(cl_ord_id);
  string str_cl_ord_id = cl_ord_id.getValue();
  snprintf(order_field.OrderRef, sizeof(order_field.OrderRef), "%s",
           str_cl_ord_id.c_str());

  FIX::OrderID order_id;
  report.getField(order_id);
  string str_order_id = order_id.getValue();
  snprintf(order_field.OrderSysID, sizeof(order_field.OrderSysID), "%s",
           str_order_id.c_str());
  
  FIX::OrderQty order_qty;
  report.getField(order_qty);
  int int_order_qty = order_qty.getValue();
  order_field.VolumeTotalOriginal = int_order_qty;

  FIX::OrdStatus ord_status;
  report.getField(ord_status);
  if (ord_status == FIX::OrdStatus_NEW) {
    order_field.OrderStatus = THOST_FTDC_OST_NoTradeQueueing;
  } else if (ord_status == FIX::OrdStatus_CANCELED) {
    order_field.OrderStatus = THOST_FTDC_OST_Canceled;
    // or THOST_FTDC_OST_PartTradedNotQueueing ?
  } else if (ord_status == FIX::OrdStatus_FILLED) {
    order_field.OrderStatus = THOST_FTDC_OST_AllTraded;
  }

  FIX::Price price;
  report.getField(price);
  double dbl_price = price.getValue();
  order_field.LimitPrice = dbl_price;

  FIX::Side side;
  report.getField(side);
  if (side == FIX::Side_BUY) {
    order_field.Direction = THOST_FTDC_D_Buy;
  } else {
    order_field.Direction = THOST_FTDC_D_Sell;
  }

  FIX::SecurityDesc security_desc;
  report.getField(security_desc);
  string str_security_desc = security_desc.getValue();
  snprintf(order_field.InstrumentID, sizeof(order_field.InstrumentID), "%s",
           str_security_desc.c_str());

  return order_field;
}

CThostFtdcTradeField ImplFixFtdcTraderApi::ToTradeField(
      const CME_FIX_NAMESPACE::ExecutionReport& report) {
  // todo
  CThostFtdcTradeField trade_field;
  memset(&trade_field, 0, sizeof(trade_field));

  FIX::Account account;
  report.getField(account);
  string str_account = account.getValue();
  snprintf(trade_field.InvestorID, sizeof(trade_field.InvestorID), "%s",
           str_account.c_str());

  FIX::ClOrdID cl_ord_id;
  report.getField(cl_ord_id);
  string str_cl_ord_id = cl_ord_id.getValue();
  snprintf(trade_field.OrderRef, sizeof(trade_field.OrderRef), "%s",
           str_cl_ord_id.c_str());

  FIX::LastPx last_px;
  report.getField(last_px);
  double dbl_last_px = last_px.getValue();
  trade_field.Price = dbl_last_px;

  FIX::LastQty last_qty;
  report.getField(last_qty);
  int int_last_qty = last_qty.getValue();
  trade_field.Volume = int_last_qty;

  FIX::OrderID order_id;
  report.getField(order_id);
  string str_order_id = order_id.getValue();
  snprintf(trade_field.OrderSysID, sizeof(trade_field.OrderSysID), "%s",
           str_order_id.c_str());

  FIX::Side side;
  report.getField(side);
  if (side == FIX::Side_BUY) {
    trade_field.Direction = THOST_FTDC_D_Buy;
  } else {
    trade_field.Direction = THOST_FTDC_D_Sell;
  }

  FIX::SecurityDesc security_desc;
  report.getField(security_desc);
  string str_security_desc = security_desc.getValue();
  snprintf(trade_field.InstrumentID, sizeof(trade_field.InstrumentID), "%s",
           str_security_desc.c_str());

  return trade_field;
}

CThostFtdcInputOrderField ImplFixFtdcTraderApi::ToInputOrderField(
      const CME_FIX_NAMESPACE::ExecutionReport& report) {
  CThostFtdcInputOrderField order_field;
  memset(&order_field, 0, sizeof(order_field));

  FIX::Account account;
  report.getField(account);
  string str_account = account.getValue();
  snprintf(order_field.InvestorID, sizeof(order_field.InvestorID), "%s",
           str_account.c_str());

  FIX::ClOrdID cl_ord_id;
  report.getField(cl_ord_id);
  string str_cl_ord_id = cl_ord_id.getValue();
  snprintf(order_field.OrderRef, sizeof(order_field.OrderRef), "%s",
           str_cl_ord_id.c_str());
  
  FIX::OrderQty order_qty;
  report.getField(order_qty);
  int int_order_qty = order_qty.getValue();
  order_field.VolumeTotalOriginal = int_order_qty;

  FIX::Price price;
  report.getField(price);
  double dbl_price = price.getValue();
  order_field.LimitPrice = dbl_price;

  FIX::Side side;
  report.getField(side);
  if (side == FIX::Side_BUY) {
    order_field.Direction = THOST_FTDC_D_Buy;
  } else {
    order_field.Direction = THOST_FTDC_D_Sell;
  }

  FIX::SecurityDesc security_desc;
  report.getField(security_desc);
  string str_security_desc = security_desc.getValue();
  snprintf(order_field.InstrumentID, sizeof(order_field.InstrumentID), "%s",
           str_security_desc.c_str());

  return order_field;
}

CThostFtdcInputOrderActionField ImplFixFtdcTraderApi::ToActionField(
      const CME_FIX_NAMESPACE::ExecutionReport& report) {
  CThostFtdcInputOrderActionField action_field;
  memset(&action_field, 0, sizeof(action_field));

  FIX::OrigClOrdID orig_cl_ord_id;
  report.getField(orig_cl_ord_id);
  string str_orig_cl_ord_id = orig_cl_ord_id.getValue();
  snprintf(action_field.OrderRef, sizeof(action_field.OrderRef), "%s",
           str_orig_cl_ord_id.c_str());

  // SecurityDesc may be not found
  // FIX::SecurityDesc security_desc;
  // report.getField(security_desc);
  // string str_security_desc = security_desc.getValue();
  // snprintf(action_field.InstrumentID, sizeof(action_field.InstrumentID),
  //          str_security_desc.c_str());

  return action_field;
}

CThostFtdcRspInfoField ImplFixFtdcTraderApi::ToRspField(
      const CME_FIX_NAMESPACE::ExecutionReport& report) {
  CThostFtdcRspInfoField rsp_field;
  memset(&rsp_field, 0, sizeof(rsp_field));

  FIX::OrdRejReason ord_rej_reason;
  report.getField(ord_rej_reason);
  rsp_field.ErrorID = ord_rej_reason.getValue();

  FIX::Text text;
  report.getField(text);
  snprintf(rsp_field.ErrorMsg, sizeof(rsp_field.ErrorMsg), "%s",
           text.getValue().c_str());

  return rsp_field;
}

CThostFtdcRspInfoField ImplFixFtdcTraderApi::ToRspField(
      const CME_FIX_NAMESPACE::OrderCancelReject& report) {
  CThostFtdcRspInfoField rsp_field;
  memset(&rsp_field, 0, sizeof(rsp_field));

  FIX::CxlRejReason cxl_rej_reason;
  report.getField(cxl_rej_reason);
  int error_id = cxl_rej_reason.getValue();
  if (error_id == 0) { // 0=Too late to cancel
    rsp_field.ErrorID = 1000;
  } else {
    rsp_field.ErrorID = error_id;
  }

  FIX::Text text;
  report.getField(text);
  snprintf(rsp_field.ErrorMsg, sizeof(rsp_field.ErrorMsg), "%s",
           text.getValue().c_str());

  return rsp_field;
}

const char* time_now() {
  static char timestamp_str[32];
  time_t rawtime;
  struct tm *timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  
  struct timeval tv;
  gettimeofday(&tv, NULL);
  
  snprintf(timestamp_str, sizeof(timestamp_str), 
           "%d%02d%02d-%02d:%02d:%02d.%03ld", 1900 + timeinfo->tm_year,
           1 + timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_hour,
           timeinfo->tm_min, timeinfo->tm_sec, tv.tv_usec / 1000);
  return timestamp_str;
}

int date_now() {
  // static char timestamp_str[32];
  time_t rawtime;
  struct tm *timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  
  int date = (1900 + timeinfo->tm_year) * 10000 +
             (1 + timeinfo->tm_mon) * 100 +
             timeinfo->tm_mday;
  return date;
}
