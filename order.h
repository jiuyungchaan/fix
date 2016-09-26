
#ifndef __ORDER_H__
#define __ORDER_H__

#include <string.h>
#include <stdlib.h>

enum Direction {
  kDirectionBuy = '0',
  kDirectionSell = '1'
};

class Order {
 public:
  char instrument_id[32];
  char account[32];
  char sys_order_id[32];
  char direction;
  char offset;
  char order_type;
  char hedge_flag_type;
  int order_id;
  int orig_order_id;
  int order_status;
  int volume;
  double limit_price;
  long ttl;

};

class OrderPool {
 public:
  static const int MAX_ORDER_SIZE = 10240;
  
  OrderPool() : order_size_(0) {
    memset(order_pool_, sizeof(order_pool_), 0);
  }

  Order *get(int order_id) {
    if (order_id >= 0 && order_id < MAX_ORDER_SIZE) {
      return order_pool_[order_id];
    } else {
      return (Order *)NULL;
    }
  }

  // add Order to OrderPool and return order ID in OrderPool
  int add(Order *order) {
    int order_id = __sync_fetch_and_add(&order_size_, 1);
    order_pool_[order_id] = order;
    return order_id;
  }

 private:
  Order *order_pool_[MAX_ORDER_SIZE];
  int order_size_;
};

#endif  /// __ORDER_H__
