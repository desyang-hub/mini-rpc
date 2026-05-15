#include <gtest/gtest.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <cstring>
#include <csignal>
#include <atomic>

#include "minirpc/common/Buffer.h"
#include "minirpc/net/utils.h"
#include "minirpc/core/RpcClient.h"
#include "minirpc/protocol/Encoder.h"
#include "minirpc/protocol/Decoder.h"
#include "minirpc/protocol/Protocol.h"

using namespace minirpc;

// 忽略 SIGPIPE
namespace {
    struct SigPipeHandler {
        SigPipeHandler() { signal(SIGPIPE, SIG_IGN); }
    } g_sigPipe;
}

// ============================================================
// RingBuffer 测试
// ============================================================

TEST(RingBufferTest, BasicAppendAndGet) {
    RingBuffer buf;
    std::string data = "Hello, World!";
    buf.append(data);

    EXPECT_EQ(buf.readable_bytes(), data.size());
    EXPECT_EQ(buf.writable_bytes(), 1024u - data.size());

    std::vector<char> out(data.size());
    buf.get_package_data(out.data(), out.size());
    EXPECT_EQ(std::string(out.begin(), out.end()), data);
}

TEST(RingBufferTest, Retrieve) {
    RingBuffer buf;
    buf.append("1234567890");
    EXPECT_EQ(buf.readable_bytes(), 10u);

    buf.retrieve(5);
    EXPECT_EQ(buf.readable_bytes(), 5u);

    std::vector<char> out(5);
    buf.get_package_data(out.data(), 5);
    EXPECT_EQ(std::string(out.begin(), out.end()), "67890");
}

TEST(RingBufferTest, RetrieveAll) {
    RingBuffer buf;
    buf.append("data");
    buf.retrieve_all();
    EXPECT_EQ(buf.readable_bytes(), 0u);
}

TEST(RingBufferTest, PeekData) {
    RingBuffer buf;
    ProtocolHeader header{};
    header.magic = 0x5250;
    header.body_len = 4;
    header.srv_name_len = 4;
    header.request_id = 1;
    header.code = 0;

    buf.append(reinterpret_cast<const char*>(&header), sizeof(header));

    ProtocolHeader peeked{};
    bool ok = buf.peek_data(&peeked, sizeof(peeked));
    EXPECT_TRUE(ok);
    EXPECT_EQ(peeked.magic, 0x5250);
}

TEST(RingBufferTest, WrapAround) {
    RingBuffer buf(1024);
    // 写入接近容量的数据
    std::string largeData(1014, 'A');
    buf.append(largeData);
    buf.retrieve(5); // 让 read_pos 偏移

    // 再写入跨越边界的数据
    std::string moreData = "more";
    buf.append(moreData);

    EXPECT_EQ(buf.readable_bytes(), largeData.size() - 5 + moreData.size());

    // 读取全部数据
    size_t readable = buf.readable_bytes();
    std::vector<char> out(readable);
    buf.get_package_data(out.data(), readable);

    std::string result(out.begin(), out.end());
    EXPECT_EQ(result.substr(0, largeData.size() - 5), std::string(largeData.begin() + 5, largeData.end()));
    EXPECT_EQ(result.substr(largeData.size() - 5), moreData);
}

TEST(RingBufferTest, ExpandIfNeeded) {
    RingBuffer buf(16); // 小容量
    std::string bigData(100, 'X');
    buf.append(bigData);

    EXPECT_EQ(buf.readable_bytes(), 100u);
    std::vector<char> out(100);
    buf.get_package_data(out.data(), 100);
    EXPECT_EQ(std::string(out.begin(), out.end()), bigData);
}

TEST(RingBufferTest, ReadableZeroInitially) {
    RingBuffer buf;
    EXPECT_EQ(buf.readable_bytes(), 0u);
}

// ============================================================
// 网络工具测试
// ============================================================

TEST(NetUtilsTest, SetNonBlocking) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(sockfd, 0);

    set_nonblocking(sockfd);

    // 验证非阻塞标志
    int flags = fcntl(sockfd, F_GETFL, 0);
    EXPECT_NE(flags, -1);
    EXPECT_TRUE(flags & O_NONBLOCK);

    ::close(sockfd);
}

// ============================================================
// Pipe 通信测试（验证双端读写，避免 socketpair 平台差异）
// ============================================================

TEST(SocketTest, PipeBasic) {
    int pipefd[2];
    ASSERT_EQ(pipe(pipefd), 0);

    // 写入
    const char* msg = "Hello from pipe";
    ssize_t n = write(pipefd[1], msg, strlen(msg));
    ASSERT_GT(n, 0);

    // 读取
    char buf[256];
    ssize_t nr = read(pipefd[0], buf, sizeof(buf));
    ASSERT_GT(nr, 0);

    std::string response(buf, nr);
    EXPECT_EQ(response, std::string(msg));

    ::close(pipefd[0]);
    ::close(pipefd[1]);
}

TEST(SocketTest, PipeDataIntegrity) {
    int pipefd[2];
    ASSERT_EQ(pipe(pipefd), 0);

    // 发送较大数据
    std::string data(1024, 'X');
    write(pipefd[1], data.data(), data.size());

    char buf[2048];
    ssize_t nr = read(pipefd[0], buf, sizeof(buf));
    EXPECT_EQ(nr, static_cast<ssize_t>(data.size()));
    EXPECT_EQ(std::string(buf, nr), data);

    ::close(pipefd[0]);
    ::close(pipefd[1]);
}
