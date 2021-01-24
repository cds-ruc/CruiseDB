// Author:RUC liang_jk

#include <assert.h>
#include <iostream>
#include "db/column_family.h"
#include "db/internal_stats.h"
#include "db/rate_estimater.h"
#include "db/version_set.h"
#include "db/db_impl/db_impl.h"
#include "monitoring/statistics.h"

#define MAX_RECORDS 100000
#define MIN_INTERVAL 1000000
#define MIN_START_RATE
#define DISK_RATE
#define DEFAULT_LIMITED_SPEED
#define MAX_LIMITED_SPEED
#define MIN_TUNE_RATIO 0.5
#define MAX_TUNE_RATIO 1.5

namespace rocksdb {

Statistics* re_statistics = nullptr;

RateEstimater::RateEstimater(ColumnFamilyData* cfd)
  : cfd_(cfd), db_handle_(nullptr), now_rate_(-1), read_bytes_(0), write_bytes_(0), read_time_(0), write_time_(0) {
	  std::cout<<"RateEstimater\t"<<cfd_->GetName()<<std::endl;
	  re_statistics = new StatisticsImpl(nullptr);
	//   std::cout<<"Getting Parameters k1 k2:"<<std::endl;
	//   std::cin >> k1 >> k2;
	  std::cout<<"Parameters:\n\tk1:\t"<<k1<<"\tk2:\t"<<k2<<std::endl;
  }

RateEstimater::RateEstimater(ColumnFamilyData* cfd, long long initial_rate)
  : cfd_(cfd), db_handle_(nullptr), now_rate_(initial_rate), read_bytes_(0), write_bytes_(0), read_time_(0), write_time_(0) {
	  std::cout<<"RateEstimater\t"<<cfd_->GetName()<<std::endl;
	  re_statistics = new StatisticsImpl(nullptr);
	//   std::cout<<"Getting Parameters k1 k2:"<<std::endl;
	//   std::cin >> k1 >> k2;
	  std::cout<<"Parameters:\n\tk1:\t"<<k1<<"\tk2:\t"<<k2<<std::endl;
  }

RateEstimater::~RateEstimater(){
	delete re_statistics;
	re_statistics = nullptr;
	std::cout<<"RateEstimater\t"<<cfd_->GetName()<<"\tRead_Bytes_Ratio:\t"<<double(read_bytes_)/(read_bytes_+write_bytes_)<<"\tRead_time_Ratio:\t"<<double(read_time_)/(read_time_+write_time_)<<std::endl;
	std::cout<<"Records:"<<std::endl;
	for(unsigned i=0;i<records_.size();++i) {
		time_t time_time_t = records_[i].time/1000000;
		char buf[128];
		strftime(buf, 127, "%T", localtime(&time_time_t));
		std::string time_str(buf);
		time_str += "." + std::to_string(records_[i].time%1000000);
		std::cout<<"Time:\t"<<time_str<<"\tScore:\t[ ";
		for(unsigned j=0;j<6;++j)std::cout<<records_[i].score[j]<<" ";
		std::cout<<"]\tMax Score:\t"<<records_[i].max_score()<<"\tRate:\t"<<records_[i].rate/1024.0/1024.0<<"MB/s\tLast:\t"<<records_[i].last/1024.0/1024.0<<"MB/s\tRatio:\t"<<records_[i].ratio<<std::endl;
	}
}

long long RateEstimater::Estimate(double last_rate) {
	assert(db_handle_);
	if (records_.empty()) {
		Record rec;
		rec.time = NowTime();		
		VersionStorageInfo* vsinfo = cfd_->current()->storage_info();
		assert(vsinfo->MaxInputLevel() < 8);
		for (int idx = 0; idx <= vsinfo->MaxInputLevel(); ++idx) {
			if (idx) {
				rec.score[idx] = vsinfo->NumLevelBytes(idx)/static_cast<double>(vsinfo->MaxBytesForLevel(idx));
			} else {
				rec.score[idx] = vsinfo->NumLevelFiles(idx)/4.0;
			}
		}
	if (now_rate_ < 0) now_rate_ = ColdBegin();
	rec.rate = now_rate_;
	rec.last = LimitedSpeed();
	rec.ratio = 0;
	records_.push_back(rec);
	cfd_->internal_stats()->Clear();
	return now_rate_;
	}
	assert(now_rate_ > 0);
	Record last_rec = records_.back();
	Record new_rec;
	new_rec.time = NowTime();
	if (last_rate < 0 || new_rec.time - last_rec.time < MIN_INTERVAL) {
		return now_rate_;
	}
	VersionStorageInfo* vsinfo = cfd_->current()->storage_info();
	assert(vsinfo->MaxInputLevel() < 8);
	for (int idx = 0; idx <= vsinfo->MaxInputLevel(); ++idx) {
		if (idx) {
			new_rec.score[idx] = vsinfo->NumLevelBytes(idx)/static_cast<double>(vsinfo->MaxBytesForLevel(idx));
		} else {
			new_rec.score[idx] = vsinfo->NumLevelFiles(idx)/4.0;
		}
	}
	new_rec.last = last_rate;
	//Strategy
	now_rate_ = last_rate;

/* 	if (new_rec.max_score()>last_rec.max_score()+1){
		now_rate_ = LimitedSpeed();
		new_rec.reason = 1;
	} else if (new_rec.score[0]<2) {
		if (new_rec.max_score()<last_rec.max_score()-0.5) {
				now_rate_ = now_rate_ * speed::RAPID_GROW;
				new_rec.reason = 2;
		} else if (new_rec.max_score()<last_rec.max_score()) {
				now_rate_ = now_rate_ * speed::QUICK_GROW;
				new_rec.reason = 3;
		} else if (new_rec.max_score()<last_rec.max_score()+0.25) {
				now_rate_ = now_rate_ * speed::GROW;
				new_rec.reason = 4;
		} else if (new_rec.max_score()<last_rec.max_score()+0.5) {
				now_rate_ = now_rate_ * speed::KEEP;
				new_rec.reason = 5;
		} else {
			now_rate_ = now_rate_ * speed::DECLINE;
			new_rec.reason = 6;
		}
	} else if (new_rec.score[0]<3) {
		if (new_rec.max_score()>last_rec.max_score()+0.5) {
			now_rate_ = now_rate_ * speed::QUICK_DECLINE;
			new_rec.reason = 7;
		} else if (new_rec.max_score()>last_rec.max_score()+0.25) {
			now_rate_ = now_rate_ * speed::DECLINE;
			new_rec.reason = 8;
		} else if (new_rec.max_score()>last_rec.max_score()) {
			now_rate_ = now_rate_ * speed::SLOW_DECLINE;
			new_rec.reason = 9;
		} else {
			now_rate_ = now_rate_ * speed::KEEP;
			new_rec.reason = 10;
		}
	} else {
		now_rate_ = LimitedSpeed();
		new_rec.reason = 11;
	} */

	double ratio=1.0;

	if (new_rec.score[0]>3) {
		now_rate_ = LimitedSpeed();
		new_rec.ratio = -1;
	} else if (new_rec.max_score() - last_rec.max_score() > 2) {
		now_rate_ = LimitedSpeed();
		new_rec.ratio = -2;
	} else {
		ratio = 1 - k1 * (new_rec.score[0]-1.5) / 3.0 - k2 * (new_rec.max_score() - last_rec.max_score()) / 5.0;
		new_rec.ratio = ratio;
		if (ratio < MIN_TUNE_RATIO) {
			now_rate_ = LimitedSpeed();
		} else if (ratio > MAX_TUNE_RATIO) {
			now_rate_ = ColdBegin();
		} else {
			now_rate_ = now_rate_ * ratio;
		}
	}
	
	//
	new_rec.rate=now_rate_;
	while(records_.size() >= MAX_RECORDS) {
		records_.pop_front();
	}
	records_.push_back(new_rec);
	return now_rate_;
}

void RateEstimater::Record_Request(int type, long long bytes, long long time) {
	assert(type == 0 || type == 1);
	assert(bytes > 0);
	if(time > 10) {
		--time;
	}
	assert(time > 0);
	if (type) {
		write_bytes_ += bytes;
		write_time_ += time;
	} else {
		read_bytes_ += bytes;
		read_time_ += time;
	}
	return;
}

long long RateEstimater::ColdBegin() {
	std::cout<<"ColdBegin Called!"<<std::endl;
	//std::cout<<"Read KB:\t"<<(read_bytes_>>10)<<"\tWrite KB:\t"<<(write_bytes_>>10)<<std::endl;
	//std::cout<<"Read Time:\t"<<read_time_<<"\tWrite Time:\t"<<write_time_<<std::endl;
	long long ret = 0;
	if (read_time_+write_time_ == 0) {
		ret = LimitedSpeed();
	} else {
		ret = (read_bytes_+write_bytes_) / ((read_time_+write_time_)/1000000.0);
	}
	std::cout<<"ColdBegin Result:\t"<<(ret>>20)<<"MB/s"<<std::endl;
	if(ret<MIN_START_RATE)ret = MIN_START_RATE;
	return ret;
}

long long RateEstimater::LimitedSpeed() {
	std::cout<<"LimitedSpeed Called!"<<std::endl;
	long long disk_rate = DISK_RATE;
	assert(re_statistics);
	long long cache_write = re_statistics->getTickerCount(BLOCK_CACHE_BYTES_WRITE);
	long long bytes_read = re_statistics->getTickerCount(BYTES_READ);

	if(bytes_read + write_bytes_ == 0) return DEFAULT_LIMITED_SPEED;

	long long comp_read = 0, comp_write = 0;
	InternalStats* internal_stats = cfd_->internal_stats();
	int maxlv = cfd_->current()->storage_info()->MaxOutputLevel(false);
	for(int lv = 1; lv < maxlv; ++lv) {
		const InternalStats::CompactionStats &level_stats = internal_stats -> TEST_GetCompactionStats().at(lv);
		if (level_stats.count == 0) break;
		comp_read += level_stats.bytes_read_output_level;
		if(lv > 1) {
			comp_read += level_stats.bytes_read_non_output_levels;
		}
		comp_write += level_stats.bytes_written;
	}

	double read_ratio = double(read_bytes_)/(read_bytes_+write_bytes_);
	double write_ratio = double(write_bytes_)/(read_bytes_+write_bytes_);

	double coff = 0;
	if (bytes_read) {
		coff += read_ratio*cache_write/bytes_read;
	}
	if(write_bytes_) {
		coff += write_ratio*comp_read/write_bytes_ + write_ratio*comp_write/write_bytes_;
	}
	long long ret = disk_rate/coff;
	std::cout<<"LimitedSpeed Result:\t"<<(ret>>20)<<"MB/s"<<std::endl;
	if (ret > MAX_LIMITED_SPEED) ret = MAX_LIMITED_SPEED;
	return ret;
}

}