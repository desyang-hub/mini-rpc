#pragma once

#include "minirpc/protocol/Protocol.h"
#include "minirpc/common/Type.h"

#include <memory>
#include <string>

namespace minirpc {

class IConnection;

struct Deleter;
class IConnectionPool;



// Smart pointer alias for polymorphic connection objects.
// using IConnectionPtr = std::unique_ptr<IConnection, IConnection::Deleter>;

// Abstract base class for all RPC connections.
class IConnection {
public:
    virtual ~IConnection() = default;

    // Send data over the connection.
    virtual void send(const Bytes& data) = 0;

    virtual int readMsg() = 0;
    
    virtual bool decode(ProtocolHeader& header, std::string& body) = 0;

    // Check if the connection is still healthy/usable.
    virtual bool isHealthy() const = 0;

    virtual int fd() const = 0;

    virtual void close() = 0;

    // Return the target address in "host:port" format.
    virtual std::string targetAddress() const = 0;

    // customer deleter
    // bool{true, false}; true: 绑定了线程池，false未绑定线程池
    virtual bool pool_process(IConnection* conn) {return false;}

    virtual void set_master_pool(IConnectionPool* pool) {}

    static std::unique_ptr<IConnection, Deleter> GetConnection(const std::string& host, uint16_t port);
};


struct Deleter {
    void operator()(IConnection* conn) const { // 加上 const 是标准做法
        if (conn) {
            // 回调处理：如果返回 true 表示已处理（例如放入线程池），不需要 delete
            // 如果返回 false 表示未处理，需要手动 delete
            if (!conn->pool_process(conn)) {
                delete conn;
            }
        }
    }
};


using IConnectionPtr = std::unique_ptr<IConnection, Deleter>;

}  // namespace minirpc
