#pragma once

#include "minirpc/core/IConnectionPool.h"

#include <memory>


namespace minirpc
{

class IConnectionPoolFactory
{
public:
    IConnectionPoolFactory() = default;
    ~IConnectionPoolFactory() = default;

    // get connection Pool
    virtual IConnectionPool* getConnectionPool(const std::string& server_name, const std::string& group_name = "DEFAULT_GROUP") = 0;

    static IConnectionPoolFactory* CreateConnectionPoolFactory();
    
};

using IConnectionPoolFactoryPtr = std::unique_ptr<IConnectionPoolFactory>;
    
} // namespace minirpc
