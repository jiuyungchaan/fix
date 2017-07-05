
#ifndef __ORDER_H__
#define __ORDER_H__

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <map>
#include <string>

// enum Direction 
#define kDirectionBuy '0'
#define kDirectionSell '1'
#define kDirectionCollateralBuy '3'
#define kDirectionCollateralSell '4'
#define kDirectionBorrowToBuy '5'
#define kDirectionBorrowToSell '6'
#define kDirectionBuyToPay '7'
#define kDirectionSellToPay '8'

// enum Offset
#define kOffsetOpen '0'
#define kOffsetClose '1'
#define kOffsetCloseToday '2'
#define kOffsetCloseYesterday '3'
#define kOffsetForceClose '4'

// enum Order Type
#define kOrderTypeLimit '0'
#define kOrderTypeMarket '1'
#define kOrderTypeStop '2'
#define kOrderTypeStopLimit '3'
#define kOrderTypeMarketLimit '4'

// enum Time In Force
#define kTimeInForceDay '0'
#define kTimeInForceGTC '1'
#define kTimeInForceFAK '2'
#define kTimeInForceGTD '3'
#define kTimeInForceFOK '4'

// enum OrderStatus
#define kOrderStatusNew '0'
#define kOrderStatusPartiallyFilled '1'
#define kOrderStatusFilled '2'
// #define kOrderStatusDoneForDay '3'
#define kOrderStatusCanceled '4'
#define kOrderStatusModified '5'
#define kOrderStatusRejected '8'
#define kOrderStatusExpired 'C'
#define kOrderStatusUndefined '?'

class Order {
 public:
  char symbol[32];
  char instrument_id[32];
  char account[32];
  char sys_order_id[32];
  char order_flow_id[32];
  char direction;
  char offset;
  char order_type;
  char hedge_flag_type;
  char time_in_force;
  char order_status;
  int order_id;
  int orig_order_id;
  int volume;
  double limit_price;
  double stop_price;
  long ttl;

  Order() : symbol{0}, instrument_id{0}, account{0}, sys_order_id{0},
            order_flow_id{0}, direction(kDirectionBuy), offset(kOffsetOpen),
            order_type(kOrderTypeLimit), hedge_flag_type('0'),
            time_in_force(kTimeInForceDay), 
            order_status(kOrderStatusUndefined), order_id(-1),
            orig_order_id(-1), volume(0), limit_price(0.00),
            ttl(0) {}

  ~Order() {}
};

class OrderAck {
 public:
  char account[32];
  char sys_order_id[32];
  char order_status;
  int order_id;

  OrderAck() : account{0}, sys_order_id{0}, 
               order_status(kOrderStatusUndefined), order_id(-1) {}
};

class Deal {
 public:
  char account[32];
  char sys_order_id[32];
  char deal_id[32];
  char transact_time[32];
  int volume;
  double price;

  Deal() : account{0}, sys_order_id{0}, deal_id{0}, transact_time{0},
           volume(0), price(0.00) {}
};

class OrderPool {
 public:
  static const int MAX_ORDER_SIZE = 10240;
  
  OrderPool() : order_size_(0) {
    memset(order_pool_, sizeof(order_pool_), 0);
  }

  void reset_sequence(int start_id) {
    order_size_ = start_id + 1;
  }

  Order *get(int order_id) {
    if (order_id >= 0 && order_id < MAX_ORDER_SIZE) {
      return order_pool_[order_id];
    } else {
      return (Order *)NULL;
    }
  }

  Order *get(std::string sys_id) {
    std::map<std::string, int>::iterator it = sys_to_local_.find(sys_id);
    if (it == sys_to_local_.end()) {
      printf("SysOrderID-%s not found !\n", sys_id.c_str());
      return (Order *)NULL;
    } else {
      return order_pool_[it->second];
    }
  }

  // add Order to OrderPool and return order ID in OrderPool
  int add(Order *order) {
    int order_id = __sync_fetch_and_add(&order_size_, 1);
    order_pool_[order_id] = order;
    return order_id;
  }

  void add_pair(std::string sys_id, int local_id) {
    std::map<std::string, int>::iterator it = sys_to_local_.find(sys_id);
    if (it != sys_to_local_.end()) {
      printf("SysOrderID-%s already match for %d !\n", sys_id.c_str(), 
             it->second);
    } else {
      sys_to_local_.insert(std::pair<std::string, int>(sys_id, local_id));
    }
  }

 private:
  Order *order_pool_[MAX_ORDER_SIZE];
  int order_size_;
  std::map<std::string, int> sys_to_local_;
};

#endif  /// __ORDER_H__
