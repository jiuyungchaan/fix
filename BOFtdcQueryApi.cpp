////////////////////////
///@author Kenny Chiu
///@date 20170120
///@summary Implementation of BOFtdcTraderApi and ImplBOFtdcQueryApi
///         
///
////////////////////////

#include "BOFtdcQueryApi.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <cctype>
#include <algorithm>
#include <vector>


const char *second_now();
const char *time_now();
void split(const std::string& str, const std::string& del, std::vector<std::string>& v);

class ImplBOFtdcQueryApi : public BOFtdcQueryApi{
 public:
  explicit ImplBOFtdcQueryApi(const char *configPath);

  // interfaces of CTsSecurityFtdcTraderApi
  void Release();
  void Init();
  int Join();
  const char *GetTradingDay();
  void RegisterFront(char *pszFrontAddress);
  void RegisterSpi(BOFtdcQuerySpi *pSpi);
  void SubscribePrivateTopic(SECURITY_TE_RESUME_TYPE nResumeType);
  void SubscribePublicTopic(SECURITY_TE_RESUME_TYPE nResumeType);
  int ReqUserLogin(CSecurityFtdcReqUserLoginField *pReqUserLoginField,
                   int nRequestID);
  int ReqUserLogout(CSecurityFtdcUserLogoutField *pUserLogout,
                    int nRequestID);
  int ReqFetchAuthRandCode(CSecurityFtdcAuthRandCodeField *pAuthRandCode,
                           int nRequestID);
  int ReqQryInstrument(CSecurityFtdcQryInstrumentField *pQryInstrument,
                       int nRequestID);
  int ReqQryTradingAccount(CSecurityFtdcQryTradingAccountField *pQryTradingAccount,
                           int nRequestID);
  int ReqQryOFInstrument(CSecurityFtdcQryOFInstrumentField *pQryOFInstrument,
                         int nRequestID);
  int ReqQryMarketDataStaticInfo(CSecurityFtdcQryMarketDataStaticInfoField *pQryMarketDataStaticInfo,
                                 int nRequestID);
  int ReqQryOrder(CSecurityFtdcQryOrderField *pQryOrder, int nRequestID);
  int ReqQryTrade(CSecurityFtdcQryTradeField *pQryTrade, int nRequestID);
  int ReqQryInvestorPosition(CSecurityFtdcQryInvestorPositionField *pQryInvestorPosition,
                             int nRequestID);
  int ReqQryCreditStockAssignInfo(CSecurityFtdcQryCreditStockAssignInfoField *pQryCreditStockAssignInfo,
                                  int nRequestID);

 protected:
  virtual ~ImplBOFtdcQueryApi();

 private:

  static void *recv_thread(void *obj);
  static void *callback_onlogin(void *obj);
  static void *callback_onlogout(void *obj);
  static void *callback_onrandcode(void *obj);

  void handle_data(char *data, int len);
  void decode(const char *message);

  // typedef std::map<std::string, std::string> stringmap;

  CSecurityFtdcInvestorPositionField ToPositionField(
        std::map<std::string, std::string>& properties);
  CSecurityFtdcTradingAccountField ToAccountField(
        std::map<std::string, std::string>& properties);
  CSecurityFtdcCreditStockAssignInfoField ToMarginStockField(
        std::map<std::string, std::string>& properties);
  CSecurityFtdcRspInfoField ToRspField(
        std::map<std::string, std::string>& properties);

  BOFtdcQuerySpi *trader_spi_;
  char front_addr_[64];
  char user_id_[64];
  char user_passwd_[64];

  int server_fd_;
  struct sockaddr_in server_addr_;

  pthread_t socket_thread_;

  std::fstream log_file_;
};


/* ===============================================
   =============== IMPLEMENTATION ================ */

using namespace std;

