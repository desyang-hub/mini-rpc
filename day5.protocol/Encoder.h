#pragma once
// 编码的过程就是：填头 + 算CRC + 拼数据。

#include <iostream>
#include <vector>
#include <string.h>

#include "protocol.h"


class Encoder
{
private:
public:
    // 简单的 CRC32 模拟（实际项目中建议用 zlib 或 boost）
    static uint32_t simple_crc32(const void* data, size_t len) {
        // 这里仅作演示，实际请用标准 CRC32 算法
        return 0xDEADBEEF; 
    }

    // 编码
    static std::vector<uint8_t> Encode(uint8_t type, const std::string& body) {
        ProtocolHeader header;

        // 消息类型
        header.type = type;

        // version
        header.version = 1;

        header.body_len = body.size();

        // 计算checksum
        header.checksum = simple_crc32(body.data(), header.body_len);

        std::vector<uint8_t> packet(sizeof(header) + header.body_len);

        // 将数据拷贝到packet中
        memcpy(packet.data(), &header, sizeof(header));

        // 将body拷贝到packet中
        memcpy(packet.data() + sizeof(header), body.data(), header.body_len);

        return packet;
    }
};