#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "crush.h"

#include "error.h"

using namespace std;

#define EPS 1e-5

bool equal(double lh, double rh);
bool gt(double lh, double rh);
bool ge(double lh, double rh);
bool lt(double lh, double rh);
bool le(double lh, double rh);
int min(int lh, int rh);
int max(int lh, int rh);
int signum(int val);
double dabs(double val);  // absolute of double
int hhmmss();

static bool trade_condition(strategy *st, string name) {
  Crush *cr = (Crush *)st;
  return cr->AllowTrade();
}

static double get_long_market_price(strategy *st, string name) {
  Crush *cr = (Crush *)st;
  return cr->long_market_price_;
}

static double get_short_market_price(strategy *st, string name) {
  Crush *cr = (Crush *)st;
  return cr->short_market_price_;
}

static double get_long_limit_price(strategy *st, string name) {
  Crush *cr = (Crush *)st;
  return cr->long_limit_price_;
}

static double get_short_limit_price(strategy *st, string name) {
  Crush *cr = (Crush *)st;
  return cr->short_limit_price_;
}

Crush::Crush(const string& name, vector<strategy*>& st, 
        CFtdTrader *trader_list, int ntrader)
        : strategy(name, st, trader_list, ntrader) {
  if (!load_config()) {
    err_exit("%s: failed to load config", name.c_str());
  }

  state_ = STATE_ON;
  sec_count_ = 0;
  pos_change_ = 0;
  ask_h1_ = bid_h1_ = 0.0;
  mid_h1_ = mid_a1_ = mid_a2_ = mid_prv_h1_ = mid_prv_a1_ = mid_prv_a2_ = 0.0;
  lag_h1_ = lag_a1_ = lag_a2_ = lagema_h1_ = lagema_a1_ = lagema_a2_ = 0.0;
  lagema_prv_h1_ = lagema_prv_a1_ = lagema_a2_ = 0.0;
  ind_score_ = 0.0;
  limit_position_clear_ = false;

  // market data information
  latest_ask_h1_ = latest_ask_a1_ = latest_ask_a2_ = 0.0;
  latest_bid_h1_ = latest_bid_a1_ = latest_bid_a2_ = 0.0;
  latest_last_h1_ = latest_last_a1_ = latest_last_a2_ = 0.0;
  latest_asize_h1_ = latest_asize_a1_ = latest_asize_a2_ = 0;
  latest_bsize_h1_ = latest_bsize_a1_ = latest_bsize_a2_ = 0;
  latest_lsize_h1_ = latest_lsize_a1_ = latest_lsize_a2_ = 0;

  // trading information
  last_trade_price_h1_ = last_trade_price_a1_ = last_trade_price_a2_ = 0.0;
  has_last_h1_ = has_last_a1_ = has_last_a2_ = false;
  trade_long_volume_h1_ = trade_long_volume_a1_ = trade_long_volume_a2_ = 0;
  trade_short_volume_h1_ = trade_short_volume_a1_ = trade_short_volume_a2_ = 0;


  open_long_order_.createLimitOrderAgent("OL", symbol_h1_, 
        &trade_condition, &get_long_market_price, Buy,
        const_cast<char*>("0"), 0, tick_size_h1_);
  open_short_order_.createLimitOrderAgent("OS", symbol_h1_, 
        &trade_condition, &get_short_market_price, Sell,
        const_cast<char*>("0"), 0, tick_size_h1_);
  open_long_limit_order_.createLimitOrderAgent("OL_Limit", symbol_h1_, 
        &trade_condition, &get_long_limit_price, Buy,
        const_cast<char*>("0"), 0, tick_size_h1_);
  open_short_limit_order_.createLimitOrderAgent("OS_Limit", symbol_h1_, 
        &trade_condition, &get_short_limit_price, Sell,
        const_cast<char*>("0"), 0, tick_size_h1_);

  int ret = pthread_create(&param_update_thread_, NULL, update_param, 
            (void *)this);
  if (ret != 0) {
    err_exit("%s: failed to create param udpate thread", name.c_str());
  }
  ret = pthread_create(&timer_event_thread_, NULL, timer_event, (void *)this);
  if (ret != 0) {
    err_exit("%s: failed to create timer event thread", name.c_str());
  }
}

