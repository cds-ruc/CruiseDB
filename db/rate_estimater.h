// Author:RUC liang_jk

#pragma once

#include <deque>
#include <atomic>

namespace rocksdb {

class ColumnFamilyData;
class DBImpl;

class RateEstimater {
	public:
		RateEstimater(ColumnFamilyData* cfd);
		RateEstimater(ColumnFamilyData* cfd, long long initial_rate);

		~RateEstimater();

		long long Estimate(double last_rate);

		void Record_Request(int type, long long bytes, long long time);

		void SetDBHandle(DBImpl* db_handle) {
			db_handle_ = db_handle;
			return;
		}

	private:
		long long ColdBegin();
		long long LimitedSpeed();

		long long NowTime(){
            struct timeval tv;
            gettimeofday(&tv, nullptr);
            return (long long)(tv.tv_sec)*1000000 + tv.tv_usec;
        };

		struct Record {
			long long time;
			double score[8];
			long long rate;
			long long last;
			double ratio;

			Record():time(0),rate(0),last(0),ratio(-1) {
				for(int i=0;i<8;++i)score[i]=0;
			}

			double max_score() {
				double ret=0;
				for(int i=0;i<8;++i)
					if(score[i]>ret)
						ret=score[i];
				return ret;
			}
		};

		double k1=0.5,k2=0.4;

		ColumnFamilyData* cfd_;
		DBImpl* db_handle_;

		std::deque<Record> records_;

		long long now_rate_;

		std::atomic<long long>read_bytes_, write_bytes_, read_time_, write_time_;
};

}
