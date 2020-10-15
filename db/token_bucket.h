// Author:RUC liang_jk

#pragma once

#include <algorithm>
#include <atomic>
#include <queue>
#include <sys/time.h>
#include "util/mutexlock.h"

namespace rocksdb {

#define DEFAULT_REFILL_PERIOD (100 * 1000)

class ColumnFamilyData;
class RateEstimater;
class DBImpl;

class TokenBucket {
    public:
        explicit TokenBucket(ColumnFamilyData* cfd, long long refill_period_us = DEFAULT_REFILL_PERIOD);
        explicit TokenBucket(ColumnFamilyData* cfd, long long rate_bytes_per_sec, long long refill_period_us, bool auto_tune = false);

        ~TokenBucket();

        void Begin(int code, DBImpl*);

        void Request(); 
        void Request(long long bytes);

        void Record(int type, long long bytes, long long begin_time);
        void Record(int type, long long begin_time);

        long long GetSingleBurstBytes() const {
            MutexLock mu(&request_mutex_);
            return refill_bytes_per_period_;
        };

        long long GetTotalBytesThrough() const {
            MutexLock mu(&request_mutex_);
            return total_bytes_through_;
        };

        long long GetTotalRequests() const{
            MutexLock mu(&request_mutex_);
            return total_requests_;
        };

        long long GetBytesPerSecond () const {
            MutexLock mu(&request_mutex_);
            return rate_bytes_per_sec_;
        };

        long long GetTime() const {
            return NowTime();
        };

    private:
        void Refill();

        long long NowTime() const {
            struct timeval tv;
            gettimeofday(&tv, nullptr);
            return (long long)(tv.tv_sec)*1000000 + tv.tv_usec;
        };

        long long CalculateRefillBytesPerPeriod(long long rate_bytes_per_sec);

        void SetBytesPerSecond(long long bytes_per_second);

        ColumnFamilyData* cfd_;

        int valid_;

        long long rate_bytes_per_sec_;
        const long long refill_period_us_;

        long long refill_bytes_per_period_;
        const long long kMinRefillBytesPerPeriod = 100;

        bool stop_;
        port::CondVar exit_cv_;
        int requests_to_wait_;

        long long available_bytes_;
        long long next_refill_us_;

        mutable port::Mutex request_mutex_;
        long long total_requests_;
        long long total_bytes_through_;

        std::queue<port::CondVar*> queue_;

        int tune_period_;
        long long tune_time_;
        long long tune_bytes_;
        RateEstimater* rate_estimater_;
        bool ShouldTune();

};

// extern TokenBucket token_bucket;

}