bool Crush::load_config() {
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
  beta_lag_h1_ = atof(configMap["beta_lag_h1"].c_str());
  beta_lag_a1_ = atof(configMap["beta_lag_a1"].c_str());
  beta_lag_a2_ = atof(configMap["beta_lag_a2"].c_str());
  beta_lagema_h1_ = atof(configMap["beta_lagema_h1"].c_str());
  beta_lagema_a1_ = atof(configMap["beta_lagema_a1"].c_str());
  beta_lagema_a2_ = atof(configMap["beta_lagema_a2"].c_str());
  beta_bsratio_h1_ = atof(configMap["beta_bsratio_h1"].c_str());
  beta_bsratio_a1_ = atof(configMap["beta_bsratio_a1"].c_str());
  beta_bsratio_a2_ = atof(configMap["beta_bsratio_a2"].c_str());
  multiplier_bbg_h1_ = atof(configMap["multiplier_bbg_h1"].c_str());
  multiplier_bbg_a1_ = atof(configMap["multiplier_bbg_a1"].c_str());
  multiplier_bbg_a2_ = atof(configMap["multiplier_bbg_a2"].c_str());
  lag_len_h1_ = atoi(configMap["lag_len_h1"].c_str());
  lag_len_a1_ = atoi(configMap["lag_len_a1"].c_str());
  lag_len_a2_ = atoi(configMap["lag_len_a2"].c_str());
  ema_len_h1_ = atoi(configMap["ema_len_h1"].c_str());
  ema_len_a1_ = atoi(configMap["ema_len_a1"].c_str());
  ema_len_a2_ = atoi(configMap["ema_len_a2"].c_str());
  liquidity_filter_ = atof(configMap["liquidity_filter"].c_str());
  offset_issue_ = atoi(configMap["offset_issue"].c_str());
  max_quote_size_ = atoi(configMap["max_quote_size"].c_str());
  qty_ = atoi(configMap["qty"].c_str());
  entry_threshold_ = atof(configMap["entry_threshold"].c_str());
  stepsize_ = atof(configMap["stepsize"].c_str());
  stepsize_spread_ = atof(configMap["stepsize_spread"].c_str());
  stepsize_maintain_ = atof(configMap["stepsize_maintain"].c_str());
  max_pos_ = atoi(configMap["max_pos"].c_str());

  time_pos_clear_ = atoi(configMap["time_pos_clear"].c_str());
  rmb_usd_ = atof(configMap["rmb_usd"].c_str());
  is_limit_ = atoi(configMap["is_limit"].c_str());

  tick_size_h1_ = atof(configMap["tick_size_h1"].c_str());
  tick_size_a1_ = atof(configMap["tick_size_a1"].c_str());
  tick_size_a2_ = atof(configMap["tick_size_a2"].c_str());
  point_value_h1_ = atof(configMap["point_value_h1"].c_str());

  int max_lag_len = max(lag_len_h1_, max(lag_len_a1_, lag_len_a2_));
  int max_ema_len = max(ema_len_h1_, max(ema_len_a1_, ema_len_a2_));
  max_param_len_ = max(max_lag_len, max_ema_len);

  return true;
}

