#pragma once

#include "minirpc/protocol/Protocol.h"
#include "minirpc/common/Type.h"
#include "minirpc/common/Response.h"
#include "minirpc/common/utils.h"

#include <cstdint>
#include <cstring>
#include <string>

namespace minirpc
{

constexpr uint32_t MAX_BODY_SIZE = 64 * 1024 * 1024; // 64MB

enum DecodeState : int8_t {
    ERR       = -1,
    UN_FINISH =  0,
    FINISHED  =  1
};

class Decoder
{
private:
    static constexpr size_t hdr_len = sizeof(ProtocolHeader);

    // Validate header and CRC; returns packet length (without check_num) or error
    // >0 = valid packet length, 0 = incomplete, -1 = error
    static int validate(const void* data, size_t len)
    {
        if (len < hdr_len) return 0;

        const auto* hdr = reinterpret_cast<const ProtocolHeader*>(data);
        if (hdr->magic != MAGIC_NUMBER) return -1;
        if (hdr->body_len > MAX_BODY_SIZE) return -1;

        size_t pkg_len = hdr_len + hdr->srv_name_len + hdr->body_len;
        if (pkg_len + 4 > len) return 0;  // check_num (4B) not yet received

        // CRC32 over header + srv_name + body
        uint32_t expected = simple_crc32(data, pkg_len);
        uint32_t actual;
        memcpy(&actual, reinterpret_cast<const uint8_t*>(data) + pkg_len, 4);
        if (expected != actual) return -1;

        return static_cast<int>(pkg_len);
    }

public:
    // --- Length check (used by MessageHandler to determine if full packet is in buffer) ---

    // Returns: >0 = packet length to consume (including check_num), 0 = need more data, -1 = error
    static int check(const void* data, size_t len)
    {
        int r = validate(data, len);
        return (r > 0) ? r + 4 : (r == 0) ? 0 : -1;
    }

    // Legacy alias - same as check()
    static int Decode(const void* data, size_t len) { return check(data, len); }

    // --- Data extraction ---

    // Extract service name and body; returns request_id
    static uint64_t decode(const void* data, std::string& srvName, std::string& body)
    {
        const auto* hdr = reinterpret_cast<const ProtocolHeader*>(data);
        const uint8_t* p = reinterpret_cast<const uint8_t*>(data) + hdr_len;

        if (hdr->srv_name_len) {
            srvName.assign(reinterpret_cast<const char*>(p), hdr->srv_name_len);
            p += hdr->srv_name_len;
        }

        if (hdr->body_len) {
            body.assign(reinterpret_cast<const char*>(p), hdr->body_len);
        }

        return hdr->request_id;
    }

    // Legacy alias
    static uint64_t Decode(const void* data, std::string& srvName, std::string& body)
    { return decode(data, srvName, body); }

    // Extract with response code
    static uint64_t Decode(const void* data, std::string& srvName, std::string& body, uint8_t& code)
    {
        const auto* hdr = reinterpret_cast<const ProtocolHeader*>(data);
        code = hdr->code;
        return decode(data, srvName, body);
    }

    // Decode into Response struct; returns request_id
    static int Decode(const void* data, Response& resp)
    {
        std::string srvName;
        std::string body;
        uint8_t code;
        uint64_t rid = Decode(data, srvName, body, code);
        resp.state = code;
        resp.data  = std::move(body);
        return static_cast<int>(rid);
    }
};

} // namespace minirpc
