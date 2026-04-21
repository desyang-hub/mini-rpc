#pragma once
// 用于定义协议结构体
#include <iostream>
#include <cstdint>   // 添加这行
#include <cstddef>   // 可选，提供 size_t

namespace minirpc
{


// 1. 定义魔数，用于快速识别协议
const uint16_t MAGIC_NUMBER = 0x5250; // 0x5250 ('R''P')

// 2. 定义消息类型 1byte
enum MessageType : uint8_t {
    MSG_REQUEST = 1,    // 请求
    MSG_RESPONSE = 2,   // 响应
    MSG_HEARTABAT = 3   // 心跳
};

// 3. 定义序列化类型
enum SerializeType : uint8_t {
    SERIALIZE_JSON = 1,
    SERIALIZE_PROTOBUF = 2
};

// 定义协议结构体
// 2 + 1x4 + 8 + 4 + 4 + 4 -> 26 个 -> 4x8 = 32
#pragma pack(push, 1)
struct ProtocolHeader
{
    // 2 byte
    uint16_t magic = MAGIC_NUMBER; 
    
    // 1 byte
    uint8_t version = 1;

    // 1 byte
    uint8_t type = 0; // 消息类型

    // 1 byte
    uint8_t serialize = SERIALIZE_JSON; // 序列化算法

    // 1 byte
    uint8_t compress = 0; // 压缩算法

    // 8 byte
    uint64_t request_id = 200; // 请求标识符

    // 4 byte
    uint32_t body_len = 0; // 消息体长度

    // 4 byte
    uint32_t checksum = 0; // 消息体的 CRC32 校验

    // 4 byte 预留长度, 用于表示服务名的长度吧 UserService.Login
    uint32_t srv_name_len = 0;

    ProtocolHeader() = default;
};
#pragma pack(pop)

static_assert(sizeof(ProtocolHeader) == 26, "Header size mismatch");



} // namespace minirpc