BOFtdcQueryApi *BOFtdcQueryApi::CreateFtdcQueryApi(const char *configPath) {
  ImplBOFtdcQueryApi *api = new ImplBOFtdcQueryApi(configPath);
  return (BOFtdcQueryApi *)api;
}

ImplBOFtdcQueryApi::ImplBOFtdcQueryApi(const char *pszFlowPath) :
    front_addr_{0}, user_id_{0}, user_passwd_{0} {
  char log_file_name[128];
  if (strcmp(pszFlowPath, "") == 0) {
    snprintf(log_file_name, sizeof(log_file_name), "cox.qry.log");
  } else {
    snprintf(log_file_name, sizeof(log_file_name), "%s.cox.qry.log", pszFlowPath);
  }
#ifndef __DEBUG__
  log_file_.open(log_file_name, fstream::out | fstream::app);
#endif
}

ImplBOFtdcQueryApi::~ImplBOFtdcQueryApi() {
  // TODO
}

void *ImplBOFtdcQueryApi::recv_thread(void *obj) {
  static const int BUF_SIZE = 2048;
  char buffer[BUF_SIZE];
  ImplBOFtdcQueryApi *self = (ImplBOFtdcQueryApi *)obj;
  int fd = self->server_fd_;
  int recv_len;
  char *total_buffer = NULL;
  int  sec_cnt = 0;  // section count
  int total_len = 0;
  while((recv_len = recv(fd, buffer, BUF_SIZE, 0)) > 0) {
    // if message received full of buffer
  
    sec_cnt++;
    char *tmp_ptr = (char *)malloc(BUF_SIZE*(sec_cnt + 1)*sizeof(char));
    if (sec_cnt > 1) {
      // memcpy(tmp_ptr, total_buffer, (sec_cnt - 1)*BUF_SIZE);
      memcpy(tmp_ptr, total_buffer, total_len);
      free(total_buffer);
    }
    // memcpy(tmp_ptr + (sec_cnt - 1)*BUF_SIZE, buffer, BUF_SIZE);
    memcpy(tmp_ptr + total_len, buffer, recv_len);
    total_buffer = tmp_ptr;
    total_len += recv_len;

    if (total_len > 1 && total_buffer[total_len - 1] == '\0') {
      self->log_file_ << time_now() << "- Data total received " << total_len << endl;

      self->handle_data(total_buffer, total_len);
      sec_cnt = 0;
      free(total_buffer);
      total_buffer = NULL;
      total_len = 0;
    }
  }

  printf("Failed to receive data! Exit thread...\n");
  self->log_file_ << time_now() << "- Failed to receive data! Exit thread..." << endl;
  return (void *)NULL;
}


void ImplBOFtdcQueryApi::handle_data(char *data, int len) {
  char *start = data;
  while (true) {
    char *p = strchr(start, '\0');
    if (p == NULL) 
      break;
    if (p - data < len - 1) {
      // printf("Multi datagram-[%ld]:%s\n", p - start, start);
      log_file_ << time_now() << "- Multi datagram-" << (long)(p - start)
                      << ":" << start << endl;
      decode(start);
      start = p + 1;
    } else {
      // printf("Last datagram-[%ld]:%s\n", p - start, start);
      log_file_ << time_now() << "- Last datagram-" << (long)(p - start)
                      << ":" << start << endl;
      decode(start);
      break;
    }
  }  // while
}