void Crush::update(DepthMarketDataField *market_data) {
  if (strcmp(market_data->InstrumentID, symbol_h1_.c_str()) == 0 ||
      strcmp(market_data->InstrumentID, symbol_a1_.c_str()) == 0 ||
      strcmp(market_data->InstrumentID, symbol_a2_.c_str()) == 0) {
    // update latest market data
    if (strcmp(market_data->InstrumentID, symbol_h1_.c_str()) == 0) {
      latest_ask_h1_ = market_data->AskPrice1;
      latest_bid_h1_ = market_data->BidPrice1;
      latest_last_h1_ = market_data->LastPrice;
      latest_asize_h1_ = market_data->AskVolume1;
      latest_bsize_h1_ = market_data->BidVolume1;
      latest_lsize_h1_ = market_data->Volume;
    } else if (strcmp(market_data->InstrumentID, symbol_a1_.c_str()) == 0) {
      latest_ask_a1_ = market_data->AskPrice1;
      latest_bid_a1_ = market_data->BidPrice1;
      latest_last_a1_ = market_data->LastPrice;
      latest_asize_a1_ = market_data->AskVolume1;
      latest_bsize_a1_ = market_data->BidVolume1;
      latest_lsize_a1_ = market_data->Volume;
    } if (strcmp(market_data->InstrumentID, symbol_a2_.c_str()) == 0) {
      latest_ask_a2_ = market_data->AskPrice1;
      latest_bid_a2_ = market_data->BidPrice1;
      latest_last_a2_ = market_data->LastPrice;
      latest_asize_a2_ = market_data->AskVolume1;
      latest_bsize_a2_ = market_data->BidVolume1;
      latest_lsize_a2_ = market_data->Volume;
    }

    if (Tradable() && IsTrading() &&
        !equal(GetBidPrice(symbol_h1_), 0.0) &&
        !equal(GetAskPrice(symbol_h1_), 0.0) &&
        !equal(GetBidPrice(symbol_a1_), 0.0) &&
        !equal(GetAskPrice(symbol_a1_), 0.0) &&
        !equal(GetBidPrice(symbol_a2_), 0.0) &&
        !equal(GetAskPrice(symbol_a2_), 0.0)) {
      mid_h1_ = GetMid(symbol_h1_);
      mid_a1_ = GetMid(symbol_a1_);
      mid_a2_ = GetMid(symbol_a2_);

      if (gt(mid_prv_h1_, 0.0)) {
        lag_h1_ = (mid_h1_ - mid_prv_h1_)/mid_prv_h1_;
        lagema_h1_ = 2/ema_len_h1_*(lag_h1_ - lagema_prv_h1_) + lagema_prv_h1_;
      }
      if (gt(mid_prv_a1_, 0.0)) {
        lag_a1_ = (mid_a1_ - mid_prv_a1_)/mid_prv_a1_;
        lagema_a1_ = 2/ema_len_a1_*(lag_a1_ - lagema_prv_a1_) + lagema_prv_a1_;
      }
      if (gt(mid_prv_a2_, 0.0)) {
        lag_a2_ = (mid_a2_ - mid_prv_a2_)/mid_prv_a2_;
        lagema_a2_ = 2/ema_len_a2_*(lag_a2_ - lagema_prv_a2_) + lagema_prv_a2_;
      }

      if (sec_count_ > max_param_len_) {
        ind_score_ = beta_lag_h1_*lag_h1_ + beta_lag_a1_*lag_a1_ + 
                     beta_lag_a2_*lag_a2_ + beta_lagema_h1_*lagema_h1_ +
                     beta_lagema_a1_*lagema_a1_ +
                     beta_lagema_a2_*lagema_a2_ +
                     beta_bsratio_h1_*GetBSRatio(symbol_h1_) +
                     beta_bsratio_a1_*GetBSRatio(symbol_a1_) +
                     beta_bsratio_a2_*GetBSRatio(symbol_a2_);
      } else {
        ind_score_ = 0.0;
      }

      update_pos_and_send();
    }

    // LimitUp & LimitDown
    if (Tradable(symbol_h1_) && IsTrading()) {
      double ask_price_a1 = GetAskPrice(symbol_a1_);
      double bid_price_a1 = GetBidPrice(symbol_a1_);
      double ask_price_a2 = GetAskPrice(symbol_a2_);
      double bid_price_a2 = GetBidPrice(symbol_a2_);

      if (!limit_stop_ && gt(bid_price_a1, 0.0) &&
          gt(ask_price_a1, 0.0) &&
          gt(bid_price_a2, 0.0) &&
          gt(ask_price_a2, 0.0) &&
          (lt(bid_price_a1 - limit_down_a1_, 10*tick_size_a1_) ||
           lt(limit_up_a1_ - ask_price_a1, 10*tick_size_a1_) ||
           lt(bid_price_a2 - limit_down_a2_, 10*tick_size_a2_) ||
           lt(limit_up_a2_ - ask_price_a2, 10*tick_size_a2_))) {
        limit_stop_ = true;
        if (GetNetPos(symbol_h1_) >= 0 && 
            (lt(bid_price_a1 - limit_down_a1_, 10*tick_size_a1_) ||
             lt(limit_up_a1_ - ask_price_a1, 10*tick_size_a1_) ||
             lt(bid_price_a2 - limit_down_a2_, 10*tick_size_a2_) ||
             lt(limit_up_a2_ - ask_price_a2, 10*tick_size_a2_))) {
          limit_position_clear_ = true;
        }
      } else if (limit_stop_ && 
         (gt(bid_price_a1 - limit_down_a1_, 20*tick_size_a1_) ||
          gt(limit_up_a1_ - ask_price_a1, 20*tick_size_a1_) ||
          gt(bid_price_a2 - limit_down_a2_, 20*tick_size_a2_) ||
          gt(limit_up_a2_ - ask_price_a2, 20*tick_size_a2_))) {
        limit_stop_ = false;
        limit_position_clear_ = false;
      }  // in 20-10 ticks close to limit-up/down
    }
  }
}

