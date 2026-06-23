/**
 * @FilePath     : /mini-rpc/include/minirpc/common/logger.h
 * @Description  :  
 * @Author       : desyang
 * @Date         : 2026-03-25 20:05:54
 * @LastEditors  : desyang
 * @LastEditTime : 2026-06-09 11:51:06
**/
#pragma once

#include "minirpc/common/nonecopyable.h"
#include "minirpc/common/BlockedQueue.h"

#include <string>
#include <memory>
#include <atomic>
#include <fstream>
#include <thread>
#include <stdexcept>
#include <unistd.h>
#include <cstring>

// 开启异步日志写入
#define ENABLE_ASYNC_LOGING() \
minirpc::Logger::GetInstance().enable_async_log_write()

#define LOG_INFO(format, ...) \
    do { \
        if (minirpc::Logger::GetInstance().getLevel() >= minirpc::INFO) { \
            char buf[1024]; \
            snprintf(buf, 1024, format, ##__VA_ARGS__); \
            minirpc::Logger::GetInstance().log(buf, minirpc::INFO); \
        } \
    } while (0)

#define LOG_ERROR(format, ...) \
    do { \
        if (minirpc::Logger::GetInstance().getLevel() >= minirpc::ERROR) { \
            char buf[1024]; \
            snprintf(buf, 1024, format, ##__VA_ARGS__); \
            minirpc::Logger::GetInstance().log(buf, minirpc::ERROR); \
        } \
    } while (0)

#define LOG_FATAL(format, ...) \
    do { \
        char buf[1024]; \
        snprintf(buf, 1024, "%s:%d %s ", __FILE__, __LINE__, __FUNCTION__); \
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), format, ##__VA_ARGS__); \
        minirpc::Logger::GetInstance().log(buf, minirpc::FATAL); \
        throw std::runtime_error(buf); \
    } while (0)

#define LOG_DEBUG(format, ...) \
    do { \
        if (minirpc::Logger::GetInstance().getLevel() >= minirpc::DEBUG) { \
            char buf[1024]; \
            snprintf(buf, 1024, format, ##__VA_ARGS__); \
            minirpc::Logger::GetInstance().log(buf, minirpc::DEBUG); \
        } \
    } while (0) 


// 定义日志级别
namespace minirpc
{
enum LoggerLevel {
    FATAL,
    ERROR,
    INFO,
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

    static Logger& GetInstance();

    // 开启异步线程
    void enable_async_log_write();
    Logger& setLevel(int level);
    int getLevel() const;

    void log(const std::string&, int log_level);
};


    
} // namespace minirpc