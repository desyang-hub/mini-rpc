#pragma once

#include "minirpc/core/IConnection.h"

namespace minirpc
{



class IConnectionPool
{
public:
    IConnectionPool() = default;
    
    virtual ~IConnectionPool() = default;

    // 借连接：若池空且未达上限 → 新建；否则等待（初期可返回 nullptr）
    virtual IConnectionPtr getConnection() = 0;
    
    // 归还连接：放回池中（不关闭）
    virtual void returnConnection(IConnection* conn) = 0;

    static IConnectionPool* GetConnectionPool(const std::string& server_name, const std::string& group_name);
};

    
} // namespace minirpc