void ImplBOFtdcQueryApi::decode(const char *message) {
  vector<string> pairs;
  split(string(message), ";", pairs);
  if (pairs.size() < 1) {
    return;
  }

  map<string, string> properties;
  vector<string> key_val;
  for(size_t i = 0; i < pairs.size(); i++) {
    key_val.clear();
    split(pairs[i], "=", key_val);
    if (key_val.size() != 2) {
#ifndef __DEBUG__
      log_file_ << "Invalid key value: " << pairs[i] << endl;
#endif  // __DEBUG__
      continue;
    }
    properties[key_val[0]] = key_val[1];
  }

  string msg_type = properties["TYPE"];
  if (msg_type == "POSITION") {
    CSecurityFtdcInvestorPositionField pos_field = ToPositionField(properties);
    CSecurityFtdcRspInfoField info_field;
    memset(&info_field, 0, sizeof(info_field));
    bool isLast = (properties["IS_LAST"] == "1");
    if (strcmp(pos_field.InvestorID, user_id_) == 0) {
      trader_spi_->OnRspQryInvestorPosition(&pos_field, &info_field, 0, isLast);
    }
  } else if (msg_type == "ACCOUNT") {
    CSecurityFtdcTradingAccountField acc_field = ToAccountField(properties);
    CSecurityFtdcRspInfoField info_field;
    memset(&info_field, 0, sizeof(info_field));
    if (strcmp(acc_field.AccountID, user_id_) == 0) {
      trader_spi_->OnRspQryTradingAccount(&acc_field, &info_field, 0, true);
    }
  } else if (msg_type == "MARGINSTOCK") {
    CSecurityFtdcCreditStockAssignInfoField stock_field = ToMarginStockField(properties);
    CSecurityFtdcRspInfoField info_field;
    memset(&info_field, 0, sizeof(info_field));
    bool isLast = (properties["IS_LAST"] == "1");
    if (strcmp(stock_field.InvestorID, user_id_) == 0) {
      trader_spi_->OnRspQryCreditStockAssignInfo(&stock_field, &info_field, 0, isLast);
    }
  } else if (msg_type == "LOGIN") {
      string error_code = properties["ERROR"];
      if (error_code == "0") {
        CSecurityFtdcRspUserLoginField login_field;
        memset(&login_field, 0, sizeof(login_field));
        trader_spi_->OnRspUserLogin(&login_field, NULL, 0, true);
      } else {
        string error_msg = properties["MESSAGE"];
        CSecurityFtdcRspUserLoginField login_field;
        memset(&login_field, 0, sizeof(login_field));
        CSecurityFtdcRspInfoField info_field;
        info_field.ErrorID = atoi(error_code.c_str());
        snprintf(info_field.ErrorMsg, sizeof(info_field.ErrorMsg), "%s",
                 error_msg.c_str());
        trader_spi_->OnRspUserLogin(&login_field, NULL, 0, true);
      }
    }
}

void ImplBOFtdcQueryApi::Release() {
  // TODO
}

void ImplBOFtdcQueryApi::Init() {
  // connect server
  if (strcmp(front_addr_, "") == 0) {
    printf("ERROR: Front Address is not registered!\n");
    return ;
  }
  vector<string> vals;
  split(string(front_addr_), "//", vals);
  if (vals.size() == 2) { 
    vector<string> addr;
    split(vals[1], ":", addr);
    if (addr.size() == 2) {
      int port = atoi(addr[1].c_str());
      server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
      if (server_fd_ == -1) {
        printf("Failed to create socket: %s-%d\n", strerror(errno), errno);
        exit(0);
      }
      char ip_addr[32];
      snprintf(ip_addr, sizeof(ip_addr), "%s", addr[0].c_str());
      memset(&server_addr_, 0, sizeof(server_addr_));
      server_addr_.sin_family = AF_INET;
      server_addr_.sin_port = htons(port);
      if (inet_pton(AF_INET, ip_addr, &server_addr_.sin_addr) <= 0) {
        printf("Failed to parse ip address: %s-%s-%d\n", ip_addr, 
               strerror(errno), errno);
        trader_spi_->OnFrontDisconnected(1001);
        return;
      }
      if (connect(server_fd_, (struct sockaddr*)&server_addr_, 
                  sizeof(server_addr_)) < 0) {
        printf("Failed to connect: [%s:%d]-%s-%d\n", ip_addr, port, 
               strerror(errno), errno);
        trader_spi_->OnFrontDisconnected(1001);
        return; 
      }
      trader_spi_->OnFrontConnected();

      int ret = pthread_create(&socket_thread_, NULL, recv_thread, (void *)this);
      if (ret != 0) {
        printf("Failed to create socket thread!\n");
        exit(0);
      }
      printf("Socket thread create successfully!\n");
    }
  } else {
    printf("ERROR: Invalid Front Address: %s\n", front_addr_);
    trader_spi_->OnFrontDisconnected(1001);   
  }
}