void Crush::OnOrderAgentAllTraded(const char *InstrumentID, int nOrderRef,
      double LimitPrice, int Volume, int OAid) {
  strategy::OnOrderAgentAllTraded(InstrumentID, nOrderRef, LimitPrice,
        Volume, OAid);
  if (strcmp(InstrumentID, symbol_h1_.c_str()) != 0) {
    return;
  }
  
  has_last_h1_ = true;
  last_trade_price_h1_ = LimitPrice;

  map<int, OrderAgent*>& ref2OA = orderMap[OAid][InstrumentID];
  const map<int, OrderAgent*>::iterator& it = ref2OA.find(nOrderRef);

  if (it != ref2OA.end()) {
    OrderAgent *pOrder = it->second;
    if (pOrder->direction == Buy) {
      bid_h1_ = GetBidPrice(symbol_h1_);
      ask_h1_ = LimitPrice + tick_size_h1_;
      trade_long_volume_h1_ += Volume;
    } else {
      bid_h1_ = LimitPrice - tick_size_h1_;
      ask_h1_ = GetAskPrice(symbol_h1_);
      trade_short_volume_h1_ += Volume;
    }
  }
}

void *Crush::update_param(void *obj) {
  Crush *self = (Crush *)obj;
  while (true) {
    sleep(1);
    if (self->Tradable() && !equal(self->GetAskPrice(self->symbol_h1_), 0.0) &&
        !equal(self->GetBidPrice(self->symbol_h1_), 0.0) &&
        !equal(self->GetAskPrice(self->symbol_a1_), 0.0) &&
        !equal(self->GetBidPrice(self->symbol_a1_), 0.0) &&
        !equal(self->GetAskPrice(self->symbol_a2_), 0.0) &&
        !equal(self->GetBidPrice(self->symbol_a2_), 0.0)) {
      self->sec_count_++;

      self->mid_list_h1_.push_back(self->mid_h1_);
      self->mid_list_a1_.push_back(self->mid_a1_);
      self->mid_list_a2_.push_back(self->mid_a2_);
      self->lagema_list_h1_.push_back(self->lagema_h1_);
      self->lagema_list_a1_.push_back(self->lagema_a1_);
      self->lagema_list_a2_.push_back(self->lagema_a2_);

      int len_h1 = self->mid_list_h1_.size();
      if (len_h1 > self->lag_len_h1_) {
        self->mid_prv_h1_ = self->mid_list_h1_[0];
        self->lagema_prv_h1_ = self->lagema_list_h1_[0];
        self->mid_list_h1_.erase(self->mid_list_h1_.begin());
        self->lagema_list_h1_.erase(self->lagema_list_h1_.begin());
      }

      int len_a1 = self->mid_list_a1_.size();
      if (len_a1 > self->lag_len_a1_) {
        self->mid_prv_a1_ = self->mid_list_a1_[0];
        self->lagema_prv_a1_ = self->lagema_list_a1_[0];
        self->mid_list_a1_.erase(self->mid_list_a1_.begin());
        self->lagema_list_a1_.erase(self->lagema_list_a1_.begin());
      }

      int len_a2 = self->mid_list_a2_.size();
      if (len_a2 > self->lag_len_a2_) {
        self->mid_prv_a2_ = self->mid_list_a2_[0];
        self->lagema_prv_a2_ = self->lagema_list_a2_[0];
        self->mid_list_a2_.erase(self->mid_list_a2_.begin());
        self->lagema_list_a2_.erase(self->lagema_list_a2_.begin());
      }
    } else if (!self->Tradable() || self->PositionClear()) {
      self->open_long_limit_order_.cancelOrder();
      self->open_short_limit_order_.cancelOrder();
    }

    if (self->PositionClear() && self->GetNetPos(self->symbol_h1_) != 0) {
      self->update_pos_and_send();
    }

    // check here to trigger the PositionLimit script in RTS
    if (self->IsPositionLimit()) {
      int net_pos = self->GetNetPos(self->symbol_h1_);
      if (net_pos >= self->max_pos_*self->qty_) {
        self->open_long_limit_order_.cancelOrder();
      } else if (net_pos <= -1*self->max_pos_*self->qty_) {
        self->open_short_limit_order_.cancelOrder();
      }
    }
  }  // while
  return NULL;
}

