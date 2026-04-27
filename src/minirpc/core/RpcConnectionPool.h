#pragma once

#include "minirpc/core/IConnectionPool.h"
#include "minirpc/common/ThreadPool.h"

#include <mutex>
#include <queue>
#include <string>
#include <atomic>

/**
 * @brief 对于一个连接池，对应一个EventLoop，需要管理所有的连接的消息触发，当连接中断，需要将连接状态置为可析构状态，当调用getConnection()方法时，如果连接为可析构状态，那么进入析构
 * 
 * 
 */

namespace minirpc
{

    
class RpcConnectionPool : public IConnectionPool
{
private:
    // 用户确保多线程安全
    mutable std::mutex mutex_;
    // 用于放置连接
    std::queue<IConnectionPtr> connection_que_;
    // 服务名@组名
    std::string server_name_;
    std::string group_name_;

    int epollfd_ = -1;
    std::atomic_bool is_running_ = true;
    ThreadPool threadPool_;

private:
    IConnectionPtr connect();

    // 需要有一个方法来处理事件循环
    void loop();

    void addConnectionListener(IConnection* conn);

    void onMessage(IConnection* conn);

public:
    RpcConnectionPool(const std::string& server_name, const std::string& group_name = "DEFAULT_GROUP");
    ~RpcConnectionPool() = default;

    // 借连接：若池空且未达上限 → 新建；否则等待（初期可返回 nullptr）
    virtual IConnectionPtr getConnection() override;

    // 归还连接：放回池中（不关闭）
    virtual void returnConnection(IConnection* conn) override;
};


} // namespace minirpc
