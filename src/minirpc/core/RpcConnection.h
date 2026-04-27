#pragma once

#include "minirpc/core/IConnection.h"
#include "minirpc/common/Buffer.h"

#include <chrono>
#include <string>

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

namespace minirpc {

// 前向声明
class IConnectionPool;

// A raw socket-based TCP connection using send/recv.
// Implements RpcConnection with synchronous I/O and deadline support.
class RpcConnection : public IConnection {
public:
    // Send/receive timeout, default 30 seconds.
    static constexpr int kDefaultTimeoutMs = 30000;

    explicit RpcConnection(const std::string& host, uint16_t port);
    ~RpcConnection() override;

    // Disable copy
    RpcConnection(const RpcConnection&) = delete;
    RpcConnection& operator=(const RpcConnection&) = delete;

    // Establish the TCP connection to the remote server.
    // Throws std::runtime_error on failure.
    void connect();

    // Send data over the connection with a timeout.
    // Throws std::runtime_error on timeout or failure.
    void send(const Bytes& data) override;

    virtual int readMsg() override;


    virtual bool decode(ProtocolHeader& header, std::string& body) override;

    // Check if the connection is still healthy/usable.
    bool isHealthy() const override;

    virtual int fd() const override;

    // Return the target address in "host:port" format.
    std::string targetAddress() const override;

    bool pool_process(IConnection* conn) override;

    void set_master_pool(IConnectionPool* pool) override;

    // Close the underlying socket.
    void close() override;

    // Send data with a specific timeout (milliseconds).
    void sendWithTimeout(const Bytes& data, int timeoutMs);

    // Read data with a specific timeout (milliseconds).
    std::string recvWithTimeout(int timeoutMs);

    // Set socket options: SO_KEEPALIVE, TCP_NODELAY.
    void setKeepAlive(bool enabled = true);
    void setNoDelay(bool enabled = true);

    int socketHandle() const { return sock_; }

private:
    int timeoutRecv(int timeoutMs);
    int timeoutSend(int timeoutMs);

    // 用于绑定自己所在的池，析构过程，需要自动回收资源
    IConnectionPool* master_pool_;

    RingBuffer buf_;
    int pkg_len_ = -1;
    ProtocolHeader header_;

    std::string host_;
    uint16_t port_;
    int sock_;
    bool healthy_{true};
    bool closed_{false};
};

}  // namespace minirpc
