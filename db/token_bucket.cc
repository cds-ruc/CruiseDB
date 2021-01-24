// Author:RUC liang_jk

#include <assert.h>
#include <iostream>
#include "db/column_family.h"
#include "db/token_bucket.h"
#include "db/rate_estimater.h"

namespace rocksdb {

#define TUNE_PERIOD 300000

TokenBucket::TokenBucket(ColumnFamilyData* cfd,
                         long long rate_bytes_per_sec,
                         long long refill_period_us,
                         bool auto_tune)
    : cfd_(cfd),
      valid_(0),
      rate_bytes_per_sec_(rate_bytes_per_sec),
      refill_period_us_(refill_period_us),
      refill_bytes_per_period_(CalculateRefillBytesPerPeriod(rate_bytes_per_sec_)),
      stop_(false),
      exit_cv_(&request_mutex_),
      requests_to_wait_(0),
      available_bytes_(0),
      next_refill_us_(NowTime()),
      total_requests_(0),
      total_bytes_through_(0),
      tune_period_(TUNE_PERIOD),
      tune_time_(0),
      tune_bytes_(0),
      rate_estimater_(nullptr) {
        std::cout<<"TokenBucket\t"<<cfd_->GetName()<<std::endl;
        if (auto_tune) {
          rate_estimater_ = new RateEstimater(cfd_,rate_bytes_per_sec);
        }
    }

TokenBucket::TokenBucket(ColumnFamilyData* cfd,
                         long long refill_period_us)
    : cfd_(cfd),
      valid_(0),
      refill_period_us_(refill_period_us),
      stop_(false),
      exit_cv_(&request_mutex_),
      requests_to_wait_(0),
      available_bytes_(0),
      next_refill_us_(NowTime()),
      total_requests_(0),
      total_bytes_through_(0),
      tune_period_(TUNE_PERIOD),
      tune_time_(0),
      tune_bytes_(0) {
        std::cout<<"TokenBucket\t"<<cfd_->GetName()<<std::endl;
        rate_estimater_ = new RateEstimater(cfd_);
      }

TokenBucket::~TokenBucket() {
  MutexLock mu(&request_mutex_);
  stop_ = true;
  requests_to_wait_ = queue_.size();

  while(!queue_.empty()) {
    queue_.front()->Signal();
    queue_.pop();
  }

  while (requests_to_wait_ > 0) {
    exit_cv_.Wait();
  }

  std::cout<<"Token Bucket\t"<<cfd_->GetName()<<"\tTotal Requests: "<<total_requests_<<"\tTotal Bytes Through: "<<total_bytes_through_<<std::endl;

  if (rate_estimater_) delete rate_estimater_;
}

void TokenBucket::Begin(int code, DBImpl* db_handle) {
  MutexLock mu(&request_mutex_);
  assert(valid_ == code - 1);
  std::cout<<"TokenBucket\t"<<cfd_->GetName()<<"\tBegin\t"<<code<<std::endl;
  valid_ = code;
  if (valid_ >= 3) {
    assert(false);
  } else if (valid_ >= 2 && rate_estimater_) {
    rate_estimater_->SetDBHandle(db_handle);
    SetBytesPerSecond(rate_estimater_->Estimate(-1));
    tune_time_ = NowTime();
  }
  return;
}

void TokenBucket::SetBytesPerSecond(long long bytes_per_second) {
  assert(bytes_per_second > 0);
  rate_bytes_per_sec_ = bytes_per_second;
  refill_bytes_per_period_ = CalculateRefillBytesPerPeriod(bytes_per_second);
}

bool TokenBucket::ShouldTune() {
  return (rate_estimater_ && total_requests_ > 0 && total_requests_ % tune_period_ == 0);
}

void TokenBucket::Request(long long bytes) {
  if (valid_ < 2) {
    return;
  }

  MutexLock mu(&request_mutex_);

  assert(bytes <= refill_bytes_per_period_);

  if (stop_) {
    return;
  }

  ++total_requests_;

  if (ShouldTune()) {
    assert(rate_estimater_);
    SetBytesPerSecond(rate_estimater_->Estimate((total_bytes_through_-tune_bytes_)/((NowTime()-tune_time_)/1000000.0)));
    tune_time_ = NowTime();
    tune_bytes_ = total_bytes_through_;
  }

  port::CondVar cv(&request_mutex_);
  queue_.push(&cv);

  while (queue_.front() != &cv) {
    cv.Wait();
    if (stop_) {
      --requests_to_wait_;
      exit_cv_.Signal();
      return;
    }
  }

  assert(queue_.front() == &cv);
  assert(!stop_);

  while(available_bytes_ < bytes) {
    if (NowTime() >= next_refill_us_) {
      Refill();
    } else {
      // long long begin=NowTime();
      // long long expected=next_refill_us_-begin;
      cv.TimedWait(next_refill_us_);
      // long long end=NowTime();
      // std::cout<<"Timed Wait Expected Time: "<<expected<<" us\t Used Time: "<<end-begin<<" us"<<std::endl;
    }
  }

  available_bytes_ -= bytes;
  total_bytes_through_ += bytes;

  queue_.pop();
  if (!queue_.empty()) {
    queue_.front()->Signal();
  }
  return;
}

void TokenBucket::Record(int type, long long bytes, long long begin_time) {
  assert(type == 0 || type ==1);
  assert(bytes > 0);
  assert(begin_time > 0);
  if (valid_ && rate_estimater_) {
    rate_estimater_->Record_Request(type, bytes, NowTime()-begin_time);
  }
  return;
}

long long TokenBucket::CalculateRefillBytesPerPeriod(long long rate_bytes_per_sec) {
  if (port::kMaxInt64 / rate_bytes_per_sec < refill_period_us_) {
    return port::kMaxInt64 / 1000000;
  } else {
    return std::max(kMinRefillBytesPerPeriod,
                    rate_bytes_per_sec * refill_period_us_ / 1000000);
  }
}

void TokenBucket::Refill() {
  next_refill_us_ = NowTime() + refill_period_us_;
  if (available_bytes_ < refill_bytes_per_period_) {
    available_bytes_ += refill_bytes_per_period_;
  }
  return;
}

#define SINGLE_SIZE 1 //KB

void TokenBucket::Request() {
  return Request((SINGLE_SIZE)<<10);
}

void TokenBucket::Record(int type, long long begin_time) {
  return Record(type, (SINGLE_SIZE)<<10, begin_time);
}

// #define SINGLE_TIME 100 //ms
// #define START_RATE 40 //MB/S
// TokenBucket token_bucket((long long)(START_RATE) << 20,(SINGLE_TIME)*1000);

}
