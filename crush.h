#ifndef __CRUSH_H__
#define __CRUSH_H__

#include <string>
#include <vector>
#include <map>

#include <pthread.h>

#include <strategy.h>
#include <timer_queue.hpp>

class Crush : public strategy {
 public:
  static const int STATE_OFF = 0;
  static const int STATE_ON  = 1;

  int state_;
  // constance section
  account *account_h1_;
  account *account_a1_;
  account *account_a2_;
  std::string symbol_h1_;
  std::string symbol_a1_;
  std::string symbol_a2_;

  int lag_len_h1_, lag_len_a1_, lag_len_a2_;  // LagParam_H1/A1/2
  int ema_len_h1_, ema_len_a1_, ema_len_a2_;  // EMAParam_H1/A1/2
  double beta_lag_h1_, beta_lag_a1_, beta_lag_a2_;  // beta_lag_H1/A1/A2
  double beta_lagema_h1_, beta_lagema_a1_, beta_lagema_a2_;  // beta_lagema_
  double beta_bsratio_h1_, beta_bsratio_a1_, beta_bsratio_a2_;  // bsration_
  double multiplier_bbg_h1_, multiplier_bbg_a1_, multiplier_bbg_a2_; //

  int market_type_;
  int nday_;
  int offset_issue_;
  int max_quote_size_;
  int qty_;
  int max_pos_;
  int max_param_len_;  // max param among lag_len_ and ema_len_
  double liquidity_filter_;
  double entry_threshold_;
  double stepsize_;
  double stepsize_spread_;
  double stepsize_maintain_;

  int time_pos_clear_;  // interval time to clear position
  double rmb_usd_;
  bool is_limit_;  // indicates whether it is a limit order

  // lastest market data information
  double latest_ask_h1_, latest_ask_a1_, latest_ask_a2_;
  double latest_bid_h1_, latest_bid_a1_, latest_bid_a2_;
  double latest_last_h1_, latest_last_a1_, latest_last_a2_;
  int latest_asize_h1_, latest_asize_a1_, latest_asize_a2_;
  int latest_bsize_h1_, latest_bsize_a1_, latest_bsize_a2_;
  int latest_lsize_h1_, latest_lsize_a1_, latest_lsize_a2_;

  // trading information
  double last_trade_price_h1_, last_trade_price_a1_, last_trade_price_a2_;
  bool has_last_h1_, has_last_a1_, has_last_a2_;
  int trade_long_volume_h1_, trade_long_volume_a1_, trade_long_volume_a2_;
  int trade_short_volume_h1_, trade_short_volume_a1_, trade_short_volume_a2_;

  double tick_size_h1_;
  double tick_size_a1_;
  double tick_size_a2_;
  double point_value_h1_;
  double ask_h1_, bid_h1_;
  double mid_h1_, mid_a1_, mid_a2_;  // mid_H1/A1/2
  double mid_prv_h1_, mid_prv_a1_, mid_prv_a2_;  // mid_prv_H1/A1/2
  double lag_h1_, lag_a1_, lag_a2_;  // lag_H1/A1/2
  double lagema_h1_, lagema_a1_, lagema_a2_;
  double lagema_prv_h1_, lagema_prv_a1_, lagema_prv_a2_;  // lagema_prv_
  // v_mid_H1/A1/2 in RTS
  std::vector<double> mid_list_h1_, mid_list_a1_, mid_list_a2_;
  // v_lagema_H1/A1/2 in RTS
  std::vector<double> lagema_list_h1_, lagema_list_a1_, lagema_list_a2_;


  // variable section
  std::string trading_time_;  // self
  int trading_date_start_;  // self
  int trading_date_end_;  // self
  int sec_count_;  // self
  double ind_score_;  // ind_socre_
  bool trigger_pos_clear_;  // self
  bool limit_position_clear_;  // self
  bool limit_stop_;  // self
  int pos_change_;  // pos_change
  int limit_up_a1_;  // self
  int limit_down_a1_;  // self
  int limit_up_a2_;  // self
  int limit_down_a2_;  // self

  // OrderAgent **order_agents;
  OrderAgent open_long_order_;
  OrderAgent open_short_order_;
  OrderAgent open_long_limit_order_;
  OrderAgent open_short_limit_order_;

  double long_market_price_;
  double short_market_price_;
  double long_limit_price_;
  double short_limit_price_;

  TimerQueue<OrderAgent> cancel_timer_;  // timer to cancel OrderAgent
  TimerQueue<Crush> clear_timer_;  // timer to clear position

  pthread_t param_update_thread_;
  pthread_t timer_event_thread_;

  Crush(const std::string& name, std::vector<strategy*>& st, 
        CFtdTrader *trader_list, int ntrader);
  bool load_config();
  void update(DepthMarketDataField *market_data);
  void OnOrderAgentAllTraded(const char *InstrumentID, int nOrderRef,
        double LimitPrice, int Volume, int OAid);

  static void *update_param(void *obj);
  static void *timer_event(void *obj);
  void update_pos_and_send();

  // Utilities functions
  bool Tradable(const std::string &symbol);
  bool Tradable();
  bool IsTrading();
  bool AllowTrade();
  bool IsPositionLimit();
  bool MarketEnd();
  bool PositionClear();  // positionClear in RTS

 private:
  // Utilities functions

  int GetBidSize(const std::string &symbol);  // BidSize_H1/A1/2 in RTS
  int GetAskSize(const std::string &symbol);  // AskSize_H1/A1/2 in RTS

  double GetAskPrice(const std::string &symbol);  // AskPrice_H1/A1/2 in RTS
  double GetBidPrice(const std::string &symbol);  // BidPrice_H1/A1/2 in RTS
  double GetLastPrice(const std::string &symbol);  // LastPrice_H1/A1/2 in RTS
  double GetMidPrice(const std::string &symbol);  // MidPrice_H1/A1/2 in RTS
  double GetMid(const std::string &symbol);  // Mid_H1/A1/2 in RTS
  double GetSpread(const std::string &symbol);  // BSSpread_H1/A1/2 in RTS
  int GetVolume(const std::string &symbol);  // Volume_Cumulative_H1/A1/2
  double GetBSRatio(const std::string &symbol);  // bsratio_H1/A1/2 in RTS

  // int GetLongPos(const std::string &symbol);
  // int GetShortPos(const std::string &symbol);
  int GetLongVolume(const std::string &symbol);
  int GetShortVolume(const std::string &symbol);

  int GetNetPos(const std::string &symbol);
  double GetScoreAdjust();
};

#endif  // __CRUSH_H__
