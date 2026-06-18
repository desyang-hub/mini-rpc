#pragma once

#include "minirpc/protocol/Protocol.h"
#include "minirpc/common/Type.h"
#include "minirpc/common/utils.h"

#include <cstring>
#include <string>
#include <vector>

namespace minirpc
{

class Encoder
{
private:
    // Build packet: [header | srv_name | body | check_num(4B)]
    // CRC covers header + srv_name + body
    static Bytes buildPacket(const ProtocolHeader& header, const std::string& srvName)
    {
        constexpr size_t header_len = sizeof(ProtocolHeader);
        size_t body_len   = header.body_len;
        size_t name_len   = header.srv_name_len;
        size_t pkg_len    = header_len + name_len + body_len;

        Bytes packet(pkg_len + 4);  // +4 for check_num at the end
        size_t off = 0;

        memcpy(packet.data() + off, &header, header_len); off += header_len;
        if (name_len) {
            memcpy(packet.data() + off, srvName.data(), name_len); off += name_len;
        }
        /* body is appended in place by caller via the body_len field */

        uint32_t check_num = simple_crc32(packet.data(), pkg_len);
        memcpy(packet.data() + pkg_len, &check_num, 4);

        return packet;
    }

public:
    // --- Request encoding ---

    // Encode a request with service name and serialized body
    static Bytes encodeRequest(const std::string& srvName, const std::string& body, uint64_t requestId)
    {
        ProtocolHeader header;
        header.magic      = MAGIC_NUMBER;
        header.version    = 1;
        header.type       = MSG_REQUEST;
        header.request_id = requestId;
        header.srv_name_len = srvName.size();
        header.body_len   = body.size();

        size_t header_len = sizeof(ProtocolHeader);
        size_t name_len   = header.srv_name_len;
        size_t blen       = header.body_len;
        size_t pkg_len    = header_len + name_len + blen;

        Bytes packet(pkg_len + 4);

        memcpy(packet.data(), &header, header_len);
        if (name_len) memcpy(packet.data() + header_len, srvName.data(), name_len);
        if (blen)     memcpy(packet.data() + header_len + name_len, body.data(), blen);

        uint32_t check_num = simple_crc32(packet.data(), pkg_len);
        memcpy(packet.data() + pkg_len, &check_num, 4);

        return packet;
    }

    // --- Response encoding ---

    static Bytes successResponse(uint64_t requestId, const std::string& body)
    {
        ProtocolHeader header;
        header.magic      = MAGIC_NUMBER;
        header.version    = 1;
        header.type       = MSG_RESPONSE;
        header.code       = SUCCESS;
        header.request_id = requestId;
        header.srv_name_len = 0;
        header.body_len   = body.size();

        size_t header_len = sizeof(ProtocolHeader);
        size_t pkg_len    = header_len + header.body_len;

        Bytes packet(pkg_len + 4);

        memcpy(packet.data(), &header, header_len);
        if (header.body_len) memcpy(packet.data() + header_len, body.data(), header.body_len);

        uint32_t check_num = simple_crc32(packet.data(), pkg_len);
        memcpy(packet.data() + pkg_len, &check_num, 4);

        return packet;
    }

    static Bytes errorResponse(uint64_t requestId, uint8_t code, const std::string& msg)
    {
        ProtocolHeader header;
        header.magic      = MAGIC_NUMBER;
        header.version    = 1;
        header.type       = MSG_RESPONSE;
        header.code       = code;
        header.request_id = requestId;
        header.srv_name_len = 0;
        header.body_len   = msg.size();

        size_t header_len = sizeof(ProtocolHeader);
        size_t pkg_len    = header_len + header.body_len;

        Bytes packet(pkg_len + 4);

        memcpy(packet.data(), &header, header_len);
        if (header.body_len) memcpy(packet.data() + header_len, msg.data(), header.body_len);

        uint32_t check_num = simple_crc32(packet.data(), pkg_len);
        memcpy(packet.data() + pkg_len, &check_num, 4);

        return packet;
    }
};

} // namespace minirpc
