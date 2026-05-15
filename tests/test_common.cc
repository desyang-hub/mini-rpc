#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include "minirpc/common/utils.h"
#include "minirpc/common/logger.h"
#include "minirpc/common/timeStamp.h"
#include "minirpc/common/Random.h"

using namespace minirpc;

// ============================================================
// CRC32 测试
// ============================================================

TEST(CommonTest, CRC32KnownValue) {
    // CRC32 of empty string should be 0
    uint32_t crc = simple_crc32(reinterpret_cast<const uint8_t*>(""), 0);
    EXPECT_EQ(crc, 0);
}

TEST(CommonTest, CRC32NonEmpty) {
    std::string data = "hello";
    uint32_t crc = simple_crc32(reinterpret_cast<const uint8_t*>(data.data()), data.size());
    EXPECT_NE(crc, 0);
}

TEST(CommonTest, CRC32Deterministic) {
    std::string data = "test data";
    uint32_t crc1 = simple_crc32(reinterpret_cast<const uint8_t*>(data.data()), data.size());
    uint32_t crc2 = simple_crc32(reinterpret_cast<const uint8_t*>(data.data()), data.size());
    EXPECT_EQ(crc1, crc2);
}

TEST(CommonTest, CRC32DifferentData) {
    std::string data1 = "hello";
    std::string data2 = "world";
    uint32_t crc1 = simple_crc32(reinterpret_cast<const uint8_t*>(data1.data()), data1.size());
    uint32_t crc2 = simple_crc32(reinterpret_cast<const uint8_t*>(data2.data()), data2.size());
    EXPECT_NE(crc1, crc2);
}

TEST(CommonTest, CRC32LargeData) {
    std::vector<uint8_t> data(10240, 0xAB);
    uint32_t crc = simple_crc32(data.data(), data.size());
    EXPECT_NE(crc, 0);
}

// ============================================================
// TimeStamp 测试
// ============================================================

TEST(CommonTest, TimestampNow) {
    TimeStamp ts = TimeStamp::Now();
    std::string str = ts.toString();
    EXPECT_FALSE(str.empty());
    EXPECT_NE(str, "0");
}

// ============================================================
// Random 测试
// ============================================================

TEST(RandomTest, RandInRange) {
    for (int i = 0; i < 100; ++i) {
        int val = Random::RandInt(0, 10);
        EXPECT_GE(val, 0);
        EXPECT_LT(val, 10);
    }
}

TEST(RandomTest, RandIntDeterministicRange) {
    std::atomic<int> seen[10]{};
    for (int i = 0; i < 1000; ++i) {
        int val = Random::RandInt(0, 10);
        EXPECT_GE(val, 0);
        EXPECT_LT(val, 10);
    }
}

// ============================================================
// Logger 测试
// ============================================================

TEST(LoggerTest, LoggerNoCrash) {
    LOG_INFO("test info %d", 1);
    LOG_DEBUG("test debug %s", "hello");
    LOG_ERROR("test error %d", 42);
}

TEST(LoggerTest, LoggerWithPrintfStyle) {
    LOG_INFO("format test: %s %d %f", "test", 123, 3.14);
    LOG_ERROR("error format: %s", "error message");
}