int ImplBOFtdcQueryApi::Join() {
  // dummy function to adapt CTP API
  return 0;
}

const char * ImplBOFtdcQueryApi::GetTradingDay() {
  static char timestamp_str[32];
  time_t rawtime;
  struct tm *timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  
  int date = (1900 + timeinfo->tm_year) * 10000 +
             (1 + timeinfo->tm_mon) * 100 +
             timeinfo->tm_mday;
  snprintf(timestamp_str, sizeof(timestamp_str), "%d", date);
  return timestamp_str;
}

void ImplBOFtdcQueryApi::RegisterFront(char *pszFrontAddress) {
  snprintf(front_addr_, sizeof(front_addr_), "%s", pszFrontAddress);
}


void ImplBOFtdcQueryApi::RegisterSpi(BOFtdcQuerySpi *pSpi) {
  trader_spi_ = pSpi;
}

void *ImplBOFtdcQueryApi::callback_onlogin(void *obj) {
  sleep(1);
  ImplBOFtdcQueryApi *self = (ImplBOFtdcQueryApi *)obj;
  CSecurityFtdcRspUserLoginField login_field;
  memset(&login_field, 0, sizeof(login_field));
  self->trader_spi_->OnRspUserLogin(&login_field, NULL, 0, true);  
  return (void *)NULL;
}

void *ImplBOFtdcQueryApi::callback_onlogout(void *obj) {
  sleep(1);
  ImplBOFtdcQueryApi *self = (ImplBOFtdcQueryApi *)obj;
  CSecurityFtdcUserLogoutField logout_field;
  memset(&logout_field, 0, sizeof(logout_field));
  CSecurityFtdcRspInfoField info_field;
  memset(&info_field, 0, sizeof(info_field));
  self->trader_spi_->OnRspUserLogout(&logout_field, &info_field, 0, true);  
  return (void *)NULL;
}

void *ImplBOFtdcQueryApi::callback_onrandcode(void *obj) {
  sleep(1);
  ImplBOFtdcQueryApi *self = (ImplBOFtdcQueryApi *)obj;
  CSecurityFtdcAuthRandCodeField code_field;
  memset(&code_field, 0, sizeof(code_field));
  snprintf(code_field.RandCode, sizeof(code_field.RandCode), "nonsense");
  CSecurityFtdcRspInfoField info_field;
  memset(&info_field, 0, sizeof(info_field));
  self->trader_spi_->OnRspFetchAuthRandCode(&code_field, &info_field, 0, true);
  return (void *)NULL;
}

int ImplBOFtdcQueryApi::ReqUserLogin(
      CSecurityFtdcReqUserLoginField *pReqUserLoginField, int nRequestID) {
  snprintf(user_id_, sizeof(user_id_), "%s", pReqUserLoginField->UserID);
  snprintf(user_passwd_, sizeof(user_passwd_), "%s", pReqUserLoginField->Password);

  char message[1024];
  snprintf(message, sizeof(message), "COMMAND=LOGIN;ACCOUNT=%s", user_id_);
  int ret = send(server_fd_, message, strlen(message) + 1, 0);
  log_file_ << time_now() << "- Send message:" << strlen(message) + 1 << ":" << message <<  endl;
  if (ret < 0) {
    printf("Failed to send message - %d\n", ret);
    return 1;
  }
  return 0;
}

