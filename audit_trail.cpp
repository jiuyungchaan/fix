#include "audit_trail.h"
#include "utils.h"

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
}

void AuditTrail::WriteLog(AuditLog& log) {
  if (log_file_.good()) {
    log_file_ << log.ToString() << endl;
    log_file_.flush();
  } else {
    printf("Write File Error!\n");
  }
}

