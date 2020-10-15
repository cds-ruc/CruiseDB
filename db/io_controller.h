// Author:RUC liang_jk

#pragma once

#include <atomic>
#include <unistd.h>
#include <rocksdb/env.h>

namespace rocksdb {

class IoController {
    public:
        explicit IoController();

        ~IoController();

        ssize_t AddJob(int fd, const char* src, size_t bytes_to_write, Env::IOPriority pri);

        Status status();

    private:
        std::atomic<int> running_high_,waiting_,call_times_,waiting_times_,sleeping_times_;
        Status status_;
};

extern IoController io_controller;

}
