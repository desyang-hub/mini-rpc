#include "minirpc/common/logger.h"
#include "minirpc/common/timeStamp.h"

#include <thread>
#include <iostream>
#include <unistd.h>

namespace minirpc
{

Logger& Logger::GetInstanse() {
    static Logger logger;
    return logger;
}

void Logger::enable_async_log_write() {
    is_async_ = true;
    log_file_ = std::fopen("app.log", "a");
    if (log_file_ == nullptr) {
        perror("fopen error");
        exit(-1);
    }
    blocked_que_ = std::make_unique<BlockedQueue<std::string>>();
    async_log_write_thread_ = std::thread(&Logger::async_log_write, this);
}

Logger::Logger() {
}

Logger::~Logger() {
    if (is_async_) {
        blocked_que_->close();
        if (async_log_write_thread_.joinable())
            async_log_write_thread_.join();

        fflush(log_file_);
        fclose(log_file_);
    }
}

void Logger::async_log_write() {
    std::string msg;
    while (true) {
        // 做任务，就是将日志一条条写入到日志文件中
        if (blocked_que_->pop(msg)) { {
            fputs(msg.c_str(), log_file_);
            fflush(log_file_);
        }
        }
        else
            break;
    }
}

Logger& Logger::setLevel(int level) {
    log_level_ = level;
    return *this;
}

int Logger::getLevel() const {
    return log_level_;
}

void Logger::log(const std::string& msg, int log_level) {

    std::string info = TimeStamp::Now().toString() + " ";

    // 打印日志级别
    switch (log_level)
    {
    case INFO:
        info += "[INFO] ";
        break;

    case ERROR:
        info += "[ERROR] ";
        break;

    case FATAL:
        info += "[FATAL] ";
        break;

    case DEBUG:
        info += "[DEBUG] ";
        break;
    
    default:
        break;
    }

    // 时间和message
    
    info += msg;
    info.push_back('\n');

    // 写入日志
    if (is_async_) {
        blocked_que_->enqueue(std::move(info));
    }
    else {
        std::cout << info;
    }
}

    
} // namespace minirpc
