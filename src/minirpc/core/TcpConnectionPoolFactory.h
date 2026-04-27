#pragma once

#include "minirpc/core/IConnectionPoolFactory.h"

#include <unordered_map>
#include <shared_mutex>
#include <memory>


namespace minirpc
{

using IConnectionPoolPtr = std::unique_ptr<IConnectionPool>;

class TcpConnectionPoolFactory : public IConnectionPoolFactory
{
private:
    mutable std::shared_mutex sd_mutex_;
    std::unordered_map<std::string, IConnectionPoolPtr> connection_pools_;
public:
    TcpConnectionPoolFactory() = default;
    ~TcpConnectionPoolFactory() = default;

    // get connection Pool
    IConnectionPool* getConnectionPool(const std::string& server_name, const std::string& group_name = "DEFAULT_GROUP") override;
};


    
} // namespace minirpc
