// Author:RUC liang_jk

#define USLEEP_TIME 100 //us
#define MAX_WAITING_TIME 1000 //ms

#include <iostream>
#include "db/io_controller.h"
#include "rocksdb/status.h"

namespace rocksdb {

IoController::IoController()
    : running_high_(0),
      waiting_(0),
      call_times_(0),
      waiting_times_(0),
      sleeping_times_(0),
      status_(Status::OK()) {
          std::cout<<"IoController"<<std::endl;
      }

IoController::~IoController() {
    std::cout<<"Call times: "<<call_times_<<std::endl;
    std::cout<<"Waiting times: "<<waiting_times_<<std::endl;
    std::cout<<"Sleeping time: "<<sleeping_times_*(USLEEP_TIME/1000.0)<<"ms"<<std::endl;
}

ssize_t IoController::AddJob(int fd, const char* src, size_t bytes_to_write, Env::IOPriority pri
) {
    ++call_times_;
    if (!status_.ok()) {
        return 0;
    }
    ssize_t done;
    if (pri>=Env::IO_HIGH) {
        ++running_high_;
        done = write(fd , src, bytes_to_write);
        --running_high_;
    } else {
        if (running_high_>0) {
            ++waiting_;
            ++waiting_times_;
            int sleeping_times=0;
            while(running_high_>0&&sleeping_times*(USLEEP_TIME/1000.0)<MAX_WAITING_TIME) {
                usleep(USLEEP_TIME);
                ++sleeping_times;
            }
            --waiting_;
            sleeping_times_+=sleeping_times;
        }
        done = write(fd , src, bytes_to_write);
    }
    return done;
}

Status IoController::status() {
    if (!status_.ok()) {
      return status_;
    }
    return Status::OK();
}

IoController io_controller;

}
