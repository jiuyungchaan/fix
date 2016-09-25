
#ifndef __ORDER_H__
#define __ORDER_H__

class Order {
 public:
  char instrument_id[32];
  char direction;
  char offset;
  char order_type;
  char hedge_flag_type;
  int order_id;
  int order_status;
  int volume;
  double limit_price;
  long ttl;

};

#endif  /// __ORDER_H__
