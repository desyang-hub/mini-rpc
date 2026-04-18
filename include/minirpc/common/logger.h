/**
 * @FilePath     : /gtest_demo/include/common/logger.h
 * @Description  :  
 * @Author       : desyang
 * @Date         : 2026-03-25 20:05:54
 * @LastEditors  : desyang
 * @LastEditTime : 2026-03-25 20:46:26
**/
#pragma once

#include "minirpc/common/nonecopyable.h"
#include "minirpc/common/blockedQueue.h"

#include <string>
#include <memory>
#include <atomic>
#include <fstream>
#include <thread>
#include <unistd.h>
#include <string.h>

// 开启异步日志写入
#define ENABLE_ASYNC_LOGING() \
minirpc::Logger::GetInstanse().enable_async_log_write()

#define LOG_INFO(format, ...) \
    if (minirpc::Logger::GetInstanse().getLevel() >= minirpc::INFO) {                                    \
        char buf[1024];                     \
        snprintf(buf, 1024, format, ##__VA_ARGS__);\
        minirpc::Logger::GetInstanse().log(buf, minirpc::INFO);     \
    };                             \


#define LOG_ERROR(format, ...) \
if (minirpc::Logger::GetInstanse().getLevel() >= minirpc::ERROR) {                                    \
        char buf[1024];                     \
        snprintf(buf, 1024, format, ##__VA_ARGS__);\
        minirpc::Logger::GetInstanse().log(buf, minirpc::ERROR);     \
    }; 

#define LOG_FATAL(format, ...) \
if (true) {                                    \
        char buf[1024];                     \
        snprintf(buf, 1024, "%s:%d %s ", __FILE__, __LINE__, __FUNCTION__); \
        snprintf(buf + strlen(buf), 1024, format, ##__VA_ARGS__);\
        minirpc::Logger::GetInstanse().log(buf, minirpc::FATAL);     \
    }; exit(-1);


// 为了避免debug输出太多信息
#define LOG_DEBUG(format, ...) \
if (minirpc::Logger::GetInstanse().getLevel() >= minirpc::DEBUG) {                                    \
        char buf[1024];                     \
        snprintf(buf, 1024, format, ##__VA_ARGS__);\
        minirpc::Logger::GetInstanse().log(buf, minirpc::DEBUG);     \
    }; 


// 定义日志级别
namespace minirpc
{
enum LoggerLevel {
    INFO,
    ERROR,
    FATAL,
    DEBUG
};

// 一个日志类
class Logger : public nonecopyable {
private:
    Logger();
    
    int log_level_;
    bool is_async_{false};
    std::unique_ptr<BlockedQueue<std::string>> blocked_que_;
    std::thread async_log_write_thread_;
    FILE* log_file_;

    void async_log_write();
    
public:
    ~Logger();

    static Logger& GetInstanse();

    // 开启异步线程
    void enable_async_log_write();
    Logger& setLevel(int level);
    int getLevel() const;

    void log(const std::string&, int log_level);
};


    
} // namespace minirpc