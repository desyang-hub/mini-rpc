/**
 * @FilePath     : /mini-rpc/include/minirpc/net/ConnectionManager.h
 * @Description  :
 * @Author       : desyang
 * @Date         : 2026-06-10 11:46:52
 * @LastEditors  : desyang
 * @LastEditTime : 2026-06-16 11:23:37
**/
#pragma once

#include "minirpc/net/TcpClient.h"
#include "minirpc/net/EndPoint.h"
#include "minirpc/common/nonecopyable.h"
#include <unordered_map>
#include <mutex>
#include <muduo/net/Callbacks.h>
#include <vector>

namespace minirpc
{

class ConnectionManager;

// Forward declare implementation
class ConnectionManagerImpl;

// 连接管理器，用户可以通过 List<EndPoint> 来获取
class ConnectionManager : public nonecopyable
{
public:
    ConnectionManager();
    ~ConnectionManager();

    TcpClientPtr getConnection(const EndPoint& ep);

    TcpClientPtr getConnection(const std::vector<EndPoint>& eps);

    void setMessageCallback(muduo::net::MessageCallback cb);

    void recovery(TcpClientPtr ptr);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace minirpc
