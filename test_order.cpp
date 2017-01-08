#include "test_order.h"

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "error.h"

using namespace std;

static bool checkCond_DirectOrder(strategy *st, string name) {
  TestOrder *to = (TestOrder *)st;
  return to->allow_trading;
}

static double calcPrc__DirectOrder(strategy *st, string name) {
  TestOrder *to = (TestOrder *)st;
  return to->price;
}

TestOrder::TestOrder(const string& name, vector<strategy*>& st, 
      CFtdTrader *trader_list, int ntrader)
      : strategy(name, st, trader_list, ntrader) {
  if (!load_config()) {
    err_exit("%s: failed to load config", name.c_str());
  }
  
  norder_list = new int[ntrader];
  memset(norder_list, 0, sizeof(int) * ntrader);
  order_agents = new OrderAgent*[ntrader];

  for (int i = 0; i < ntrader; i++) {
    const int low = static_cast<int>(lowLimit[instrument_id] / atick_value + 0.5);
    const int high = static_cast<int>(highLimit[instrument_id] / atick_value + 0.5);

    order_agents[i] = new OrderAgent[high - low + 1];
    for (int j = 0; j <= high - low; j++) {
      order_agents[i][j].st = this;
      order_agents[i][j].OAid = i;
      order_agents[i][j].trader = &trader_list[i];
      order_agents[i][j].HedgeFlagType = '1';

      order_agents[i][j].createLimitOrderAgent("directOA", 
            instrument_id, &checkCond_DirectOrder, &calcPrc__DirectOrder,
            Buy, const_cast<char*>("0"), quantity, atick_value);

      trader_list[i].m_OA_OLH[stid][instrument_id][j] = &order_agents[i][j];
      trader_list[i].m_OA_CLH[stid][instrument_id][j] = &order_agents[i][j];
      trader_list[i].m_OA_OSH[stid][instrument_id][j] = &order_agents[i][j];
      trader_list[i].m_OA_CSH[stid][instrument_id][j] = &order_agents[i][j];
    }
  }
}

bool TestOrder::load_config() {
  string file_name = name + "_Config.txt";
  configMap.clear();
  ifstream config_file(file_name.c_str());

  string line;
  while(std::getline(config_file, line)) {
    istringstream config_line(line);
    string key = "";
    if (getline(config_line, key, '=')) {
      string value = "";
      if (getline(config_line, value)) {
        configMap.insert(pair<string, string>(key, value));
      }
    }
  }
  config_file.close();

  symbol_h1_ = configMap["symbol_h1"];
  symbol_a1_ = configMap["symbol_a1"];
  symbol_a2_ = configMap["symbol_a2"];
  beta_lags_[symbol_h1_] = atof(configMap["beta_lag_h1"]);
  beta_lags_[symbol_a1_] = atof(configMap["beta_lag_a1"]);
  beta_lags_[symbol_a2_] = atof(configMap["beta_lag_a2"]);
  beta_lagemas_[symbol_h1_] = atof(configMap["beta_lagema_h1"]);
  beta_lagemas_[symbol_a1_] = atof(configMap["beta_lagema_a1"]);
  beta_lagemas_[symbol_a2_] = atof(configMap["beta_lagema_a2"]);
  beta_bsratios_[symbol_h1_] = atof(configMap["beta_bsratio_h1"]);
  beta_bsratios_[symbol_a1_] = atof(configMap["beta_bsratio_a1"]);
  beta_bsratios_[symbol_a2_] = atof(configMap["beta_bsratio_a2"]);
  multiplier_bbgs_[symbol_h1_] = atof(configMap["multiplier_bbg_h1"]);
  multiplier_bbgs_[symbol_a1_] = atof(configMap["multiplier_bbg_a1"]);
  multiplier_bbgs_[symbol_a2_] = atof(configMap["multiplier_bbg_a2"]);
  lags_[symbol_h1_] = atof(configMap["lag_h1"]);
  lags_[symbol_a1_] = atof(configMap["lag_a1"]);
  lags_[symbol_a2_] = atof(configMap["lag_a2"]);
  emas_[symbol_h1_] = atof(configMap["ema_h1"]);
  emas_[symbol_a1_] = atof(configMap["ema_a1"]);
  emas_[symbol_a2_] = atof(configMap["ema_a2"]);
  offset_issue_ = atoi(configMap["offset_issue"]);
  max_quote_size_ = atoi(configMap["max_quote_size"]);
  qty_ = atoi(configMap["qty"]);
  max_pos_ = atoi(configMap["max_pos"]);
  entry_threshold_ = atof(configMap["entry_threshold"]);
  stepsize_ = atof(configMap["stepsize"]);
  stepsize_spread_ = atof(configMap["stepsize_spread"]);
  stepsize_maintain_ = atof(configMap["stepsize_maintain"]);
  liquidity_filter_ = atof(configMap["liquidity_filter"]);

  max_param_ = max(lags_, emas_);

  time_pos_clear_ = atoi(configMap["time_pos_clear"]);
  rmb_usd_ = atof(configMap["rmb_usd"]);
  is_limit_ = atoi(configMap["is_limit"]);

  atick_value = atof(configMap["AtickValue"].c_str());

  highLimit[instrument_id] = atof(configMap["aHighLimit"].c_str());
  lowLimit[instrument_id] = atof(configMap["aLowLimit"].c_str());
  quantity = atoi(configMap["quantity"].c_str());
  price = atof(configMap["limit_price"].c_str());
  direction = ToBool(configMap["isBuy"])? FTDC_D_Buy : FTDC_D_Sell;
  flag_type = ToBool(configMap["isClose"])? FTDC_OF_CloseToday : FTDC_OF_Open;
  hedge_flag_type = FTDC_CHF_Speculation;
  allow_trading = ToBool(configMap["allowTrading"]);

  return true;
}

void TestOrder::update(DepthMarketDataField *market_data) {
  // update lastest bid and ask
  printf("%s,%lf,%d,%lf,%d\n", market_data->InstrumentID,
        market_data->BidPrice1, market_data->BidVolume1,
        market_data->AskPrice1, market_data->AskVolume1);

  if (strcmp(market_data->InstrumentID, instrument_id.c_str()) == 0) {
    printf("%s.%s update\n", market_data->InstrumentID, 
           market_data->ExchangeID);

    static bool flag = true;
    double price = calcPrc__DirectOrder(this, name);
    const int low = static_cast<int>(lowLimit[instrument_id] / atick_value + 0.5);
    const int price_index = static_cast<int>(price / atick_value + 0.5) - low;

    if (flag) {
      allow_trading = true;
      // order_agents[0][cur_order_id].limit_price = 9776.0;
      order_agents[stid][price_index].actOnce();
    } else {
      allow_trading = false;
      printf("order status:%d\n", order_agents[stid][price_index].OrderStatus);
      if (order_agents[stid][price_index].hasOrder()) {
        order_agents[stid][price_index].cancelOrder();
        printf("has order and cancel\n");
      }
    }

    flag = !flag;
  }

}

void TestOrder::calculateMarketStat() {

}

bool TestOrder::validTradingHour() {
  return true;
}

bool TestOrder::marketEnd() {
  return false;
}

void TestOrder::saveState() {
  
}

void TestOrder::getInstrumentNameMap(map<string, string>& m_instrument) {
  m_instrument.insert(pair<string, string>(instrument_id, instrument_id));
}

register_strategy(TestOrder);
