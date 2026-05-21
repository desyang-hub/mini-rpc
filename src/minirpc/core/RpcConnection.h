#pragma once

#include "minirpc/core/IConnection.h"
#include "minirpc/net/BufferedConnection.h"

#include <chrono>
#include <string>

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

namespace minirpc {

class IConnectionPool;

class RpcConnection : public IConnection {
public:
    static constexpr int kDefaultTimeoutMs = 30000;

    explicit RpcConnection(const std::string& host, uint16_t port);
    ~RpcConnection() override;

    RpcConnection(const RpcConnection&) = delete;
    RpcConnection& operator=(const RpcConnection&) = delete;

    void connect();
    void send(const Bytes& data) override;
    int readMsg() override;
    bool decode(ProtocolHeader& header, std::string& body) override;
    bool isHealthy() const override;
    int fd() const override;
    std::string targetAddress() const override;
    bool pool_process(IConnection* conn) override;
    void set_master_pool(IConnectionPool* pool) override;
    void close() override;

    void sendWithTimeout(const Bytes& data, int timeoutMs);
    std::string recvWithTimeout(int timeoutMs);
    void setKeepAlive(bool enabled = true);
    void setNoDelay(bool enabled = true);
    int socketHandle() const { return sock_; }

private:
    IConnectionPool* master_pool_;
    BufferedConnection buf_conn_;

    std::string host_;
    uint16_t port_;
    int sock_;
    bool healthy_{true};
    bool closed_{false};
};

}  // namespace minirpc

