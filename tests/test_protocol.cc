#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <string>
#include "minirpc/protocol/Encoder.h"
#include "minirpc/protocol/Decoder.h"
#include "minirpc/protocol/Protocol.h"
#include "minirpc/common/utils.h"

using namespace minirpc;

// ============================================================
// Encoder/Decoder 基础测试
// ============================================================

TEST(ProtocolTest, EncodeDecodeBasic) {
    std::string srvName = "TestService.hello";
    std::string body = "hello body";
    Bytes encode_bytes = Encoder::Encode(srvName, body);

    ProtocolHeader header;
    std::string decoded_body;
    std::string decoded_name;
    bool success = Decoder::Decode(encode_bytes, header, decoded_name, decoded_body);

    EXPECT_TRUE(success);
    EXPECT_EQ(srvName, decoded_name);
    EXPECT_EQ(body, decoded_body);
}

TEST(ProtocolTest, EncodeDecodeEmptyBody) {
    std::string srvName = "TestService.empty";
    std::string body = "";
    Bytes encode_bytes = Encoder::Encode(srvName, body);

    ProtocolHeader header;
    std::string decoded_body;
    std::string decoded_name;
    bool success = Decoder::Decode(encode_bytes, header, decoded_name, decoded_body);

    EXPECT_TRUE(success);
    EXPECT_EQ(srvName, decoded_name);
    EXPECT_EQ(body, decoded_body);
}

TEST(ProtocolTest, EncodeDecodeLargeBody) {
    std::string srvName = "TestService.large";
    std::string body(10240, 'X');  // 10KB
    Bytes encode_bytes = Encoder::Encode(srvName, body);

    ProtocolHeader header;
    std::string decoded_body;
    std::string decoded_name;
    bool success = Decoder::Decode(encode_bytes, header, decoded_name, decoded_body);

    EXPECT_TRUE(success);
    EXPECT_EQ(srvName, decoded_name);
    EXPECT_EQ(body, decoded_body);
}

TEST(ProtocolTest, EncodeDecodeVeryLargeBody) {
    std::string srvName = "TestService.veryLarge";
    std::string body(1024 * 1024, 'A');  // 1MB
    Bytes encode_bytes = Encoder::Encode(srvName, body);

    ProtocolHeader header;
    std::string decoded_body;
    std::string decoded_name;
    bool success = Decoder::Decode(encode_bytes, header, decoded_name, decoded_body);

    EXPECT_TRUE(success);
    EXPECT_EQ(srvName, decoded_name);
    EXPECT_EQ(body, decoded_body);
}

TEST(ProtocolTest, EncodeDecodeMultiWordBody) {
    std::string srvName = "TestService.multi";
    std::string body = "word1 word2 word3 中文测试";
    Bytes encode_bytes = Encoder::Encode(srvName, body);

    ProtocolHeader header;
    std::string decoded_body;
    std::string decoded_name;
    bool success = Decoder::Decode(encode_bytes, header, decoded_name, decoded_body);

    EXPECT_TRUE(success);
    EXPECT_EQ(srvName, decoded_name);
    EXPECT_EQ(body, decoded_body);
}

TEST(ProtocolTest, DecodeInvalidMagic) {
    std::string srvName = "TestService.bad";
    std::string body = "data";
    Bytes encode_bytes = Encoder::Encode(srvName, body);

    // 破坏魔数
    encode_bytes[0] = 0x00;
    encode_bytes[1] = 0x00;

    ProtocolHeader header;
    std::string decoded_body;
    std::string decoded_name;
    int result = Decoder::Decode(encode_bytes, header, decoded_name, decoded_body);

    EXPECT_EQ(result, ERR);
}

TEST(ProtocolTest, DecodeShortPacket) {
    // 构造一个只有魔数+1字节的短包
    std::vector<uint8_t> short_packet(2, 0);
    // 写入正确魔数
    short_packet[0] = (MAGIC_NUMBER >> 8) & 0xFF;
    short_packet[1] = MAGIC_NUMBER & 0xFF;

    ProtocolHeader header;
    std::string decoded_body;
    std::string decoded_name;
    int result = Decoder::Decode(short_packet, header, decoded_name, decoded_body);

    EXPECT_EQ(result, UN_FINISH);
}

TEST(ProtocolTest, DecodeInvalidCRC) {
    std::string srvName = "TestService.badCRC";
    std::string body = "test data";
    Bytes encode_bytes = Encoder::Encode(srvName, body);

    // 破坏 body 内容（CRC 会不匹配）
    encode_bytes[encode_bytes.size() - 1] ^= 0xFF;

    ProtocolHeader header;
    std::string decoded_body;
    std::string decoded_name;
    int result = Decoder::Decode(encode_bytes, header, decoded_name, decoded_body);

    EXPECT_EQ(result, ERR);
}

TEST(ProtocolTest, DecodeCheckValid) {
    std::string srvName = "TestService.check";
    std::string body = "body content";
    Bytes encode_bytes = Encoder::Encode(srvName, body);

    int result = Decoder::Check(encode_bytes);
    EXPECT_GT(result, 0);  // 返回完整包长度
}

TEST(ProtocolTest, DecodeCheckInvalid) {
    std::vector<uint8_t> bad_packet(2, 0);
    bad_packet[0] = (MAGIC_NUMBER >> 8) & 0xFF;
    bad_packet[1] = MAGIC_NUMBER & 0xFF;

    int result = Decoder::Check(bad_packet);
    EXPECT_EQ(result, UN_FINISH);
}

TEST(ProtocolTest, EncodeDecodeServiceName) {
    std::string srvName = "com.example.UserService.login";
    std::string body = "{\"name\":\"test\"}";
    Bytes encode_bytes = Encoder::Encode(srvName, body);

    ProtocolHeader header;
    std::string decoded_body;
    std::string decoded_name;
    bool success = Decoder::Decode(encode_bytes, header, decoded_name, decoded_body);

    EXPECT_TRUE(success);
    EXPECT_EQ(srvName, decoded_name);
    EXPECT_EQ(body, decoded_body);
}

// ============================================================
// Encoder 自身测试
// ============================================================

TEST(EncoderTest, EncodeReturnsValidBytes) {
    auto bytes = Encoder::Encode("Test", "data");
    EXPECT_GT(bytes.size(), 0);
}

TEST(EncoderTest, EncodeContainsMagic) {
    auto bytes = Encoder::Encode("Test", "data");
    // 小端序机器上，低字节在前
    uint16_t magic = static_cast<uint16_t>(bytes[0]) | (static_cast<uint16_t>(bytes[1]) << 8);
    EXPECT_EQ(magic, MAGIC_NUMBER);
}
