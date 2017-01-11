////////////////////////
///@author Kenny Chiu
///@date 20170105
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
#include <vector>
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
	AuditLog(std::string logstr);
	~AuditLog();

	void ClearAll();
	void ClearElement(std::string key);

	void WriteVector(const std::vector<std::string>& v);
	int WriteElement(std::string key, std::string value);
	int WriteElement(std::string key, const char *value);
	int WriteElement(std::string key, int value);
	int WriteElement(std::string key, double value);
	int WriteElement(std::string key, char value);

	std::string ToString();

 // private:
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
	int ReqQryInvestorPosition(CThostFtdcQryInvestorPositionField *pQryInvestorPosition,
														 int nRequestID);
	int ReqQryTradingAccount(CThostFtdcQryTradingAccountField *pQryTradingAccount,
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

	class Position {
	 public:
		std::string instrument_id_;
		int long_yd_pos_, short_yd_pos_;
		int long_trade_vol_, short_trade_vol_;
		double long_avg_price_, short_avg_price_;
		double long_turnover_, short_turnover_;
		Position(const std::string& instrument_id);
		~Position(){};
		void SetYdLongPosition(int vol, double price);
		void SetYdShortPosition(int vol, double price);
		void AddLongTrade(int vol, double price);
		void AddShortTrade(int vol, double price);
	};

	class PositionPool {
	 public:
		PositionPool();
		~PositionPool() {};

		void SetYdLongPosition(const std::string& instrument, int vol, double price);
		void SetYdShortPosition(const std::string& instrument, int vol, double price);
		void AddLongTrade(const std::string& instrument, int vol, double price);
		void AddShortTrade(const std::string& instrument, int vol, double price);
		int GetLongTradeVol(const std::string& instrument);
		int GetShortTradeVol(const std::string& instrument);
		int GetLongPos(const std::string& instrument);
		int GetShortPos(const std::string& instrument);
		int GetNetPos(const std::string& instrument);
	 // private:
		std::map<std::string, Position*> position_map_;
		pthread_mutex_t mutex_;
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

	void LoadSettlementPosition(const std::string& position_file_name);
	void LoadAuditTrail(const std::string& audit_file_name);

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

	static void *query_position(void *obj);

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
	OrderPool order_pool_;
	PositionPool position_pool_;
	std::fstream log_file_;
};

const char *time_now();
int date_now();
int date_yesterday();
void split(const std::string& str, const std::string& del,
					 std::vector<std::string>& v);

/* ===============================================
	 =============== IMPLEMENTATION ================ */

using namespace std;

AuditLog::AuditLog() {
	ClearAll();
}

AuditLog::AuditLog(string logstr) {
	ClearAll();
	vector<string> v;
	split(logstr, ",", v);
	if (v.size() == 46) {
		WriteVector(v);
	} else if (v.size() == 45) {
		v.push_back("");
		WriteVector(v);
	} else {
		cout << "Invalid audit log: " << logstr << endl;
	}
}

AuditLog::~AuditLog() {
	// TODO
}

void AuditLog::ClearAll() {
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

void AuditLog::ClearElement(string key) {
	map<string, string>::iterator iter = elements.find(key);
	if (iter == elements.end()) {
		elements.insert(pair<string, string>(key, ""));
	} else {
		iter->second = "";
	}
}

void AuditLog::WriteVector(const vector<string>& v) {
	elements["sending_timestamps"] = v[0];
	elements["receiving_timestamps"] = v[1];
	elements["message_direction"] = v[2];
	elements["operator_id"] = v[3];
	elements["self_match_prevention_id"] = v[4];
	elements["account_number"] = v[5];
	elements["session_id"] = v[6];
	elements["executing_firm_id"] = v[7];
	elements["manual_order_identifier"] = v[8];
	elements["message_type"] = v[9];
	elements["customer_type_indicator"] = v[10];
	elements["origin"] = v[11];
	elements["cme_globex_message_id"] = v[12];
	elements["message_link_id"] = v[13];
	elements["order_flow_id"] = v[14];
	elements["spread_leg_link_id"] = v[15];
	elements["instrument_description"] = v[16];
	elements["market_segment_id"] = v[17];
	elements["client_order_id"] = v[18];
	elements["cme_globex_order_id"] = v[19];
	elements["buy_sell_indicator"] = v[20];
	elements["quantity"] = v[21];
	elements["limit_price"] = v[22];
	elements["stop_price"] = v[23];
	elements["order_type"] = v[24];
	elements["order_qualifier"] = v[25];
	elements["ifm_flag"] = v[26];
	elements["display_quantity"] = v[27];
	elements["minimum_quantity"] = v[28];
	elements["country_of_origin"] = v[29];
	elements["fill_price"] = v[30];
	elements["fill_quantity"] = v[31];
	elements["cumulative_quantity"] = v[32];
	elements["remaining_quantity"] = v[33];
	elements["aggressor_flag"] = v[34];
	elements["source_of_cancellation"] = v[35];
	elements["reject_reason"] = v[36];
	elements["processed_quotes"] = v[37];
	elements["cross_id"] = v[38];
	elements["quote_request_id"] = v[39];
	elements["message_quote_id"] = v[40];
	elements["quote_entry_id"] = v[41];
	elements["bid_price"] = v[42];
	elements["bid_size"] = v[43];
	elements["offer_price"] = v[44];
	elements["offer_size"] = v[45];
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

ImplFixFtdcTraderApi::Position::Position(const string& instrument_id) :
		instrument_id_(instrument_id), long_yd_pos_(0), short_yd_pos_(0), 
		long_trade_vol_(0), short_trade_vol_(0), long_avg_price_(0.0),
		short_avg_price_(0.0), long_turnover_(0.0), short_turnover_(0.0) {}

void ImplFixFtdcTraderApi::Position::SetYdLongPosition(int vol, double price) {
	long_yd_pos_ = vol;
}

void ImplFixFtdcTraderApi::Position::SetYdShortPosition(int vol, double price) {
	short_yd_pos_ = vol;
}

void ImplFixFtdcTraderApi::Position::AddLongTrade(int vol, double price) {
	long_turnover_ += price * (double)vol;
	long_trade_vol_ += vol;
	long_avg_price_ = (long_trade_vol_ == 0)? 0.0 : long_turnover_/long_trade_vol_;
}

void ImplFixFtdcTraderApi::Position::AddShortTrade(int vol, double price) {
	short_turnover_ += price * (double)vol;
	short_trade_vol_ += vol;
	short_avg_price_ = (short_trade_vol_ == 0)? 0.0 : short_turnover_/short_trade_vol_;
}

ImplFixFtdcTraderApi::PositionPool::PositionPool() {
	pthread_mutex_init(&mutex_, NULL);
}

void ImplFixFtdcTraderApi::PositionPool::SetYdLongPosition(const string& instrument,
			int vol, double price) {
	pthread_mutex_lock(&mutex_);
	map<string, Position*>::iterator it = position_map_.find(instrument);
	if (it == position_map_.end()) {
		Position *pos = new Position(instrument);
		pos->SetYdLongPosition(vol, price);
		position_map_.insert(pair<string, Position*>(instrument, pos));
	} else {
		Position *pos = it->second;
		pos->SetYdLongPosition(vol, price);
	}
	pthread_mutex_unlock(&mutex_);
}

void ImplFixFtdcTraderApi::PositionPool::SetYdShortPosition(const string& instrument,
			int vol, double price) {
	pthread_mutex_lock(&mutex_);
	map<string, Position*>::iterator it = position_map_.find(instrument);
	if (it == position_map_.end()) {
		Position *pos = new Position(instrument);
		pos->SetYdShortPosition(vol, price);
		position_map_.insert(pair<string, Position*>(instrument, pos));
	} else {
		Position *pos = it->second;
		pos->SetYdShortPosition(vol, price);
	}
	pthread_mutex_unlock(&mutex_);
}

void ImplFixFtdcTraderApi::PositionPool::AddLongTrade(const string& instrument,
			int vol, double price) {
	pthread_mutex_lock(&mutex_);
	map<string, Position*>::iterator it = position_map_.find(instrument);
	if (it == position_map_.end()) {
		Position *pos = new Position(instrument);
		pos->AddLongTrade(vol, price);
		position_map_.insert(pair<string, Position*>(instrument, pos));
	} else {
		Position *pos = it->second;
		pos->AddLongTrade(vol, price);
	}
	pthread_mutex_unlock(&mutex_);
}

void ImplFixFtdcTraderApi::PositionPool::AddShortTrade(const string& instrument,
			int vol, double price) {
	pthread_mutex_lock(&mutex_);
	map<string, Position*>::iterator it = position_map_.find(instrument);
	if (it == position_map_.end()) {
		Position *pos = new Position(instrument);
		pos->AddShortTrade(vol, price);
		position_map_.insert(pair<string, Position*>(instrument, pos));
	} else {
		Position *pos = it->second;
		pos->AddShortTrade(vol, price);
	}
	pthread_mutex_unlock(&mutex_);
}

int ImplFixFtdcTraderApi::PositionPool::GetLongTradeVol(const string& instrument) {
	map<string, Position*>::iterator it = position_map_.find(instrument);
	int vol = 0;
	if (it != position_map_.end()) {
		pthread_mutex_lock(&mutex_);
		Position *pos = it->second;
		vol = pos->long_trade_vol_;
		pthread_mutex_unlock(&mutex_);
	}
	return vol;
}

int ImplFixFtdcTraderApi::PositionPool::GetShortTradeVol(const string& instrument) {
	map<string, Position*>::iterator it = position_map_.find(instrument);
	int vol = 0;
	if (it != position_map_.end()) {
		pthread_mutex_lock(&mutex_);
		Position *pos = it->second;
		vol = pos->short_trade_vol_;
		pthread_mutex_unlock(&mutex_);
	}
	return vol;
}

int ImplFixFtdcTraderApi::PositionPool::GetLongPos(const string& instrument) {
	map<string, Position*>::iterator it = position_map_.find(instrument);
	int vol = 0;
	if (it != position_map_.end()) {
		pthread_mutex_lock(&mutex_);
		Position *pos = it->second;
		int long_vol = pos->long_trade_vol_;
		int short_vol = pos->short_trade_vol_;
		vol = (long_vol > short_vol)? long_vol - short_vol : 0;
		pthread_mutex_unlock(&mutex_);
	}
	return vol;
}

int ImplFixFtdcTraderApi::PositionPool::GetShortPos(const string& instrument) {
	map<string, Position*>::iterator it = position_map_.find(instrument);
	int vol = 0;
	if (it != position_map_.end()) {
		pthread_mutex_lock(&mutex_);
		Position *pos = it->second;
		int long_vol = pos->long_trade_vol_;
		int short_vol = pos->short_trade_vol_;
		vol = (short_vol > long_vol)? short_vol - long_vol : 0;
		pthread_mutex_unlock(&mutex_);
	}
	return vol;
}

int ImplFixFtdcTraderApi::PositionPool::GetNetPos(const string& instrument) {
	map<string, Position*>::iterator it = position_map_.find(instrument);
	int vol = 0;
	if (it != position_map_.end()) {
		pthread_mutex_lock(&mutex_);
		Position *pos = it->second;
		int long_vol = pos->long_trade_vol_;
		int short_vol = pos->short_trade_vol_;
		vol = long_vol - short_vol;
		pthread_mutex_unlock(&mutex_);
	}
	return vol;
}

ImplFixFtdcTraderApi::ImplFixFtdcTraderApi(const char *configPath) :
		last_msg_seq_num_(0), acc_session_id_{0}, firm_id_{0}, sender_sub_id_{0},
		target_sub_id_{0}, sender_loc_id_{0}, self_match_prev_id_{0} {
	snprintf(fix_config_path_, sizeof(fix_config_path_), "%s", configPath);
#ifndef __DEBUG__
	log_file_.open("./dropcopy.log", fstream::out | fstream::app);
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
	return 0;
}

int ImplFixFtdcTraderApi::ReqOrderAction(
			CThostFtdcInputOrderActionField *pInputOrderAction, int nRequestID) {
	return 0;
}

int ImplFixFtdcTraderApi::ReqMassOrderAction(
			CThostFtdcInputOrderActionField *pInputOrderAction, int nRequestID) {
	return 0;
}

int ImplFixFtdcTraderApi::ReqQryInvestorPosition(
			CThostFtdcQryInvestorPositionField *pQryInvestorPosition, int nRequestID) {
	pthread_t query_thread;
	int ret = pthread_create(&query_thread, NULL, query_position, (void *)this);
	if (ret != 0) {
		cout << "Failed to query investor position!" << endl;
	}
	return ret;
}

int ImplFixFtdcTraderApi::ReqQryTradingAccount(
			CThostFtdcQryTradingAccountField *pQryTradingAccount, int nRequestID) {
	return 0;
}

void *ImplFixFtdcTraderApi::query_position(void *obj) {
	ImplFixFtdcTraderApi *self = (ImplFixFtdcTraderApi *)obj;
	map<string, Position*>::iterator it;
	for (it = self->position_pool_.position_map_.begin();
			 it != self->position_pool_.position_map_.end(); it++) {
		Position *position = it->second;
		int net_position = position->long_trade_vol_ - position->short_trade_vol_;
		if (net_position != 0) {
			CThostFtdcInvestorPositionField field;
			CThostFtdcRspInfoField rsp;
			memset(&field, 0, sizeof(field));
			memset(&rsp, 0, sizeof(rsp));
			snprintf(field.InvestorID, sizeof(field.InvestorID), "%s", self->user_id_);
			snprintf(field.InstrumentID, sizeof(field.InstrumentID), "%s", 
							 position->instrument_id_.c_str());
			int date = date_now();
			snprintf(field.TradingDay, sizeof(field.TradingDay), "%d", date);
			if (net_position > 0) {
				field.PosiDirection = THOST_FTDC_PD_Long;
				field.YdPosition = position->long_yd_pos_;
				field.Position = net_position;
			} else {
				field.PosiDirection = THOST_FTDC_PD_Short;
				field.YdPosition = position->short_yd_pos_;
				field.Position = -1*net_position;
			}

			self->trader_spi_->OnRspQryInvestorPosition(&field, &rsp, 0, true);
		}
	}
	return (void *)NULL;
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
	string settle_position_file_name = dict.getString("SETTLEPOSITIONFILE");

	char audit_file_name[256];
	snprintf(audit_file_name, sizeof(audit_file_name), 
					 "%d_%s_%s_%s_%s_%s_%s_audittrail.globex.csv",
					 date, ec_id.c_str(), market_code.c_str(), platform_code.c_str(),
					 session_id.c_str(), client_name.c_str(), add_detail.c_str());
	LoadSettlementPosition(settle_position_file_name);
	LoadAuditTrail(audit_file_name);

	audit_trail_.Init(audit_file_name);

	cout << "Session created : " << session_id_ << endl;
}

void ImplFixFtdcTraderApi::onLogon(const FIX::SessionID& sessionID) {
	cout << "Logon - " << sessionID << endl;
	// trader_spi_->OnFrontConnected();
	CThostFtdcRspUserLoginField login_field;
	memset(&login_field, 0, sizeof(login_field));
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
			if (trade_field.Direction == THOST_FTDC_D_Buy) {
				position_pool_.AddLongTrade(trade_field.InstrumentID, 
							trade_field.Volume, trade_field.Price);
			} else {
				position_pool_.AddShortTrade(trade_field.InstrumentID, 
							trade_field.Volume, trade_field.Price);
			}
			LogAuditTrail(report);
			trader_spi_->OnRtnTrade(&trade_field);   // OnRtnTrade
		}
		break;
		case FIX::ExecType_FILL: {
			CThostFtdcTradeField trade_field = ToTradeField(report);
			CThostFtdcOrderField order_field = ToOrderField(report);
			if (trade_field.Direction == THOST_FTDC_D_Buy) {
				position_pool_.AddLongTrade(trade_field.InstrumentID, 
							trade_field.Volume, trade_field.Price);
			} else {
				position_pool_.AddShortTrade(trade_field.InstrumentID, 
							trade_field.Volume, trade_field.Price);
			}
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
		int real_order_id = atoi(cl_ord_id.getValue().c_str()) / 100;
		char buffer[64];
		snprintf(buffer, sizeof(buffer), "%s%08d",
						 target_comp_id.getValue().c_str(), real_order_id);
		order_flow_id = buffer;
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
		int real_order_id = atoi(cl_ord_id.getValue().c_str()) / 100;
		char buffer[64];
		snprintf(buffer, sizeof(buffer), "%s%08d",
						 target_comp_id.getValue().c_str(), real_order_id);
		order_flow_id = buffer;
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

void ImplFixFtdcTraderApi::LoadSettlementPosition(
			const string& settle_position_file_name) {
	fstream position_file;
	position_file.open(settle_position_file_name.c_str(), fstream::in);
	vector<string> info_list;
	if (position_file.good()) {
		while(!position_file.eof()) {
			char line[512];
			info_list.clear();
			position_file.getline(line, 512);
			split(line, ",", info_list);
			if (info_list.size() != 4) {
				continue;
			}
			string instrument = info_list[0];
			string direction = info_list[1];
			int position = atoi(info_list[2].c_str());
			double price = atof(info_list[3].c_str());
			cout << "LoadSettlementPosition:" << direction << " " << instrument
					 << " " << position << " " << price << endl;
			if (direction == "long") {
				position_pool_.AddLongTrade(instrument, position, price);
			} else if (direction == "short") {
				position_pool_.AddShortTrade(instrument, position, price);
			}
		}
	} // if file exists
	position_file.close();
}

void ImplFixFtdcTraderApi::LoadAuditTrail(const string& audit_file_name) {
	fstream audit_file;
	audit_file.open(audit_file_name.c_str(), fstream::in);
	vector<string> info_list;
	if (audit_file.good()) {
		while(!audit_file.eof()) {
			char line[512];
			info_list.clear();
			audit_file.getline(line, 512);
			AuditLog audit_log(line);
			string message_type = audit_log.elements["message_type"];
			if (message_type == "") {
				continue;
			}
			split(message_type, "/", info_list);
			if (info_list.size() != 2) {
				continue;
			}
			int msg_type = atoi(info_list[0].c_str());
			int order_status = atoi(info_list[1].c_str());
			if (msg_type == 8 && (order_status == 1 || order_status == 2)) {
				int direction = atoi(audit_log.elements["buy_sell_indicator"].c_str());
				string instrument = audit_log.elements["instrument_description"];
				int vol = atoi(audit_log.elements["fill_quantity"].c_str());
				double price = atof(audit_log.elements["fill_price"].c_str());
				cout << "LoadAuditTrail:" << instrument << " " << direction
						 << " " << vol << " " << price << endl;
				if (direction == 1) {
					position_pool_.AddLongTrade(instrument, vol, price);
				} else if (direction == 2) {
					position_pool_.AddShortTrade(instrument, vol, price);
				}
			} // if message_type is FILL NOTICE
		}
	} // if file exists
	audit_file.close(); 
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
		FIX::OrigClOrdID orig_cl_ord_id;
		report.getField(orig_cl_ord_id);
		string str_orig_cl_ord_id = orig_cl_ord_id.getValue();
		snprintf(order_field.OrderRef, sizeof(order_field.OrderRef), "%s",
						 str_orig_cl_ord_id.c_str());
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

int date_yesterday() {
	// static char timestamp_str[32];
	time_t rawtime;
	struct tm *timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	int year = 1900 + timeinfo->tm_year;
	int month = 1 + timeinfo->tm_mon;
	int day = timeinfo->tm_mday;

	int mon_days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	if (year % 4 == 0 && (year % 400 == 0 || year % 100 != 0)) {
		mon_days[1] = 29;
	}
	int year_yes = (day == 1 && month == 1)? year - 1 : year;
	int month_yes = (day == 1)? month - 1 : month;
	month_yes = (month_yes == 0)? 12 : month_yes;
	int day_yes = (day == 1)? mon_days[month_yes - 1] : day - 1;

	int date = year_yes * 10000 + month_yes * 100 + day_yes;
	return date;
}

void split(const string& str, const string& del, vector<string>& v) {
	string::size_type start, end;
	start = 0;
	end = str.find(del);
	while(end != string::npos) {
		v.push_back(str.substr(start, end - start));
		start = end + del.size();
		end = str.find(del, start);
	}
	if (start != str.length()) {
		v.push_back(str.substr(start));
	}
}
