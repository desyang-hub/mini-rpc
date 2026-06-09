#pragma once

#include "minirpc/protocol/Protocol.h"
#include "minirpc/common/Type.h"
#include "minirpc/common/utils.h"


// 编码的过程就是：填头 + 算CRC + 拼数据。

#include <cstdint>   // 添加这行
#include <cstddef>   // 可选，提供 size_t
#include <vector>
#include <cstring>

namespace minirpc
{



class Encoder
{
private:
public:
    static std::vector<uint8_t> Encode(ProtocolHeader& header, const std::string& body, uint8_t type = MSG_RESPONSE) {
        header.body_len = body.size();
        header.srv_name_len = 0;
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

        // 计算checksum（覆盖srv_name + body，防止srv_name被篡改）
        Bytes crc_input(header.srv_name_len + header.body_len);
        memcpy(crc_input.data(), srvName.data(), header.srv_name_len);
        memcpy(crc_input.data() + header.srv_name_len, body.data(), header.body_len);
        header.checksum = simple_crc32(crc_input.data(), crc_input.size());

        std::vector<uint8_t> packet(sizeof(header) + header.srv_name_len + header.body_len);

        // 将header拷贝到packet中
        memcpy(packet.data(), &header, sizeof(header));

        // 将srvName 拷贝到packet中
        memcpy(packet.data() + sizeof(header), srvName.data(), header.srv_name_len);

        // 将body拷贝到packet中
        memcpy(packet.data() + sizeof(header) + header.srv_name_len, body.data(), header.body_len);

        return packet;
    }

    // 编码，将必要的数据进行封装
    static Bytes Encode(const char* name, const void* data, size_t len, uint8_t type = MSG_REQUEST) {
        ProtocolHeader header;
        int header_len = sizeof(header);

        // magic
        header.magic = MAGIC_NUMBER;

        // 消息类型
        header.type = type;

        // version
        header.version = 1;

        // srvNameLen
        header.srv_name_len = strlen(name);

        // bodyLen
        header.body_len = len;

        size_t pkg_len = header_len + header.srv_name_len + len;

        // 计算checksum（覆盖srv_name + body，防止srv_name被篡改）
        Bytes crc_input(pkg_len + 4); // 剩下4字节是用于存放check_num
        memcpy(crc_input.data(), &header, header_len);
        memcpy(crc_input.data() + header_len, name, header.srv_name_len);
        memcpy(crc_input.data() + header_len + header.srv_name_len, data, len);
        uint32_t check_num = simple_crc32(crc_input.data(), pkg_len);
        memcpy(crc_input.data() + pkg_len, &check_num, 4);

        return crc_input;
    }

    static Bytes EncodeReq(uint64_t id, const char* name, const void* data, size_t len) {
        ProtocolHeader header;
        int header_len = sizeof(header);

        // magic
        header.magic = MAGIC_NUMBER;

        header.request_id = id;

        // 消息类型
        header.type = MSG_REQUEST;

        // version
        header.version = 1;

        // srvNameLen
        header.srv_name_len = strlen(name);

        // bodyLen
        header.body_len = len;

        size_t pkg_len = header_len + header.srv_name_len + len;

        // 计算checksum（覆盖srv_name + body，防止srv_name被篡改）
        Bytes crc_input(pkg_len + 4); // 剩下4字节是用于存放check_num
        memcpy(crc_input.data(), &header, header_len);
        memcpy(crc_input.data() + header_len, name, header.srv_name_len);
        memcpy(crc_input.data() + header_len + header.srv_name_len, data, len);
        uint32_t check_num = simple_crc32(crc_input.data(), pkg_len);
        memcpy(crc_input.data() + pkg_len, &check_num, 4);

        return crc_input;
    }

    // 这里是encode的基础实现，用于将header + srvNmae + body + check_num take package
    static Bytes Encode(const ProtocolHeader& header, const char* name, const void* data) {
        constexpr size_t header_len = sizeof(ProtocolHeader);
        size_t pkg_len = header_len + header.srv_name_len + header.body_len;

        // 计算checksum（覆盖srv_name + body，防止srv_name被篡改）
        Bytes crc_input(pkg_len + 4); // 剩下4字节是用于存放check_num
        memcpy(crc_input.data(), &header, header_len);
        if (header.srv_name_len)
            memcpy(crc_input.data() + header_len, name, header.srv_name_len);
        if (header.body_len)
            memcpy(crc_input.data() + header_len + header.srv_name_len, data, header.body_len);
        uint32_t check_num = simple_crc32(crc_input.data(), pkg_len);
        memcpy(crc_input.data() + pkg_len, &check_num, 4);

        return crc_input;
    }

    static Bytes ErrorRes(uint64_t request_id, uint8_t errcode, const char* errmsg) {
        ProtocolHeader header;
        header.code = errcode;
        header.request_id = request_id;
        header.srv_name_len = 0;
        header.body_len = strlen(errmsg);

        return Encode(header, nullptr, errmsg);
    }

    static Bytes SuccessRes(uint64_t request_id, const void* data, size_t len) {
        ProtocolHeader header;
        header.code = SUCCESS;
        header.request_id = request_id;
        header.srv_name_len = 0;
        header.body_len = len;

        return Encode(header, nullptr, data);
    }

    static Bytes SuccessRes(uint64_t request_id, const Bytes& data) {
        return SuccessRes(request_id, data.data(), data.size());
    }
};


} // namespace minirpc