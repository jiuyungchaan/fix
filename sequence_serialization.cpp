#include "sequence_serialization.h"

#include <stdio.h>

#include "utils.h"

using namespace std;

SequenceSerialization::SequenceSerialization() : prefix_{0} {
  // TODO
  int ret = pthread_mutex_init(&trans_lock_, NULL);
  if (ret != 0) {
    printf("Failed to initialize transaction mutex lock\n");
  } else {
    printf("Initialize transaction mutex lock sucessfully\n");
  }
}

SequenceSerialization::~SequenceSerialization() {
  // TODO
}

void SequenceSerialization::SetPrefix(const char *prefix) {
  snprintf(prefix_, sizeof(prefix_), "%s", prefix);
}

int SequenceSerialization::Init() {
  date_ = date_now();
  char dump_file_name[64];
  snprintf(dump_file_name, sizeof(dump_file_name), "%s_%d.seq", prefix_, date_);
  dump_file_.open(dump_file_name, fstream::in | fstream::out | fstream::app);
  if (!dump_file_.good()) {
    printf("Can't open file %s\n", dump_file_name);
    file_good_ = false;
    return 1;
  }
  printf("open file sucessfully\n");
  file_good_ = true;
  // content of file is like 'OrderID:FlowID:TransID'
  // example:               '00000012:00001002:00000333'
  char c = dump_file_.get();
  if (dump_file_.eof()) {
    dump_file_.close();
    dump_file_.open(dump_file_name, fstream::in | fstream::out);
    cur_order_id_ = cur_flow_id_ = cur_trans_id_ = 0;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%08d:%08d:%08d", cur_order_id_,
        cur_flow_id_, cur_trans_id_);
    // dump_file_.clear();  // clear() to transfer read mode to write mode
    dump_file_.seekp(0);
    dump_file_.write(buffer, 26);
    dump_file_.flush();
  } else {
    // dump_file_.clear();
    dump_file_.seekg(0);
    char order_id_buf[16], flow_id_buf[16], trans_id_buf[16];
    dump_file_.read(order_id_buf, 9);  // read to ':'
    order_id_buf[8] = '\0';  // rewrite ':' to '\0'
    dump_file_.read(flow_id_buf, 9);
    flow_id_buf[8] = '\0';
    dump_file_.read(trans_id_buf, 8);
    trans_id_buf[8] = '\0';

    cur_order_id_ = atoi(order_id_buf);
    cur_flow_id_ = atoi(flow_id_buf);
    cur_trans_id_ = atoi(trans_id_buf);
    printf("order:%d flow:%d trans:%d\n", cur_order_id_, cur_flow_id_, cur_trans_id_);
    // cur_order_id_++;
    // cur_flow_id_++;
    // cur_trans_id_++;

    dump_file_.close();
    dump_file_.open(dump_file_name, fstream::in | fstream::out);
  }
}

void SequenceSerialization::DumpOrderID(int order_id) {
  if (!file_good_) {
    printf("Invalid file stream\n");
    return;
  }
  if (order_id != cur_order_id_ + 1) {
    printf("WARNING: OrderID Gap Detected %d-->%d\n", cur_order_id_, order_id);
  }
  cur_order_id_ = order_id;
  char order_id_buf[16];
  snprintf(order_id_buf, sizeof(order_id_buf), "%08d", cur_order_id_);
  dump_file_.seekp(0);
  dump_file_.write(order_id_buf, 8);
  dump_file_.flush();
}

int SequenceSerialization::GetCurOrderID() {
  return cur_order_id_;
}

void SequenceSerialization::DumpFlowID() {
  if (!file_good_) {
    printf("Invalid file stream\n");
    return;
  }
  char flow_id_buf[16];
  snprintf(flow_id_buf, sizeof(flow_id_buf), "%08d", cur_flow_id_);
  dump_file_.seekp(9);
  dump_file_.write(flow_id_buf, 8);
  dump_file_.flush();
}

int SequenceSerialization::GetCurFlowID() {
  return cur_flow_id_;
}

const char *SequenceSerialization::GenFlowIDStr(int flow_id) {
  static char flow_id_buf[32];
  cur_flow_id_ = flow_id;
  snprintf(flow_id_buf, sizeof(flow_id_buf), "%s%08d", prefix_, flow_id);
  DumpFlowID();
  return flow_id_buf;
}

int SequenceSerialization::GetCurTransID() {
  return cur_trans_id_;
}

void SequenceSerialization::DumpTransID() {
  if (!file_good_) {
    printf("Invalid file stream\n");
    return;
  }
  char trans_id_buf[16];
  snprintf(trans_id_buf, sizeof(trans_id_buf), "%08d", cur_trans_id_);
  dump_file_.seekp(18);
  dump_file_.write(trans_id_buf, 8);
  dump_file_.flush();
}

const char *SequenceSerialization::GenTransIDStr() {
  static char trans_id_buf[32];
  pthread_mutex_lock(&trans_lock_);
  cur_trans_id_++;
  snprintf(trans_id_buf, sizeof(trans_id_buf), "%d%08d", date_, cur_trans_id_);
  DumpTransID();
  pthread_mutex_unlock(&trans_lock_);
  return trans_id_buf;
}

