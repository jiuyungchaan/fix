#ifndef __TEST_ORDER_H__
#define __TEST_ORDER_H__

#include <string>
#include <vector>
#include <map>

#include <strategy.h>

class TestOrder : public strategy {
 public:
  account *trader_account;

  std::string instrument_id;
  int quantity;
  double price;
  double atick_value;
  char direction;
  char flag_type;
  char hedge_flag_type;
  bool allow_trading;

  int *norder_list;
  OrderAgent **order_agents;

  TestOrder(const std::string& name, std::vector<strategy*>& st, 
        CFtdTrader *trader_list, int ntrader);
  bool load_config();
  void update(DepthMarketDataField *market_data);

  void calculateMarketStat();
  bool validTradingHour();
  bool marketEnd();
  void getInstrumentNameMap(std::map<std::string, std::string>& m_instrument);
  void saveState();
};

#endif  // __TEST_ORDER_H__