void *Crush::timer_event(void *obj) {
  Crush *self = (Crush *)obj;
  OrderAgent *order_agent;
  Crush *st;
  while(true) {
    order_agent = self->cancel_timer_.pop();
    if (order_agent != NULL) {
      if (order_agent->hasOrder()) {
        order_agent->cancelOrder();
      }
    }

    st = self->clear_timer_.pop();
    if (st != NULL) {
      if (st->PositionClear() && st->GetNetPos(st->symbol_h1_) != 0) {
        st->update_pos_and_send();
      }
    }
    usleep(100);
  }
  return NULL;
}

void Crush::update_pos_and_send() {
  if (!IsTrading() || !AllowTrade()) {
    return;
  }
  // Target position update
  pos_change_ = 0;
  double score_adjust = GetScoreAdjust();
  
  if (Tradable() && !PositionClear()) {
    // !oTxExists("OL") && !oTxExists("OS") && !openLong.hasOrder() &&
    // !openShort.hasOrder()
    if (!open_long_order_.hasOrder() && !open_short_order_.hasOrder()) {
      if (gt(score_adjust, entry_threshold_)) {
        int net_pos = GetNetPos(symbol_h1_);
        pos_change_ = (net_pos >= max_pos_*qty_)? 0 : \
                      min(qty_, max_pos_*qty_ - net_pos);
      } else if (lt(score_adjust, -1.0*entry_threshold_)) {
        int net_pos = GetNetPos(symbol_h1_);
        pos_change_ = (net_pos <= -1*max_pos_*qty_) ? 0 : \
                      -1*min(qty_, max_pos_*qty_ + net_pos);
      }
    }
  } else if (PositionClear()) {
    // Cancel limit order
    open_long_limit_order_.cancelOrder();
    open_short_limit_order_.cancelOrder();
    int net_pos = GetNetPos(symbol_h1_);
    if (net_pos != 0 && MarketEnd()/* && trigger_pos_clear>=1*/) {
      pos_change_ = -1*min(qty_, abs(net_pos))*signum(net_pos);
      clear_timer_.push(this, time_pos_clear_);
    } else if (net_pos != 0 && !MarketEnd()) {
      pos_change_ = -1*min(qty_, abs(net_pos))*signum(net_pos);
    }
  }

  // Order Issue
  if (pos_change_ != 0) {
    // Cancel limit order
    open_long_limit_order_.cancelOrder();
    open_short_limit_order_.cancelOrder();
    // Issue market order
    // !oTxExists("OL") && !oTxExists("OS") && !openLong.hasOrder() &&
    // !openShort.hasOrder()
    if (!open_long_order_.hasOrder() && !open_short_order_.hasOrder()) {
      if (pos_change_ > 0) {
        int volume = min(abs(pos_change_), max_quote_size_);
        int offset_order = PositionClear()? offset_issue_ : \
              max(0, ((int)(dabs(score_adjust) - entry_threshold_)) \
              /stepsize_spread_);
        double price = GetAskPrice(symbol_h1_) + offset_order*tick_size_h1_;
        long_market_price_ = price;
        open_long_order_.quantity = volume;
        open_long_order_.actOnce();
        cancel_timer_.push(&open_long_order_, 300);  // 0.3 second to cancel
      } else if (pos_change_ < 0) {
        int volume = min(abs(pos_change_), max_quote_size_);
        int offset_order = PositionClear()? offset_issue_ : \
              max(0 , ((int)(dabs(score_adjust) - entry_threshold_)) \
              /stepsize_spread_);
        double price = GetBidPrice(symbol_h1_) - offset_order*tick_size_h1_;
        short_market_price_ = price;
        open_short_order_.quantity = volume;
        open_short_order_.actOnce();
        cancel_timer_.push(&open_short_order_, 300);  // 0.3 second to cancel
      }
    }
  } else if (is_limit_ && pos_change_ == 0 && sec_count_ > max_param_len_ &&
      !PositionClear() &&(!open_long_order_.hasOrder() &&
      !open_short_order_.hasOrder())) {
    double bid_price = GetBidPrice(symbol_h1_);
    double ask_price = GetAskPrice(symbol_h1_);
    // Long order
    int offset_ol_limit = min(-1, (int)((score_adjust - entry_threshold_)\
          /stepsize_spread_));
    int offset_ol_maintain = min(-1, (int)((score_adjust - entry_threshold_ +
          stepsize_maintain_)/stepsize_spread_));
    double price_ol_limit = ask_price + offset_ol_limit*tick_size_h1_;
    double price_ol_maintain = ask_price + offset_ol_maintain*tick_size_h1_;
    long_limit_price_ = open_long_limit_order_.hasOrder() ? long_limit_price_ :
          price_ol_limit;
    // Short order
    int offset_os_limit = max(1, (int)((score_adjust + entry_threshold_)\
          /stepsize_spread_ + 1.0));
    int offset_os_maintain = max(1, (int)((score_adjust + entry_threshold_ - \
          stepsize_maintain_)/stepsize_spread_ + 1.0));
    double price_os_limit = bid_price + offset_os_limit*tick_size_h1_;
    double price_os_maintain = bid_price + offset_os_maintain*tick_size_h1_;
    short_limit_price_ = open_short_limit_order_.hasOrder() ? \
          short_limit_price_ : price_os_limit;
    if (offset_ol_maintain < -2) {
      open_long_limit_order_.cancelOrder();
    }
    if (offset_os_maintain > 2) {
      open_short_limit_order_.cancelOrder();
    }
    // Order Issue or Update
    if (lt(price_ol_limit, price_os_limit) && \
        lt(price_ol_limit, short_limit_price_) && \
        lt(long_limit_price_, price_os_limit)) {
      int net_pos = GetNetPos(symbol_h1_);
      if (offset_ol_maintain >= -2 && net_pos < max_pos_*qty_ && \
          (!open_short_limit_order_.hasOrder() || \
            gt(price_ol_limit, long_limit_price_) || \
            (lt(price_ol_limit, long_limit_price_) && \
             lt(price_ol_maintain, long_limit_price_)))) {
        open_long_limit_order_.cancelOrder();
        int volume = min(min(max_pos_*qty_ - net_pos, qty_), max_quote_size_);
        open_long_limit_order_.quantity = volume;
        open_long_limit_order_.actOnce();
      }
      if (offset_os_maintain <= 2 && net_pos > -1*max_pos_*qty_ && \
          (!open_short_limit_order_.hasOrder() || \
            lt(price_os_limit, short_limit_price_) || \
            (gt(price_os_limit, short_limit_price_) && \
             gt(price_os_maintain, short_limit_price_)))) {
        open_short_limit_order_.cancelOrder();
        int volume = min(min(max_pos_*qty_ + net_pos, qty_), max_quote_size_);
        open_short_limit_order_.quantity = volume;
        open_short_limit_order_.actOnce();
      }
    }
  }
}

