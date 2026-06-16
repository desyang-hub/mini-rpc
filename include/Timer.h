/**
 * @FilePath     : /mini-rpc/include/Timer.h
 * @Description  :  
 * @Author       : desyang
 * @Date         : 2026-06-16 09:07:41
 * @LastEditors  : desyang
 * @LastEditTime : 2026-06-16 09:07:42
**/
#pragma once

#include <iostream>
#include <chrono>
#include <memory>


/**
 * @usage:
 * 
 * TIMER_START() => TIMER_START_TAG(None); 
 * 
 * TIMER_START_TAG(TAG); 通过TAG来区分变量名，请确保同一作用域内部TAG不同
 * 
 * TIMER_FINISH_TAG(TAG); 用于截断计时，不使用该函数，计时器将直接作用到结尾
 * 
 */


#define TIMER_START() \
    TIMER_START_TAG(None)

#define TIMER_FINISH() \
    TIMER_FINISH_TAG(None)

#define TIMER_START_TAG(TAG) \
    std::unique_ptr<Timer> TAG##__ = std::make_unique<Timer>(__FILE__, __LINE__, __FUNCTION__, #TAG)

#define TIMER_FINISH_TAG(TAG) \
    TAG##__->set_func_end_line(__LINE__); \
    TAG##__.reset();


class Timer
{
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
    std::string func_name_;
    std::string file_path_;
    std::string tag_;
    size_t func_end_line_ = 0;
    size_t line_;

public:
    void set_func_end_line(size_t func_end_line) {
        func_end_line_ = func_end_line;
    }
public:
    Timer(const std::string& file_path, size_t line, const std::string& func_name, const std::string tag="") : func_name_(func_name),
    file_path_(file_path),
    line_(line),
    tag_(tag),
    start_(std::chrono::high_resolution_clock::now()) {

    }
    ~Timer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_);
        
        std::cout << "=================== Start ====================" << std::endl;
        std::cout << "StartPos: " << file_path_ << ":" << line_ << std::endl;

        std::cout << "MethodNmae: " << func_name_ << std::endl;
        std::cout << "TimerTag: " << (tag_.empty() ? "None" : tag_) << std::endl;
        std::cout << "TimeSpends: " << duration.count() << " ms" << std::endl;

        if (func_end_line_ == 0) {
            std::cout << "Function execute finish!" << std::endl;
        }
        else {
            std::cout << "FinishPos: " << file_path_ << ":" << func_end_line_ << std::endl;
        }
        
        std::cout << "=================== Finish ===================" << std::endl;
    }
};
