#pragma once

#include <memory>
#include <vector>

#include <muduo/net/Callbacks.h>

#include "minirpc/common/nonecopyable.h"
#include "minirpc/net/EndPoint.h"
#include "minirpc/net/TcpClient.h"

namespace minirpc
{

// Connection pool manager - manages TcpClient connections to multiple endpoints
class ConnectionManager : public nonecopyable
{
public:
    ConnectionManager();
    ~ConnectionManager();

    // Get or create a connection to a single endpoint
    TcpClientPtr getConnection(const EndPoint& ep);

    // Get or create a connection from a list of endpoints
    TcpClientPtr getConnection(const std::vector<EndPoint>& eps);

    // Set message callback for all connections
    void setMessageCallback(muduo::net::MessageCallback cb);

    // Return a connection to the pool for reuse
    void recovery(TcpClientPtr ptr);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace minirpc
