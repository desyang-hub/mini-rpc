#include "TcpConnectionPoolFactory.h"

#include <mutex>


namespace minirpc
{

// 创建连接池工厂实例
IConnectionPoolFactory* IConnectionPoolFactory::CreateConnectionPoolFactory() {
    return new TcpConnectionPoolFactory();
}

IConnectionPool* TcpConnectionPoolFactory::getConnectionPool(const std::string& server_name, const std::string& group_name) {
    std::string key = server_name + "@" + group_name;

    IConnectionPool* ptr{};
    {
        std::shared_lock<std::shared_mutex> lock(sd_mutex_);
        auto it = connection_pools_.find(key);
        if (it != connection_pools_.end()) {
            ptr = it->second.get();
        }
    }
    
    // 如果池已经建立了，那么直接返回
    if (ptr) return ptr;

    // 否则获取池，将池放到factory管理，并返回ptr
    ptr = IConnectionPool::GetConnectionPool(server_name, group_name);
    if (message_handler_) {
        ptr->setMessageHandler(message_handler_);
    }
    {
        std::unique_lock<std::shared_mutex> lock(sd_mutex_);
        connection_pools_[key] = std::unique_ptr<IConnectionPool>(ptr);
    }
    return ptr;
}
    
} // namespace minirpc
