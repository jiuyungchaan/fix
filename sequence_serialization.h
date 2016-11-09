#ifndef __SEQUENCE_SERIALIZATION_H__
#define __SEQUENCE_SERIALIZATION_H__

#include <fstream>

#include <pthread.h>

class SequenceSerialization {
 public:
  SequenceSerialization();
  virtual ~SequenceSerialization();

  int Init();
  void SetPrefix(const char *prefix);

  // Order-ID is identifier of requests including NewOrderSingle,
  // OrderCancelRequest, OrderCancelReplaceRequest and so on
  void DumpOrderID(int order_id);
  int GetCurOrderID();

  // Order-Flow-ID is identifier throughout the life of order starting
  // with the new order and ending once the order has been filled or 
  // canceled/expired
  void DumpFlowID();
  int GetCurFlowID();
  const char *GenFlowIDStr(int flow_id);

  // Transaction-ID is identifier of messages including request messages
  // sent and response messages received
  void DumpTransID();
  int GetCurTransID();
  const char *GenTransIDStr();

 private:
  static const int MAX_ID = 100000000;
  std::fstream dump_file_;
  bool file_good_;
  int date_;
  char prefix_[32];

  int cur_order_id_;
  int cur_flow_id_;
  int cur_trans_id_;

  pthread_mutex_t trans_lock_;
};

#endif  // __SEQUENCE_SERIALIZATION_H__