int ImplBOFtdcQueryApi::ReqUserLogout(
      CSecurityFtdcUserLogoutField *pUserLogout, int nRequestID) {
  close(server_fd_);
  pthread_t callback_thread;
  pthread_create(&callback_thread, NULL, callback_onlogout, (void *)this);
  return 0;
}

int ImplBOFtdcQueryApi::ReqFetchAuthRandCode(
      CSecurityFtdcAuthRandCodeField *pAuthRandCode, int nRequestID) {
  pthread_t callback_thread;
  pthread_create(&callback_thread, NULL, callback_onrandcode, (void *)this);
  return 0;
}

CSecurityFtdcInvestorPositionField ImplBOFtdcQueryApi::ToPositionField(
      std::map<std::string, std::string>& properties) {
  CSecurityFtdcInvestorPositionField pos_field;
  memset(&pos_field, 0, sizeof(pos_field));

  string position_type = properties["POSITION_TYPE"];
  string instrument_id = properties["SYMBOL"];
  string investor_id = properties["ACCOUNT_ID"];
  // QUANTITY = current position = yd_pos + buy_td - sell_td
  string quantity = properties["QUANTITY"];
  // AVAILABLE_QTY = yd_pos - sell_td
  string available = properties["AVAILABLE_QTY"];
  // LONG_QTY = buy_td
  string long_qty = properties["LONG_QTY"];
  string short_qty = properties["SHORT_QTY"];
  string today_position = properties["TODAY_POSITION"];
  string average_price = properties["AVERAGE_PRICE"];
  string total_cost = properties["TOTAL_COST"];
  string market_value = properties["MARKET_VALUE"];

  vector<string> ins_exch;
  string readable_symbol = instrument_id;
  string exchange;
  split(instrument_id, ".", ins_exch);
  if (ins_exch.size() == 2) {
    readable_symbol = ins_exch[0];
    exchange = ins_exch[1];
    if (ins_exch[1] == "SH") {
      exchange = "SSE";
    } else if (ins_exch[1] == "SZ") {
      exchange = "SZE";
    }
  }

  snprintf(pos_field.InvestorID, sizeof(pos_field.InvestorID), "%s",
           investor_id.c_str());
  snprintf(pos_field.InstrumentID, sizeof(pos_field.InstrumentID), "%s",
           readable_symbol.c_str());
  snprintf(pos_field.ExchangeID, sizeof(pos_field.ExchangeID), "%s",
           exchange.c_str());
  pos_field.PosiDirection = SECURITY_FTDC_PD_Long;
  // pos_field.YdPosition = atof(quantity.c_str());
  pos_field.YdPosition = atof(available.c_str()) + atof(short_qty.c_str());
  pos_field.TodayPosition = atof(long_qty.c_str());
  pos_field.Position = atof(quantity.c_str());
  pos_field.PositionCost = atof(total_cost.c_str());
  pos_field.StockValue = atof(market_value.c_str());
  pos_field.OpenVolume = atof(long_qty.c_str());
  pos_field.CloseVolume = atof(short_qty.c_str());
  // pos_field.Available = atof(available.c_str());

  return pos_field;
}

