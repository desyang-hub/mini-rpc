#pragma once

#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <utility>
#include <unistd.h>
#include <thread>
#include <functional>
#include <iostream>

namespace minirpc
{

const int THREAD_POOL_DEFAULT_SIZE = 4;
const int THREAD_CONTAINER_INIT_SIZE = 64;


/**
 * 线程池类，用于管理一组线程来执行任务
 */
/**
 * @brief 线程池类，用于管理和复用线程执行任务
 */
class ThreadPool
{
private:
    using Job = std::function<void()>;

    std::vector<std::thread> workers_;    // 存储工作线程的容器
    std::queue<Job> jobs_;                // 存储待执行任务的队列
    std::mutex mutex_;                  // 用于保护任务队列的互斥锁，防止多线程同时访问
    std::condition_variable condition_;  // 用于线程间同步的条件变量，用于线程的等待和唤醒
    bool stop_;                           // 标记线程池是否停止，用于控制线程池的生命周期

public:
    /**
     * @brief 构造函数，初始化线程池
     * @param pool_size 线程池中线程的数量，默认值为 THREAD_POOL_DEFAULT_SIZE
     */
    ThreadPool(int pool_size = THREAD_POOL_DEFAULT_SIZE, int thread_container_init_size = THREAD_CONTAINER_INIT_SIZE);

    /**
     * @brief 析构函数，清理线程池资源
     */
    ~ThreadPool();

public:
    /**
     * @brief 提交任务到线程池
     * @param job 要提交的任务函数，支持任意可调用对象
     * @note 使用右值引用实现完美转发，避免不必要的拷贝
     */
    template<class FUNC>
    inline void submit(FUNC &&job);
};

// 提交任务，注意job是一个无参函数
template<class FUNC>
void ThreadPool::submit(FUNC &&job) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        jobs_.emplace(std::forward<FUNC>(job));
    }

    // 唤醒一个线程,这个动作无需锁
    condition_.notify_one();
}


} // namespace minirpc