#pragma once

#include <cstdint>
#include <cstddef>

namespace minirpc
{

constexpr uint16_t MAGIC_NUMBER = 0x5250;  // 'R' 'P'

enum MessageType : uint8_t {
    MSG_REQUEST  = 1,
    MSG_RESPONSE = 2,
    MSG_HEARTBEAT = 3
};

enum SerializeType : uint8_t {
    SERIALIZE_JSON     = 1,
    SERIALIZE_PROTOBUF = 2
};

enum StateCode : uint8_t {
    SUCCESS = 0,
    FAILED  = 1,
    TIMEOUT = 2
};

// Packed protocol header - 27 bytes
#pragma pack(push, 1)
struct ProtocolHeader
{
    uint16_t magic       = MAGIC_NUMBER;
    uint8_t  version     = 1;
    uint8_t  type        = 0;
    uint8_t  serialize   = SERIALIZE_JSON;
    uint8_t  compress    = 0;
    uint64_t request_id  = 0;
    uint32_t body_len    = 0;
    uint32_t checksum    = 0;
    uint32_t srv_name_len = 0;
    uint8_t  code        = 0;
};
#pragma pack(pop)

static_assert(sizeof(ProtocolHeader) == 27, "ProtocolHeader size mismatch");

} // namespace minirpc
