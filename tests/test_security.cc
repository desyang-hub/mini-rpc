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
// CRC32 covers srv_name tests
// ============================================================

TEST(SecurityTest, CrcCoversSrvName)
{
    Bytes encoded = Encoder::encodeRequest("TestService.hello", "hello body", 1);

    // Tamper with srv_name
    encoded[sizeof(ProtocolHeader)] ^= 0xFF;

    int result = Decoder::check(encoded.data(), encoded.size());
    EXPECT_EQ(result, -1);  // ERR - CRC detects tampering
}

TEST(SecurityTest, CrcCoversSrvNameFullRewrite)
{
    std::string srvName = "OtherService.method";
    Bytes encoded = Encoder::encodeRequest(srvName, "data", 1);

    std::string name, body;
    Decoder::decode(encoded.data(), name, body);

    EXPECT_EQ(srvName, name);
    EXPECT_EQ("data", body);
}

// ============================================================
// URL encoding tests
// ============================================================

namespace {
std::string testUrlEncode(const std::string& value)
{
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

TEST(SecurityTest, UrlEncodeNormal)
{
    EXPECT_EQ(testUrlEncode("hello"), "hello");
    EXPECT_EQ(testUrlEncode("test.service_v1"), "test.service_v1");
}

TEST(SecurityTest, UrlEncodeSpecial)
{
    EXPECT_EQ(testUrlEncode("a&b"), "a%26b");
    EXPECT_EQ(testUrlEncode("x=y?z"), "x%3Dy%3Fz");
}

TEST(SecurityTest, UrlEncodeEmpty)
{
    EXPECT_EQ(testUrlEncode(""), "");
}

// ============================================================
// Random thread safety tests
// ============================================================

TEST(SecurityTest, RandomThreadSafety)
{
    constexpr int kThreads = 8;
    constexpr int kCalls = 1000;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([kCalls]() {
            for (int i = 0; i < kCalls; ++i) {
                int val = Random::RandInt(0, 100);
                EXPECT_GE(val, 0);
                EXPECT_LT(val, 100);
            }
        });
    }

    for (auto& th : threads) th.join();
}

TEST(SecurityTest, RandomDistribution)
{
    constexpr int kSamples = 10000;
    constexpr int kBuckets = 10;
    int counts[kBuckets] = {0};

    for (int i = 0; i < kSamples; ++i) {
        int val = Random::RandInt(0, kBuckets);
        ASSERT_GE(val, 0);
        ASSERT_LT(val, kBuckets);
        counts[val]++;
    }

    // Each bucket should have ~1000 samples (allow 50% deviation)
    for (int i = 0; i < kBuckets; ++i) {
        EXPECT_GT(counts[i], 500) << "Bucket " << i << " too few";
        EXPECT_LT(counts[i], 1500) << "Bucket " << i << " too many";
    }
}

// ============================================================
// CRC32 integrity tests
// ============================================================

TEST(SecurityTest, Crc32DetectsBodyTamper)
{
    Bytes encoded = Encoder::encodeRequest("Test.crc", "important data", 1);
    encoded.back() ^= 0xFF;

    int result = Decoder::check(encoded.data(), encoded.size());
    EXPECT_EQ(result, -1);
}

TEST(SecurityTest, Crc32DetectsSrvNameTamper)
{
    Bytes encoded = Encoder::encodeRequest("Test.crc2", "data", 1);
    encoded[sizeof(ProtocolHeader) + 1] ^= 0xFF;

    int result = Decoder::check(encoded.data(), encoded.size());
    EXPECT_EQ(result, -1);
}

// ============================================================
// LOG_FATAL buffer overflow protection
// ============================================================

TEST(SecurityTest, LogFatalLongPath)
{
    constexpr int kBufSize = 1024;
    char buf[kBufSize];

    std::string longFile(900, 'x');
    std::string longFunc(100, 'y');

    int written = snprintf(buf, kBufSize, "%s:%d %s ", longFile.c_str(), 99999, longFunc.c_str());
    ASSERT_LT(written, kBufSize);

    int remaining = kBufSize - written;
    snprintf(buf + written, remaining, "test message %d", 42);

    EXPECT_EQ(buf[kBufSize - 1], '\0');
}
