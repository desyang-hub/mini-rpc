#include "RpcConnection.h"

#include "RpcConnectionPool.h"
#include "minirpc/common/logger.h"
#include "minirpc/protocol/Decoder.h"

#include <cstring>
#include <stdexcept>
#include <string>
#include <fcntl.h>
#include <system_error>
#include <netinet/tcp.h>


namespace minirpc {

namespace {

// 安全地获取错误信息
std::string get_error_message(int err) {
    char buf[256];
    // POSIX 版 strerror_r：返回 int，成功为 0
    if (strerror_r(err, buf, sizeof(buf)) == 0) {
        return std::string(buf);
    } else {
        return "Unknown error (" + std::to_string(err) + ")";
    }
}

void checkError(const std::string& msg, int result) {
    if (result < 0) {
        throw std::runtime_error(msg + ": " + std::strerror(errno));
    }
}

void checkWouldBlock(const std::string& msg, int result) {
    if (result < 0 && errno != EINPROGRESS) {
        throw std::runtime_error(msg);
    }
}

void waitForReady(int sock, bool forRead, int timeoutMs) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(sock, &set);

    struct timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;

    int ret = select(sock + 1,
                     forRead ? &set : nullptr,
                     forRead ? nullptr : &set,
                     nullptr,
                     &tv);
    checkError("select", ret);
}

}  // namespace

// 工厂方法
IConnectionPtr IConnection::GetConnection(const std::string& host, uint16_t port) {

    return std::unique_ptr<RpcConnection, Deleter>(new RpcConnection(host, port));
}


RpcConnection::RpcConnection(const std::string& host, uint16_t port)
    : host_(host),
      port_(port),
      sock_(-1),
      healthy_(true),
      closed_(false),
      master_pool_(nullptr) {
        connect();
      }

RpcConnection::~RpcConnection() {
    close();
}

void RpcConnection::connect() {
    if (closed_) {
        throw std::runtime_error("Connection is already closed");
    }

    // Create a TCP socket.
    sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    checkError("socket", sock_ >= 0);

    // Resolve address: try inet_pton first (IPv4), then getaddrinfo.
    struct sockaddr_in addr4;
    memset(&addr4, 0, sizeof(addr4));
    addr4.sin_family = AF_INET;
    addr4.sin_port = htons(port_);

    if (inet_pton(AF_INET, host_.c_str(), &addr4.sin_addr) != 1) {
        // Hostname resolution via getaddrinfo (works for both IPv4 and IPv6).
        struct addrinfo hints, *result = nullptr;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        std::string portStr = std::to_string(port_);
        int rc = getaddrinfo(host_.c_str(), portStr.c_str(), &hints, &result);
        if (rc != 0 || result == nullptr) {
            if (sock_ != -1) {
                ::close(sock_);
            }
            throw std::runtime_error("DNS resolution failed: " + host_);
        }

        memcpy(&addr4, result->ai_addr, result->ai_addrlen);
        freeaddrinfo(result);
    }

    sockaddr_in addrStorage = addr4;
    sockaddr* addrPtr = reinterpret_cast<sockaddr*>(&addrStorage);
    socklen_t addrLen = sizeof(addrStorage);

    // Set non-blocking for connect timeout.
    checkError("fcntl", fcntl(sock_, F_SETFL, O_NONBLOCK) >= 0);

    // Initiate non-blocking connect.
    int rc = ::connect(sock_, addrPtr, addrLen);
    checkWouldBlock("connect", rc);

    // Wait for connect to finish via select.
    waitForReady(sock_, false /* write */, 5000);

    // Check result via SO_ERROR.
    int soError = 0;
    socklen_t optLen = sizeof(soError);

    getsockopt(sock_, SOL_SOCKET, SO_ERROR, &soError, &optLen);
    if (soError != 0) {
        close();
        throw std::runtime_error("connect failed: " + std::to_string(soError));
    }

    // Back to blocking mode.
    fcntl(sock_, F_SETFL, 0);

    healthy_ = true;

    
}

void RpcConnection::send(const Bytes& data) {
    if (!healthy_ || closed_) {
        throw std::runtime_error("Connection is not healthy");
    }
    sendWithTimeout(data, kDefaultTimeoutMs);
}