// Utilities functions
bool Crush::Tradable(const string &symbol) {
  // TODO
  // return mPhase(symbol) == "TRADE" && mBidDepth(symbol) > 0
  // && mAskDepth(symbol) > 0 && !mBidIsUnlimited(symbol, 1)
  // && !mAskIsUnlimited(symbol, 1) && mBidPrice(symbol, 1) >
  // mAskPrice(symbol, 1)
  return true;
}

bool Crush::Tradable() {
  return Tradable(symbol_h1_) && Tradable(symbol_a1_) && Tradable(symbol_a2_);
}

bool Crush::IsTrading() {
  int now = hhmmss();
  return (now >= 900000 && now <= 101500) ||
         (now >= 103000 && now <= 113000) ||
         (now >= 133000 && now < 160000);
}

bool Crush::AllowTrade() {
  return lt(GetSpread(symbol_h1_), liquidity_filter_*tick_size_h1_) &&
         gt(GetSpread(symbol_h1_), 0.0) && state_ == STATE_ON;
}

bool Crush::IsPositionLimit() {
  return abs(GetNetPos(symbol_h1_)) >= max_pos_*qty_;
}

bool Crush::MarketEnd() {
  int now = hhmmss();
  return now > 145500 && now < 210000;
}

bool Crush::PositionClear() {
  return (MarketEnd() || limit_position_clear_);
}

