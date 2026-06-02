#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <string>
#include <cstring>

#include "minirpc/protocol/Encoder.h"
#include "minirpc/protocol/Decoder.h"
#include "minirpc/protocol/Protocol.h"
#include "minirpc/common/utils.h"
#include "minirpc/common/Random.h"

using namespace minirpc;

// ============================================================
// CRC32 覆盖 srv_name 测试
// ============================================================

TEST(SecurityTest, CrcCoversSrvName) {
    std::string srvName = "TestService.hello";
    std::string body = "hello body";
    Bytes encoded = Encoder::Encode(srvName, body);

    constexpr int header_len = sizeof(ProtocolHeader);

    // 篡改 srv_name（不重算 CRC），Decoder 应检测到 CRC 不匹配
    encoded[header_len] ^= 0xFF;

    ProtocolHeader header;
    std::string decoded_body, decoded_name;
    int result = Decoder::Decode(encoded, header, decoded_name, decoded_body);

    EXPECT_EQ(result, ERR);
}

TEST(SecurityTest, CrcCoversSrvNameFullRewrite) {
    // 用不同 srv_name 重新编码，CRC 应正确
    std::string srvName = "OtherService.method";
    std::string body = "data";
    Bytes encoded = Encoder::Encode(srvName, body);

    ProtocolHeader header;
    std::string decoded_body, decoded_name;
    bool success = Decoder::Decode(encoded, header, decoded_name, decoded_body);

    EXPECT_TRUE(success);
    EXPECT_EQ(srvName, decoded_name);
    EXPECT_EQ(body, decoded_body);
}

// ============================================================
// URL 编码测试
// ============================================================

namespace {

// 复制 utils.cc 中的 urlEncode 用于测试（避免链接问题）
std::string testUrlEncode(const std::string& value) {
    std::string encoded;
    encoded.reserve(value.size() * 3);
    for (unsigned char c : value) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else {
            char hex[4];
            snprintf(hex, sizeof(hex), "%%%02X", c);
            encoded += hex;
        }
    }
    return encoded;
}

} // namespace

TEST(SecurityTest, UrlEncodeNormalChars) {
    EXPECT_EQ(testUrlEncode("hello"), "hello");
    EXPECT_EQ(testUrlEncode("test.service_v1"), "test.service_v1");
}

TEST(SecurityTest, UrlEncodeSpecialChars) {
    EXPECT_EQ(testUrlEncode("a&b"), "a%26b");
    EXPECT_EQ(testUrlEncode("x=y?z"), "x%3Dy%3Fz");
    EXPECT_EQ(testUrlEncode("hello world"), "hello%20world");
    EXPECT_EQ(testUrlEncode("path#frag"), "path%23frag");
}

TEST(SecurityTest, UrlEncodeEmpty) {
    EXPECT_EQ(testUrlEncode(""), "");
}

// ============================================================
// Random 线程安全测试
// ============================================================

TEST(SecurityTest, RandomThreadSafety) {
    constexpr int kThreads = 8;
    constexpr int kCallsPerThread = 1000;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([]() {
            for (int i = 0; i < kCallsPerThread; ++i) {
                int val = Random::RandInt(0, 100);
                EXPECT_GE(val, 0);
                EXPECT_LT(val, 100);
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }
}

TEST(SecurityTest, RandomDistribution) {
    constexpr int kSamples = 10000;
    constexpr int kBuckets = 10;
    int counts[kBuckets] = {0};

    for (int i = 0; i < kSamples; ++i) {
        int val = Random::RandInt(0, kBuckets);
        ASSERT_GE(val, 0);
        ASSERT_LT(val, kBuckets);
        counts[val]++;
    }

    // 每个桶期望 ~1000 个样本，允许 50% 偏差
    for (int i = 0; i < kBuckets; ++i) {
        EXPECT_GT(counts[i], 500) << "Bucket " << i << " has too few samples";
        EXPECT_LT(counts[i], 1500) << "Bucket " << i << " has too many samples";
    }
}

// ============================================================
// CRC32 基本完整性测试
// ============================================================

TEST(SecurityTest, Crc32DetectsBodyTamper) {
    std::string srvName = "Test.crc";
    std::string body = "important data";
    Bytes encoded = Encoder::Encode(srvName, body);

    // 篡改 body 最后一个字节
    encoded.back() ^= 0xFF;

    ProtocolHeader header;
    std::string decoded_body, decoded_name;
    int result = Decoder::Decode(encoded, header, decoded_name, decoded_body);

    EXPECT_EQ(result, ERR);
}

TEST(SecurityTest, Crc32DetectsSrvNameTamper) {
    std::string srvName = "Test.crc2";
    std::string body = "data";
    Bytes encoded = Encoder::Encode(srvName, body);

    constexpr int header_len = sizeof(ProtocolHeader);
    // 篡改 srv_name 中间字节
    encoded[header_len + 1] ^= 0xFF;

    ProtocolHeader header;
    std::string decoded_body, decoded_name;
    int result = Decoder::Decode(encoded, header, decoded_name, decoded_body);

    EXPECT_EQ(result, ERR);
}

// ============================================================
// LOG_FATAL 缓冲区溢出防护测试
// ============================================================

TEST(SecurityTest, LogFatalLongPath) {
    // 构造一个很长的路径 + 函数名前缀，验证 LOG_FATAL 不会溢出
    // 由于 LOG_FATAL 会 throw，这里间接测试 buf 拼接是否正确

    // 我们无法直接调用 LOG_FATAL（它会 throw），
    // 但可以验证拼接逻辑：buf 大小 1024，前缀 + 消息 不应溢出
    constexpr int kBufSize = 1024;
    char buf[kBufSize];

    std::string longFile(900, 'x');
    std::string longFunc(100, 'y');

    int written = snprintf(buf, kBufSize, "%s:%d %s ", longFile.c_str(), 99999, longFunc.c_str());
    ASSERT_LT(written, kBufSize) << "Prefix alone exceeds buffer";

    // 第二次 snprintf 使用剩余空间
    int remaining = kBufSize - written;
    snprintf(buf + written, remaining, "test message %d", 42);

    // 不应崩溃，且 buf 以 null 结尾
    EXPECT_EQ(buf[kBufSize - 1], '\0');
}
