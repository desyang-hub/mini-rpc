#pragma once

#include "common/nonecopyable.h"

#include <mutex>
#include <queue>
#include <condition_variable>

#include <iostream>

namespace common
{

template<class T>
class BlockedQueue : public nonecopyable
{
private:
    std::mutex mutex_;
    std::queue<T> que_;
    std::condition_variable condition_;
    bool is_close{false};

public:
    BlockedQueue() {}
    ~BlockedQueue() {
        close();
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (is_close) return;
            is_close = true;
        }
        condition_.notify_all();
    }
public:

    void enqueue(const T& t) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            que_.push(t);
            
        }
        condition_.notify_one();
    }

    void enqueue(T&& t) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            que_.emplace(std::move(t));
        }
        condition_.notify_one();
    }

    // 从阻塞队列中取出数据
    bool pop(T& t) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        condition_.wait(lock, [this]() {
            return !que_.empty() || is_close;
        });

        if (!que_.empty()) {
            t = std::move(que_.front());
            que_.pop();
            return true;
        }
        else {
            return false;
        }
    }
};
    
} // namespace common
