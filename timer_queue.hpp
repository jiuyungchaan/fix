#ifndef __TIMER_QUEUE_HPP__
#define __TIMER_QUEUE_HPP__

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

long cur_millisec() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  long milsec = tv.tv_sec*1000 + tv.tv_usec/1000;
  return milsec;
}

template <typename T>
class TimerQueue {
 private:
  struct qitem {
    T *value;
    long interval;  // millisecond
    long timestamp;
    qitem() : value(NULL), interval(0),  timestamp(cur_millisec()){}
    qitem(T *v, long i) : value(v), interval(i), timestamp(cur_millisec()) {}

    bool operator < (const qitem &item) const {
      return this->timestamp + this->interval < item.timestamp + item.interval;
    }
    bool expired() const {
      return this->timestamp + this->interval <= cur_millisec();
    }
  };

  qitem* queue_[10000];
  int size_;
  pthread_mutex_t mutex_;

 public:
  TimerQueue(){
    size_ = 0;
    memset(queue_, 0, sizeof(queue_));
    pthread_mutex_init(&mutex_, NULL);
  }

  virtual ~TimerQueue(){}

  void push(T *v, long interval) {
    pthread_mutex_lock(&mutex_);
    qitem *qi = new qitem(v, interval);
    queue_[size_++] = qi;
    heapify();
    pthread_mutex_unlock(&mutex_);
  }

  T *pop() {
    T *ret = NULL;
    pthread_mutex_lock(&mutex_);
    if (size_ > 0 && queue_[0]->expired()) {
      ret = queue_[0]->value;
      delete queue_[0];
      queue_[0] = queue_[--size_];
      heapify();
    }
    pthread_mutex_unlock(&mutex_);
    return ret;
  }

  void heapify() {
    for (int i = (size_ - 2)/2; i >= 0; i--) {
      int left = i*2 + 1;
      int right = i*2 + 2;
      if (left >= size_) {
        continue;
      }
      int min_pos = left;
      if (right < size_) {
        min_pos = (*queue_[left] < *queue_[right]) ? left : right;
      }
      if (*queue_[min_pos] < *queue_[i]) {
        qitem *temp = queue_[i];
        queue_[i] = queue_[min_pos];
        queue_[min_pos] = temp;
      }
    }
  }

};

#endif  // __PRIOR_QUEUE_HPP__