CSecurityFtdcTradingAccountField ImplBOFtdcQueryApi::ToAccountField(
      std::map<std::string, std::string>& properties) {
  CSecurityFtdcTradingAccountField acc_field;
  memset(&acc_field, 0, sizeof(acc_field));

  string investor_id = properties["ACCOUNT_ID"];
  string account_type = properties["ACCOUNT_TYPE"];
  string net_worth = properties["RT_ACCOUNT_NET_WORTH"];
  string cash_balance = properties["RT_CASH_BALANCE"];
  string trading_power = properties["RT_TRADING_POWER"];
  string market_value = properties["RT_MARKET_VALUE"];
  string borrow_sell_capital = properties["BORROW_SELL_CAPITAL"];
  string total_debt = properties["TOTAL_DEBT"];
  string borrow_buy_debt = properties["BORROW_BUY_DEBT"];
  string borrow_sell_debt = properties["BORROW_SELL_DEBT"];
  string maintenance_margin = properties["MAINTENANCE_MARGIN"];
  string margin_available = properties["MARGIN_AVAILABLE"];
  string total_credit = properties["TOTAL_CREDIT"];
  string borrow_buy_credit = properties["BORROW_BUY_CREDIT"];
  string borrow_sell_credit = properties["BORROW_SELL_CREDIT"];
  string total_credit_left = properties["TOTAL_CREDIT_LEFT"];
  string buy_credit_left = properties["BORROW_BUY_CREDIT_LEFT"];
  string sell_credit_left = properties["BORROW_SELL_CREDIT_LEFT"];

  snprintf(acc_field.AccountID, sizeof(acc_field.AccountID), "%s", 
           investor_id.c_str());
  if (account_type == "CASH") {
    acc_field.AccountType = SECURITY_FTDC_AcT_Normal;
  } else if (account_type == "MARGIN") {
    acc_field.AccountType = SECURITY_FTDC_AcT_Credit;
  }
  acc_field.PreMortgage = atof(net_worth.c_str());
  acc_field.Balance = atof(total_debt.c_str());
  acc_field.Available = atof(trading_power.c_str());
  acc_field.StockValue = atof(market_value.c_str());
  acc_field.CreditRatio = atof(maintenance_margin.c_str());
  acc_field.ShortSellAmount = atof(borrow_sell_capital.c_str());
  acc_field.PreCredit = atof(borrow_buy_debt.c_str());
  acc_field.PreDeposit = atof(borrow_sell_debt.c_str());
  acc_field.Credit = atof(margin_available.c_str());
  acc_field.CreditAmount = atof(total_credit.c_str());
  acc_field.ShortSellProfit = atof(borrow_sell_credit.c_str());
  acc_field.MarginTradeProfit = atof(borrow_buy_credit.c_str());
  acc_field.SettleMargin = atof(total_credit_left.c_str());
  acc_field.ExchangeMargin = atof(sell_credit_left.c_str());
  acc_field.DeliveryMargin = atof(buy_credit_left.c_str());

  return acc_field;
}

CSecurityFtdcCreditStockAssignInfoField ImplBOFtdcQueryApi::ToMarginStockField(
      map<string, string>& properties) {
  CSecurityFtdcCreditStockAssignInfoField stock_field;
  memset(&stock_field, 0, sizeof(stock_field));

  string account = properties["ACCOUNT_ID"];
  string instrument_id = properties["SYMBOL"];
  string total_margin = properties["TOTAL_MARGIN_QUOTA"];
  string avail_margin = properties["AVAILABLE_MARGIN_QUOTA"];
  string ydvolume = properties["QUANTITY"];
  string tdvolume = properties["TODAY_POSITION"];

  vector<string> ins_exch;
  string readable_symbol = instrument_id;
  string exchange;
  split(instrument_id, ".", ins_exch);
  if (ins_exch.size() == 2) {
    readable_symbol = ins_exch[0];
    exchange = ins_exch[1];
    if (ins_exch[1] == "SH") {
      exchange = "SSE";
    } else if (ins_exch[1] == "SZ") {
      exchange = "SZE";
    }
  }
  snprintf(stock_field.InvestorID, sizeof(stock_field.InvestorID),
           "%s", account.c_str());
  snprintf(stock_field.InstrumentID, sizeof(stock_field.InstrumentID),
           "%s", readable_symbol.c_str());
  snprintf(stock_field.ExchangeID, sizeof(stock_field.ExchangeID),
           "%s", exchange.c_str());
  stock_field.LimitVolume = atoi(total_margin.c_str());
  stock_field.LeftVolume = atoi(avail_margin.c_str());
  stock_field.YDVolume = atoi(ydvolume.c_str());
  stock_field.FrozenVolume = atoi(tdvolume.c_str());

  return stock_field;
}