int RpcConnection::readMsg() {
    int errno_code;
    int byteNum = buf_.read_fd(sock_, &errno_code);

    // 读数据
    if (byteNum <= 0) {
        if (byteNum == 0) {
            // 正常断开
            return -1;
        }

        // 读完了，等待下一次触发
        if (errno_code == EAGAIN || errno_code == EWOULDBLOCK) {
            return 0;
        }

        // 发生了异常
        // perror("read error");
        LOG_ERROR("read error %s", strerror(errno_code));
        return -1;
    }

    // 还没有计算head 判断包头是否存在
    if (pkg_len_ == -1 && buf_.readable_bytes() >= sizeof(ProtocolHeader)) {
        buf_.peek_data(&header_, sizeof(header_));
        pkg_len_ = header_.body_len + header_.srv_name_len + sizeof(header_);
    }

    return byteNum;
}


bool RpcConnection::decode(ProtocolHeader& header, std::string& body) {
    if (pkg_len_ == -1 || buf_.readable_bytes() < pkg_len_) return false;
        
    std::vector<uint8_t> bytes(pkg_len_);
    
    buf_.get_package_data(bytes.data(), pkg_len_);

    bool is_success = Decoder::Decode(bytes, header, body);
    // 标记为-1
    pkg_len_ = -1;

    return is_success;
}

bool RpcConnection::isHealthy() const {
    if (closed_) {
        return false;
    }
    return sock_ >= 0;
}

int RpcConnection::fd() const {
    return sock_;
}

bool RpcConnection::pool_process(IConnection* conn) {
    if (master_pool_) {
        master_pool_->returnConnection(conn);
        master_pool_ = nullptr;
        return true;
    }
    return false;
}

void RpcConnection::set_master_pool(IConnectionPool* pool) {
    master_pool_ = pool;
}

std::string RpcConnection::targetAddress() const {
    return host_ + ":" + std::to_string(port_);
}

void RpcConnection::close() {
    if (closed_) {
        return;
    }

    closed_ = true;
    healthy_ = false;

    if (sock_ >= 0) {
        shutdown(sock_, SHUT_RDWR);
        ::close(sock_);
        sock_ = -1;
    }
}

void RpcConnection::sendWithTimeout(const Bytes& data, int timeoutMs) {
    if (!healthy_ || closed_) {
        throw std::runtime_error("Connection is not healthy");
    }

    // Set send timeout via select polling for writable.
    for (size_t sent = 0; sent < data.size();) {
        waitForReady(sock_, false /* write */, timeoutMs);

        int chunk = static_cast<int>(std::min(data.size() - sent,
                                              static_cast<size_t>(4096)));
        int n = static_cast<int>(::send(sock_, data.data() + sent, static_cast<int>(chunk), 0));
        if (n < 0) {
            int err = 0;

            err = errno;
            if (err == EAGAIN || err == EWOULDBLOCK) {
                waitForReady(sock_, false, 100);
                continue;
            }
            ::close(sock_);
            sock_ = -1;
            throw std::runtime_error("send failed: " + get_error_message(err));
        }
        sent += n;
    }
}

std::string RpcConnection::recvWithTimeout(int timeoutMs) {
    if (!healthy_ || closed_) {
        throw std::runtime_error("Connection is not healthy");
    }

    char buffer[4096];
    waitForReady(sock_, true /* read */, timeoutMs);

    int n = static_cast<int>(::recv(sock_, buffer, sizeof(buffer), 0));
    if (n < 0) {
        int err = 0;
        err = errno;
        ::close(sock_);
        sock_ = -1;
        throw std::runtime_error("recv failed: " + get_error_message(err));
    }
    if (n == 0) {
        // Connection closed by peer.
        close();
        return "";
    }
    return std::string(buffer, n);
}

void RpcConnection::setKeepAlive(bool enabled) {
    int opt = enabled ? 1 : 0;
    setsockopt(sock_, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
}

void RpcConnection::setNoDelay(bool enabled) {
    int opt = enabled ? 1 : 0;
    setsockopt(sock_, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
}

}  // namespace minirpc
