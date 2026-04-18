#pragma once

#include "minirpc/protocol/Protocol.h"
#include "minirpc/common/Type.h"
#include "minirpc/common/utils.h"


// 编码的过程就是：填头 + 算CRC + 拼数据。

#include <iostream>
#include <vector>
#include <string.h>

namespace minirpc
{



class Encoder
{
private:
public:
    static std::vector<uint8_t> Encode(ProtocolHeader& header, const std::string& body, uint8_t type = MSG_RESPONSE) {
        // 计算checksum
        header.checksum = simple_crc32(reinterpret_cast<const uint8_t*>(body.data()), header.body_len);

        std::vector<uint8_t> packet(sizeof(header) + header.srv_name_len + header.body_len);

        // 将header拷贝到packet中
        memcpy(packet.data(), &header, sizeof(header));

        // 将srvName 拷贝到packet中
        // memcpy(packet.data(), srvName.data(), header.srv_name_len);

        // 将body拷贝到packet中
        memcpy(packet.data() + sizeof(header) + header.srv_name_len, body.data(), header.body_len);

        return packet;
    }

    // 编码
    static Bytes Encode(const std::string& srvName, const std::string& body, uint8_t type = MSG_REQUEST) {
        ProtocolHeader header;

        // magic
        header.magic = MAGIC_NUMBER;

        // 消息类型
        header.type = type;

        // version
        header.version = 1;

        // srvNameLen
        header.srv_name_len = srvName.size();

        // bodyLen
        header.body_len = body.size();

        // 计算checksum
        header.checksum = simple_crc32(reinterpret_cast<const uint8_t*>(body.data()), header.body_len);

        std::vector<uint8_t> packet(sizeof(header) + header.srv_name_len + header.body_len);

        // 将header拷贝到packet中
        memcpy(packet.data(), &header, sizeof(header));

        // 将srvName 拷贝到packet中
        memcpy(packet.data() + sizeof(header), srvName.data(), header.srv_name_len);

        // 将body拷贝到packet中
        memcpy(packet.data() + sizeof(header) + header.srv_name_len, body.data(), header.body_len);

        return packet;
    }
};


} // namespace minirpc