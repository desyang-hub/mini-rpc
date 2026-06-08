#pragma once

#include "minirpc/common/nonecopyable.h"

#include <mutex>
#include <queue>
#include <condition_variable>

namespace minirpc
{

template<class T>
class BlockedQueue : public nonecopyable
{
private:
    std::mutex mutex_;
    std::queue<T> que_;
    std::condition_variable condition_;
    bool is_close_{false};

public:
    BlockedQueue();
    ~BlockedQueue();

    void close();
public:

    void enqueue(const T& t);
    void enqueue(T&& t);

    // 从阻塞队列中取出数据
    bool pop(T& t);
};

template<class T>
BlockedQueue<T>::BlockedQueue() : is_close_(false) {}


template<class T>
BlockedQueue<T>::~BlockedQueue() {
    this->close();
}

template<class T>
void BlockedQueue<T>::close() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (is_close_) return;
        is_close_ = true;
    }
    condition_.notify_all();
}

template<class T>
void BlockedQueue<T>::enqueue(const T& t) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        que_.push(t);
        
    }
    condition_.notify_one();
}

template<class T>
void BlockedQueue<T>::enqueue(T&& t) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        que_.emplace(std::move(t));
    }
    condition_.notify_one();
}

// 从阻塞队列中取出数据
template<class T>
bool BlockedQueue<T>::pop(T& t) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    condition_.wait(lock, [this]() {
        return !que_.empty() || is_close_;
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
    
} // namespace minirpc
