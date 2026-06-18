#include <gtest/gtest.h>
#include <cstring>
#include <string>
#include <vector>

#include "minirpc/protocol/Encoder.h"
#include "minirpc/protocol/Decoder.h"
#include "minirpc/protocol/Protocol.h"
#include "minirpc/common/utils.h"

using namespace minirpc;

// ============================================================
// Encoder/Decoder roundtrip tests
// ============================================================

TEST(ProtocolTest, EncodeDecodeBasic)
{
    std::string srvName = "TestService.hello";
    std::string body = "hello body";
    Bytes encoded = Encoder::encodeRequest(srvName, body, 42);

    EXPECT_GT(encoded.size(), 0);

    // Check length
    int pkg_len = Decoder::check(encoded.data(), encoded.size());
    EXPECT_EQ(pkg_len, static_cast<int>(encoded.size()));

    // Decode
    std::string decoded_name;
    std::string decoded_body;
    uint64_t rid = Decoder::decode(encoded.data(), decoded_name, decoded_body);

    EXPECT_EQ(rid, 42ULL);
    EXPECT_EQ(srvName, decoded_name);
    EXPECT_EQ(body, decoded_body);
}

TEST(ProtocolTest, EncodeDecodeEmptyBody)
{
    std::string srvName = "TestService.empty";
    Bytes encoded = Encoder::encodeRequest(srvName, "", 1);

    std::string name, body;
    Decoder::decode(encoded.data(), name, body);

    EXPECT_EQ(srvName, name);
    EXPECT_EQ("", body);
}

TEST(ProtocolTest, EncodeDecodeLargeBody)
{
    std::string srvName = "TestService.large";
    std::string body(10240, 'X');  // 10KB
    Bytes encoded = Encoder::encodeRequest(srvName, body, 2);

    std::string name, decoded;
    Decoder::decode(encoded.data(), name, decoded);

    EXPECT_EQ(srvName, name);
    EXPECT_EQ(body, decoded);
}

TEST(ProtocolTest, EncodeDecodeVeryLargeBody)
{
    std::string srvName = "TestService.veryLarge";
    std::string body(1024 * 1024, 'A');  // 1MB
    Bytes encoded = Encoder::encodeRequest(srvName, body, 3);

    std::string name, decoded;
    Decoder::decode(encoded.data(), name, decoded);

    EXPECT_EQ(srvName, name);
    EXPECT_EQ(body, decoded);
}

TEST(ProtocolTest, EncodeDecodeMultiWordBody)
{
    std::string srvName = "TestService.multi";
    std::string body = "word1 word2 word3 中文测试";
    Bytes encoded = Encoder::encodeRequest(srvName, body, 4);

    std::string name, decoded;
    Decoder::decode(encoded.data(), name, decoded);

    EXPECT_EQ(srvName, name);
    EXPECT_EQ(body, decoded);
}

TEST(ProtocolTest, EncodeDecodeServiceName)
{
    std::string srvName = "com.example.UserService.login";
    std::string body = "{\"name\":\"test\"}";
    Bytes encoded = Encoder::encodeRequest(srvName, body, 5);

    std::string name, decoded;
    Decoder::decode(encoded.data(), name, decoded);

    EXPECT_EQ(srvName, name);
    EXPECT_EQ(body, decoded);
}

// ============================================================
// Decoder validation tests
// ============================================================

TEST(ProtocolTest, DecodeInvalidMagic)
{
    std::string srvName = "TestService";
    Bytes encoded = Encoder::encodeRequest(srvName, "data", 1);

    encoded[0] = 0x00;
    encoded[1] = 0x00;

    int result = Decoder::check(encoded.data(), encoded.size());
    EXPECT_EQ(result, -1);  // ERR
}

TEST(ProtocolTest, DecodeShortPacket)
{
    std::vector<uint8_t> short_packet(2);
    short_packet[0] = (MAGIC_NUMBER >> 8) & 0xFF;
    short_packet[1] = MAGIC_NUMBER & 0xFF;

    int result = Decoder::check(short_packet.data(), short_packet.size());
    EXPECT_EQ(result, 0);  // UN_FINISH
}

TEST(ProtocolTest, DecodeInvalidCRC)
{
    std::string srvName = "TestService";
    Bytes encoded = Encoder::encodeRequest(srvName, "test data", 1);

    // Tamper with body
    encoded[encoded.size() - 5] ^= 0xFF;

    int result = Decoder::check(encoded.data(), encoded.size());
    EXPECT_EQ(result, -1);  // ERR - CRC mismatch
}

TEST(ProtocolTest, DecodeTamperedSrvName)
{
    std::string srvName = "Target.Service";
    Bytes encoded = Encoder::encodeRequest(srvName, "payload", 1);

    // Tamper with service name
    encoded[sizeof(ProtocolHeader)] ^= 0xFF;

    int result = Decoder::check(encoded.data(), encoded.size());
    EXPECT_EQ(result, -1);  // ERR - CRC detects tampering
}

// ============================================================
// Encoder tests
// ============================================================

TEST(EncoderTest, EncodeReturnsValidBytes)
{
    auto bytes = Encoder::encodeRequest("Test", "data", 1);
    EXPECT_GT(bytes.size(), 0);
}

TEST(EncoderTest, EncodeContainsMagic)
{
    auto bytes = Encoder::encodeRequest("Test", "data", 1);
    uint16_t magic = static_cast<uint16_t>(bytes[0]) | (static_cast<uint16_t>(bytes[1]) << 8);
    EXPECT_EQ(magic, MAGIC_NUMBER);
}

TEST(EncoderTest, EncodeRequestPreservesId)
{
    auto bytes = Encoder::encodeRequest("Service", "body", 999);
    const auto* hdr = reinterpret_cast<const ProtocolHeader*>(bytes.data());
    EXPECT_EQ(hdr->request_id, 999ULL);
    EXPECT_EQ(hdr->type, MSG_REQUEST);
}

// ============================================================
// Response encoding tests
// ============================================================

TEST(EncoderTest, SuccessResponse)
{
    auto bytes = Encoder::successResponse(100, "ok");
    EXPECT_GT(bytes.size(), 0);

    const auto* hdr = reinterpret_cast<const ProtocolHeader*>(bytes.data());
    EXPECT_EQ(hdr->request_id, 100ULL);
    EXPECT_EQ(hdr->code, SUCCESS);
    EXPECT_EQ(hdr->type, MSG_RESPONSE);
}

TEST(EncoderTest, ErrorResponse)
{
    auto bytes = Encoder::errorResponse(200, FAILED, "not found");
    EXPECT_GT(bytes.size(), 0);

    const auto* hdr = reinterpret_cast<const ProtocolHeader*>(bytes.data());
    EXPECT_EQ(hdr->request_id, 200ULL);
    EXPECT_EQ(hdr->code, FAILED);
}

// ============================================================
// Decoder Response tests
// ============================================================

TEST(DecoderTest, DecodeResponse)
{
    auto bytes = Encoder::successResponse(50, "result_data");
    Response resp;
    int rid = Decoder::Decode(bytes.data(), resp);

    EXPECT_EQ(rid, 50ULL);
    EXPECT_EQ(resp.state, SUCCESS);
    EXPECT_EQ(resp.data, "result_data");
}
