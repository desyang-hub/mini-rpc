#include <gtest/gtest.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstring>
#include <csignal>

#include "minirpc/core/RpcServer.h"
#include "minirpc/core/RpcClient.h"
#include "minirpc/protocol/Encoder.h"
#include "minirpc/protocol/Decoder.h"
#include "minirpc/protocol/Serialize.h"

using namespace std;
using namespace minirpc;

// 忽略 SIGPIPE 防止客户端断开时 server 崩溃
namespace {
    struct SigPipeHandler {
        SigPipeHandler() {
            signal(SIGPIPE, SIG_IGN);
        }
    } g_sigPipeHandler;
}

// ============================================================
// 测试服务类
// ============================================================

class TestService {
public:
    int add(const int a, const int b) const { return a + b; }
    string echo(const string& s) const { return s; }
    int square(const int a) const { return a * a; }

    RPC_SERVICE_BIND(TestService, add, echo, square);
    RPC_SERVICE_STUB(TestService, add, echo, square);
};

RPC_SERVICE_REGISTER(TestService);

// ============================================================
// 轻量 Mock Server
// ============================================================

class MockServer {
public:
    // 启动在指定端口，返回实际端口
    int start(int port) {
        port_ = port;

        sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
        setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = port == 0 ? INADDR_ANY : inet_addr("127.0.0.1");

        bind(sockfd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        listen(sockfd_, 16);

        // 获取实际端口
        socklen_t addrlen = sizeof(addr);
        getsockname(sockfd_, reinterpret_cast<sockaddr*>(&addr), &addrlen);
        port_ = ntohs(addr.sin_port);

        fcntl(sockfd_, F_SETFL, O_NONBLOCK);

        running_ = true;
        loop_thread_ = std::thread(&MockServer::eventLoop, this);
        return port_;
    }

    void stop() {
        running_ = false;

        // 连接本地端口以唤醒 epoll_wait
        int cfd = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in target{};
        target.sin_family = AF_INET;
        target.sin_port = htons(port_);
        inet_pton(AF_INET, "127.0.0.1", &target.sin_addr);
        ::connect(cfd, reinterpret_cast<sockaddr*>(&target), sizeof(target));
        ::close(cfd);

        if (loop_thread_.joinable()) {
            loop_thread_.join();
        }
        ::close(sockfd_);
    }

    int port() const { return port_; }

private:
    void eventLoop() {
        int epfd = epoll_create1(EPOLL_CLOEXEC);

        epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = sockfd_;
        epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd_, &ev);

        epoll_event events[64];

        while (running_.load()) {
            int n = epoll_wait(epfd, events, 64, 100);
            if (n <= 0) continue;

            for (int i = 0; i < n; ++i) {
                if (events[i].data.fd == sockfd_) {
                    sockaddr_in client_addr{};
                    socklen_t len = sizeof(client_addr);
                    int cfd = accept(sockfd_, reinterpret_cast<sockaddr*>(&client_addr), &len);
                    if (cfd < 0) continue;
                    fcntl(cfd, F_SETFL, O_NONBLOCK);
                    epoll_event cev;
                    cev.events = EPOLLIN | EPOLLET;
                    cev.data.fd = cfd;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &cev);
                } else {
                    int cfd = events[i].data.fd;
                    char buf[4096];
                    while (true) {
                        int nr = recv(cfd, buf, sizeof(buf), 0);
                        if (nr <= 0) {
                            if (nr < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
                            ::close(cfd);
                            break;
                        }
                        string data(buf, nr);
                        ProtocolHeader header;
                        string body;
                        Bytes bytes_data(data.begin(), data.end());
                        if (Decoder::Decode(bytes_data, header, body)) {
                            string result;
                            string svc_name(reinterpret_cast<const char*>(data.data() + sizeof(ProtocolHeader)), header.srv_name_len);
                            if (RpcServer::Call(svc_name, body, result)) {
                                auto resp = Encoder::Encode(header, result);
                                sendAll(cfd, resp);
                            } else {
                                header.code = FAILED;
                                auto resp = Encoder::Encode(header, "");
                                sendAll(cfd, resp);
                            }
                        }
                    }
                }
            }
        }
        ::close(epfd);
    }

    static void sendAll(int fd, const Bytes& data) {
        size_t sent = 0;
        while (sent < data.size()) {
            ssize_t n = send(fd, data.data() + sent, data.size() - sent, MSG_NOSIGNAL);
            if (n <= 0) break;
            sent += n;
        }
    }

    int sockfd_ = -1;
    int port_ = 0;
    std::thread loop_thread_;
    std::atomic<bool> running_{false};
};

// ============================================================
// 测试 Fixture
// ============================================================

class RpcIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        RpcClient::ResetInstance();
        actualPort_ = mockServer_.start(0);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        RpcClient::setLocalServiceAddress("TestService", "127.0.0.1:" + std::to_string(actualPort_));
    }

    void TearDown() override {
        mockServer_.stop();
        RpcClient::ResetInstance();
    }

private:
    MockServer mockServer_;
    int actualPort_ = 0;
};

// ============================================================
// 集成测试
// ============================================================

TEST_F(RpcIntegrationTest, BasicAdd) {
    TestService::TestService_Stub stub;
    int result = stub.add(3, 5);
    EXPECT_EQ(result, 8);
}

TEST_F(RpcIntegrationTest, NegativeAdd) {
    TestService::TestService_Stub stub;
    int result = stub.add(-1, 1);
    EXPECT_EQ(result, 0);
}

TEST_F(RpcIntegrationTest, StringEcho) {
    TestService::TestService_Stub stub;
    string result = stub.echo("hello world");
    EXPECT_EQ(result, "hello world");
}

TEST_F(RpcIntegrationTest, StringEmpty) {
    TestService::TestService_Stub stub;
    string result = stub.echo("");
    EXPECT_EQ(result, "");
}

TEST_F(RpcIntegrationTest, MultipleCalls) {
    TestService::TestService_Stub stub;
    EXPECT_EQ(stub.add(1, 2), 3);
    EXPECT_EQ(stub.add(10, 20), 30);
    EXPECT_EQ(stub.add(-5, 5), 0);
}

TEST_F(RpcIntegrationTest, Square) {
    TestService::TestService_Stub stub;
    EXPECT_EQ(stub.square(4), 16);
    EXPECT_EQ(stub.square(0), 0);
    EXPECT_EQ(stub.square(-3), 9);
}

// ============================================================
// Encoder/Decoder 直接测试
// ============================================================

TEST(RpcEncodeDecodeTest, RoundtripTuple) {
    string srvName = "TestService.add";
    auto body = Serialize::Serialization(std::make_tuple(3, 5));
    auto bytes = Encoder::Encode(srvName, body);

    ProtocolHeader header;
    string decoded_body;
    string decoded_name;
    bool success = Decoder::Decode(bytes, header, decoded_name, decoded_body);

    EXPECT_TRUE(success);
    EXPECT_EQ(srvName, decoded_name);

    auto [a, b] = Serialize::Deserialization<std::tuple<int, int>>(decoded_body);
    EXPECT_EQ(a, 3);
    EXPECT_EQ(b, 5);
}
