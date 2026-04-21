#include "minirpc/common/ThreadPool.h"

namespace minirpc
{
    

ThreadPool::ThreadPool(int pool_size, int thread_container_init_size) : stop_(false), 
workers_(thread_container_init_size) {
    for (int i = 0; i < pool_size; i++)
    {
        workers_.emplace_back([this](){
            
            while (true) {
                Job job = nullptr;

                // 此处需要操作队列，而队列是共享资源
                { 
                    std::unique_lock<std::mutex> lock(mutex_);

                    // 检查条件
                    condition_.wait(lock, [this](){
                        return !this->jobs_.empty() || this->stop_;
                    });

                    if (!jobs_.empty()) {
                        // 取出job
                        job = std::move(jobs_.front());
                        jobs_.pop();
                    } // 空且已经停止则退出
                    else {
                        return; // 退出任务线程
                    }
                }

                // 此时已经释放锁，执行任务
                job();
            }
        });
    }
    
}


ThreadPool::~ThreadPool()
{
    // 回收资源，确保所有线程停止，并join
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = true;
    }
    condition_.notify_all(); // 通知所有工作线程进行状态检查 此时stop = true; 即将退出

    for (auto &worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    // 打印
    // std::cout << "[ThreadPool] All worker threads stopped." << std::endl;
}


} // namespace minirpc