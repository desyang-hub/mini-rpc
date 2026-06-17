#include <gtest/gtest.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/epoll.h>
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

#include <csignal>
#include <cstdio>
#include <execinfo.h>

using namespace std;
using namespace minirpc;

// 忽略 SIGPIPE 防止客户端断开时 server 崩溃
namespace {
    // struct SigPipeHandler {
    //     SigPipeHandler() {
    //         signal(SIGPIPE, SIG_IGN);
    //     }
    // } g_sigPipeHandler;

    void sigusr1_handler(int sig) {
        // ⚠️ 注意：信号处理函数中只能调用 async-signal-safe 函数
        // write 是安全的，printf 不安全但调试时可临时用
        const char msg[] = "\n>>> CAUGHT SIGUSR1! Backtrace:\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        
        void* buffer[20];
        int nptrs = backtrace(buffer, 20);
        backtrace_symbols_fd(buffer, nptrs, STDERR_FILENO);
        
        // 恢复默认处理后重新触发，以便 CTest 正常捕获
        signal(SIGUSR1, SIG_DFL);
        raise(SIGUSR1);
    }
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
// 测试 Fixture
// ============================================================

class RpcIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        signal(SIGUSR1, sigusr1_handler);  // 【新增】捕获并打印调用栈

        server_worker = std::thread([]{
            RpcServer::GetInstance().Start(8083);
        });

        sleep(1);
    }

    void TearDown() override {
        RpcServer::GetInstance().Stop();

        if (server_worker.joinable()) {
            server_worker.join();
        }
        std::cout << "Stop success." << std::endl;
    }

private:
    std::thread server_worker;

};

// ============================================================
// 集成测试
// ============================================================

TEST_F(RpcIntegrationTest, ALL) {
    TestService::TestService_Stub stub;
    {
        int result = stub.add(3, 5);
        EXPECT_EQ(result, 8);
    }
    {
        int result = stub.add(-1, 1);
        EXPECT_EQ(result, 0);
    }
    {
        string result = stub.echo("hello world");
        EXPECT_EQ(result, "hello world");
    }
    {
        string result = stub.echo("");
        EXPECT_EQ(result, "");
    }
    {
        EXPECT_EQ(stub.add(1, 2), 3);
        EXPECT_EQ(stub.add(10, 20), 30);
        EXPECT_EQ(stub.add(-5, 5), 0);
    }
    {
        EXPECT_EQ(stub.square(4), 16);
        EXPECT_EQ(stub.square(0), 0);
        EXPECT_EQ(stub.square(-3), 9);
    }
}

// TEST_F(RpcIntegrationTest, BasicAdd) {
//     TestService::TestService_Stub stub;
//     int result = stub.add(3, 5);
//     EXPECT_EQ(result, 8);
// }

// TEST_F(RpcIntegrationTest, NegativeAdd) {
//     TestService::TestService_Stub stub;
//     int result = stub.add(-1, 1);
//     EXPECT_EQ(result, 0);
// }

// TEST_F(RpcIntegrationTest, StringEcho) {
//     TestService::TestService_Stub stub;
//     string result = stub.echo("hello world");
//     EXPECT_EQ(result, "hello world");
// }

// TEST_F(RpcIntegrationTest, StringEmpty) {
//     TestService::TestService_Stub stub;
//     string result = stub.echo("");
//     EXPECT_EQ(result, "");
// }

// TEST_F(RpcIntegrationTest, MultipleCalls) {
//     TestService::TestService_Stub stub;
//     EXPECT_EQ(stub.add(1, 2), 3);
//     EXPECT_EQ(stub.add(10, 20), 30);
//     EXPECT_EQ(stub.add(-5, 5), 0);
// }

// TEST_F(RpcIntegrationTest, Square) {
//     TestService::TestService_Stub stub;
//     EXPECT_EQ(stub.square(4), 16);
//     EXPECT_EQ(stub.square(0), 0);
//     EXPECT_EQ(stub.square(-3), 9);
// }

// ============================================================
// Encoder/Decoder 直接测试
// ============================================================

// TEST(RpcEncodeDecodeTest, RoundtripTuple) {
//     string srvName = "TestService.add";
//     auto body = Serialize::Serialization(std::make_tuple(3, 5));
//     auto bytes = Encoder::Encode(srvName, body);

//     ProtocolHeader header;
//     string decoded_body;
//     string decoded_name;
//     bool success = Decoder::Decode(bytes, header, decoded_name, decoded_body);

//     EXPECT_TRUE(success);
//     EXPECT_EQ(srvName, decoded_name);

//     auto [a, b] = Serialize::Deserialization<std::tuple<int, int>>(decoded_body);
//     EXPECT_EQ(a, 3);
//     EXPECT_EQ(b, 5);
// }