int Crush::GetBidSize(const string &symbol) {
  if (Tradable(symbol)) {
    if (symbol == symbol_h1_) {
      return latest_bsize_h1_;
    } else if (symbol == symbol_a1_) {
      return latest_bsize_a1_;
    } else if (symbol == symbol_a2_) {
      return latest_bsize_a2_;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}

int Crush::GetAskSize(const string &symbol) {
  if (Tradable(symbol)) {
    if (symbol == symbol_h1_) {
      return latest_asize_h1_;
    } else if (symbol == symbol_a1_) {
      return latest_asize_a1_;
    } else if (symbol == symbol_a2_) {
      return latest_asize_a2_;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}

double Crush::GetAskPrice(const string &symbol) {
  if (Tradable(symbol)) {
    if (symbol == symbol_h1_) {
      return latest_ask_h1_;
    } else if (symbol == symbol_a1_) {
      return latest_ask_a1_;
    } else if (symbol == symbol_a2_) {
      return latest_ask_a2_;
    } else {
      return 0;
    }
  } else {
    if (symbol == symbol_h1_) {
      return ask_h1_;
    } else {
      return 0.0;
    }
  }
}

double Crush::GetBidPrice(const string &symbol) {
  if (Tradable(symbol)) {
    if (symbol == symbol_h1_) {
      return latest_bid_h1_;
    } else if (symbol == symbol_a1_) {
      return latest_bid_a1_;
    } else if (symbol == symbol_a2_) {
      return latest_bid_a2_;
    } else {
      return 0;
    }
  } else {
    if (symbol == symbol_h1_) {
      return bid_h1_;
    } else {
      return 0.0;
    }
  }
}

double Crush::GetLastPrice(const string &symbol) {
  if (symbol == symbol_h1_) {
    if (has_last_h1_) {
      return last_trade_price_h1_;
    } else {
      return GetMidPrice(symbol_h1_);
    }
  } else if (symbol == symbol_a1_) {
    if (has_last_a1_) {
      return last_trade_price_a1_;
    } else {
      return GetMidPrice(symbol_a1_);
    }
  } else if (symbol == symbol_a2_) {
    if (has_last_a2_) {
      return last_trade_price_a2_;
    } else {
      return GetMidPrice(symbol_a2_);
    }
  } else {
    return 0.0;
  }
}

double Crush::GetMidPrice(const string &symbol) {
  return 0.5 * (GetAskPrice(symbol) + GetBidPrice(symbol));
}

double Crush::GetMid(const string &symbol) {
  double multiplier = 1.0;
  if (symbol == symbol_h1_) {
    multiplier = multiplier_bbg_h1_;
  } else if (symbol == symbol_a1_) {
    multiplier = multiplier_bbg_a1_;
  } else if (symbol == symbol_a2_) {
    multiplier = multiplier_bbg_a2_;
  }
  return GetMidPrice(symbol) * multiplier;
}

double Crush::GetSpread(const string &symbol) {
  return GetAskPrice(symbol) - GetBidPrice(symbol);
}

int Crush::GetVolume(const string &symbol) {
  if (Tradable(symbol)) {
    // return /* mTotalQty(symbol) */
    if (symbol == symbol_h1_) {
      return trade_long_volume_h1_ + trade_short_volume_h1_;
    } else if (symbol == symbol_a1_) {
      return trade_long_volume_a1_ + trade_short_volume_a1_;
    } else if (symbol == symbol_a2_) {
      return trade_long_volume_a2_ + trade_short_volume_a2_;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}

double Crush::GetBSRatio(const string &symbol) {
  if (Tradable(symbol)) {
    int bid_size = GetBidSize(symbol);
    int ask_size = GetAskSize(symbol);
    if (bid_size + ask_size == 0) {
      return 0.0;
    } else {
      return (double)(bid_size - ask_size) / (double)(bid_size + ask_size);
    }
  } else {
    return 0.0;
  }
}

int Crush::GetLongVolume(const string &symbol) {
  if (symbol == symbol_h1_) {
    return trade_long_volume_h1_;
  } else if (symbol == symbol_a1_) {
    return trade_long_volume_a1_;
  } else if (symbol == symbol_a2_) {
    return trade_long_volume_a2_;
  } else {
    return 0;
  }
}

int Crush::GetShortVolume(const string &symbol) {
  if (symbol == symbol_h1_) {
    return trade_short_volume_h1_;
  } else if (symbol == symbol_a1_) {
    return trade_short_volume_a1_;
  } else if (symbol == symbol_a2_) {
    return trade_short_volume_a2_;
  } else {
    return 0;
  }
}

int Crush::GetNetPos(const string &symbol) {
  return GetLongVolume(symbol) - GetShortVolume(symbol);
}

double Crush::GetScoreAdjust() {
  return ind_score_ - GetNetPos(symbol_h1_)/qty_*stepsize_;
}


// calculating functions
bool equal(double lh, double rh) {
  if (lh > rh) {
    return lh - rh < EPS;
  } else {
    return rh - lh < EPS;
  }
}

bool gt(double lh, double rh) {
  if (equal(lh, rh)) {
    return false;
  } else {
    return lh > rh;
  }
}

bool ge(double lh, double rh) {
  if (equal(lh, rh)) {
    return true;
  } else {
    return lh > rh;
  }
}

bool lt(double lh, double rh) {
  if (equal(lh, rh)) {
    return false;
  } else {
    return lh < rh;
  }
}

bool le(double lh, double rh) {
  if (equal(lh, rh)) {
    return true;
  } else {
    return lh < rh;
  }
}

int min(int lh, int rh) {
  return lh < rh ? lh : rh;
}

int max(int lh, int rh) {
  return lh > rh ? lh : rh;
}

int signum(int val) {
  return val < 0 ? -1 : 1;
}

double dabs(double val) {
  return val < 0.0 ? -1.0*val : val;
}

int hhmmss() {
  time_t now;
  time(&now);
  struct tm *tm_now = localtime(&now);
  return (tm_now->tm_hour + 1)*10000 + (tm_now->tm_min + 1)*100 +
          tm_now->tm_sec + 1;
}
