#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <future>
#include <memory>
#include <utility>
#include <stdexcept>
#include <functional>
#include <condition_variable>
#include <iostream>

namespace minirpc
{

class ThreadPool
{
public:
    using TaskHandler = std::function<void()>;
private:
    std::queue<TaskHandler> queue_;
    std::vector<std::thread> workers_;
    mutable std::mutex mutex_;
    bool is_running_;
    std::condition_variable condition_;

public:
    explicit ThreadPool(int pool_size = -1);
    ~ThreadPool();

    template<class F, class ...Args>
    auto enqueue(F&& f, Args&& ...args) -> std::future<typename std::result_of<F(Args...)>::type>;

    // 禁用拷贝
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
};

inline ThreadPool::ThreadPool(int pool_size) : is_running_(true)
{
    if (pool_size < 0) pool_size = std::thread::hardware_concurrency();
    workers_.reserve(pool_size);
    // 启用多个线程，用于执行提交的任务
    for (int i = 0; i < pool_size; ++i) {
        // 创建线程，放入队列中
        workers_.emplace_back([this]{
            
            while (true) {
                TaskHandler task{};
                {
                    // 阻塞从队列中取出任务
                    std::unique_lock<std::mutex> lock(mutex_);
                    
                    condition_.wait(lock, [this]{
                        return !queue_.empty() || !is_running_;
                    });

                    if (!queue_.empty()) {
                        task = std::move(queue_.front());
                        queue_.pop();
                    }
                    else {
                        // 即将退出
                        break;
                    }
                }
                
                // 执行任务，可是我们要返回的是一个future, 通过promise来获取
                // 无需异常捕获，package_task已经将任务封装，会自动进行异常捕获，并在future.get时，自动抛出
                task();
            }

        });
    }
}

inline ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        is_running_ = false;
    }
    // 所有线程进入销毁
    condition_.notify_all();

    for (std::thread& woker : workers_) {
        if (woker.joinable())
            woker.join();
    }
}

template<class F, class ...Args>
inline auto ThreadPool::enqueue(F&& f, Args&& ...args) -> std::future<typename std::result_of<F(Args...)>::type> {
    using return_type = typename std::result_of<F(Args...)>::type;

    // 创建一个异步任务
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<return_type> res = task->get_future();

    // 加锁添加任务
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!is_running_) {
            throw std::runtime_error("enqueue on stoped ThreadPool");
        }

        queue_.emplace([task]{
            (*task)(); // 执行
        });
    }

    // 唤醒线程来取得任务
    condition_.notify_one();

    return res;
}


} // namespace minirpc