CSecurityFtdcRspInfoField ImplBOFtdcQueryApi::ToRspField(
      map<string, string>& properties) {
  CSecurityFtdcRspInfoField rsp_field;
  memset(&rsp_field, 0, sizeof(rsp_field));

  rsp_field.ErrorID = 50001;
  snprintf(rsp_field.ErrorMsg, sizeof(rsp_field.ErrorMsg), "Order Insert Failure!");

  return rsp_field;
}

int ImplBOFtdcQueryApi::ReqQryInstrument(
      CSecurityFtdcQryInstrumentField *pQryInstrument, int nRequestID) {
  // TODO
  return 0;
}

int ImplBOFtdcQueryApi::ReqQryTradingAccount(
      CSecurityFtdcQryTradingAccountField *pQryTradingAccount, int nRequestID) {
  char message[1024];
  snprintf(message, sizeof(message), "COMMAND=QUERYACCOUNT;ACCOUNT=%s", pQryTradingAccount->InvestorID);
  int ret = send(server_fd_, message, strlen(message) + 1, 0);
  // printf("Send message:%s\n", message);
  log_file_ << time_now() << "- Send message:" << message << endl;
  if (ret < 0) {
    printf("Failed to send message - %d\n", ret);
    return ret;
  }
  return 0;
}

int ImplBOFtdcQueryApi::ReqQryOFInstrument(
      CSecurityFtdcQryOFInstrumentField *pQryOFInstrument, int nRequestID) {
  // TODO
  return 0;
}

int ImplBOFtdcQueryApi::ReqQryMarketDataStaticInfo(
      CSecurityFtdcQryMarketDataStaticInfoField *pQryMarketDataStaticInfo,
      int nRequestID) {
  // TODO
  return 0;
}

int ImplBOFtdcQueryApi::ReqQryOrder(CSecurityFtdcQryOrderField *pQryOrder,
      int nRequestID) {
  // TODO
  return 0;
}

int ImplBOFtdcQueryApi::ReqQryTrade(CSecurityFtdcQryTradeField *pQryTrade,
      int nRequestID) {
  // TODO
  return 0;
}

int ImplBOFtdcQueryApi::ReqQryInvestorPosition(
      CSecurityFtdcQryInvestorPositionField *pQryInvestorPosition, 
      int nRequestID) {
  char message[1024];
  snprintf(message, sizeof(message), "COMMAND=QUERYPOSITION;ACCOUNT=%s", user_id_);
  int ret = send(server_fd_, message, strlen(message) + 1, 0);
  // printf("Send message:%s\n", message);
  log_file_ << time_now() << "- Send message:" << message << endl;
  if (ret < 0) {
    printf("Failed to send message - %d\n", ret);
    return ret;
  }
  return 0;
}


int ImplBOFtdcQueryApi::ReqQryCreditStockAssignInfo(
      CSecurityFtdcQryCreditStockAssignInfoField *pQryCreditStockAssignInfo, 
      int nRequestID) {
  char message[1024];
  snprintf(message, sizeof(message), "COMMAND=QUERYMARGINSTOCK;ACCOUNT=%s", user_id_);
  int ret = send(server_fd_, message, strlen(message) + 1, 0);
  // printf("Send message:%s\n", message);
  log_file_ << time_now() << "- Send message:" << message << endl;
  if (ret < 0) {
    printf("Failed to send message - %d\n", ret);
    return ret;
  }
  return 0;
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

const char* second_now() {
  static char timestamp_str[32];
  time_t rawtime;
  struct tm *timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);

  snprintf(timestamp_str, sizeof(timestamp_str), 
           "%02d:%02d:%02d", timeinfo->tm_hour,
           timeinfo->tm_min, timeinfo->tm_sec);
  return timestamp_str;
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
