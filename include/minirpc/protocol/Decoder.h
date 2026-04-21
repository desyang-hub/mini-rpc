#pragma once

#include "minirpc/protocol/Protocol.h"
#include "minirpc/common/utils.h"
#include "minirpc/common/Type.h"

#include <vector>
#include <string.h>

namespace minirpc
{

enum DecodeState : int8_t {
    ERR = -1,
    UN_FINISH = 0,
    FINISHED = 1
};

class Decoder
{
private:

public:
    // template<class T>
    static int Decode(const Bytes& raw_data, ProtocolHeader& header, std::string& srvName, std::string& body) {
        int header_len = sizeof(header);

        // 1. 长度不够
        if (raw_data.size() < header_len) {
            return UN_FINISH;
        }

        // 2. 零拷贝头部解析
        const ProtocolHeader* headerPtr = reinterpret_cast<const ProtocolHeader*>(raw_data.data());

        // 3. 校验魔数
        if (headerPtr->magic != MAGIC_NUMBER) {
            std::cerr << "magic check error" << std::endl;
            return ERR;
        }

        // 4. body 未完全接入
        if (raw_data.size() < header_len + headerPtr->srv_name_len + headerPtr->body_len) {
            return UN_FINISH;
        }

        // 5. 校验
        if (simple_crc32(raw_data.data() + header_len + headerPtr->srv_name_len, headerPtr->body_len) != headerPtr->checksum) {
            std::cerr << "crc32 check faied" << std::endl;
            return ERR;
        }

        // 6. 拷贝service name
        srvName = std::string(reinterpret_cast<const char*>(raw_data.data() + header_len), headerPtr->srv_name_len);

        // 7. 拷贝body
        header = *headerPtr;
        body = std::string(reinterpret_cast<const char*>(raw_data.data() + header_len + header.srv_name_len), header.body_len);

        return FINISHED;
    }


    static bool Decode(std::vector<uint8_t> raw_data, ProtocolHeader& header, std::string& body) {
        int header_len = sizeof(header);

        // 1. 长度不够
        if (raw_data.size() < header_len) {
            return false;
        }

        // 2. 零拷贝头部解析
        const ProtocolHeader* headerPtr = reinterpret_cast<const ProtocolHeader*>(raw_data.data());

        // 3. 校验魔数
        if (headerPtr->magic != MAGIC_NUMBER) {
            std::cerr << "magic check error" << std::endl;
            return false;
        }

        // 4. body 未完全接入
        if (raw_data.size() < header_len + header.srv_name_len + header.body_len) {
            return false;
        }

        // 5. 校验
        if (simple_crc32(raw_data.data() + header_len, headerPtr->body_len) != headerPtr->checksum) {
            std::cerr << "crc32 check faied" << std::endl;
            return false;
        }

        // 6. 拷贝service name
        // srvName = std::string(reinterpret_cast<const char*>(raw_data.data() + header_len), header.srv_name_len);

        // 7. 拷贝body
        header = *headerPtr;
        body = std::string(reinterpret_cast<const char*>(raw_data.data() + header_len + header.srv_name_len), header.body_len);

        return true;
    }


    static int Check(const Bytes& raw_data) {
        int header_len = sizeof(ProtocolHeader);

        // 1. 长度不够
        if (raw_data.size() < header_len) {
            return UN_FINISH;
        }

        // 2. 零拷贝头部解析
        const ProtocolHeader* headerPtr = reinterpret_cast<const ProtocolHeader*>(raw_data.data());

        // 3. 校验魔数
        if (headerPtr->magic != MAGIC_NUMBER) {
            std::cerr << "magic check error" << std::endl;
            return ERR;
        }

        // 4. body 未完全接入
        if (raw_data.size() < header_len + headerPtr->srv_name_len + headerPtr->body_len) {
            return UN_FINISH;
        }

        // 5. 校验
        if (simple_crc32(reinterpret_cast<const uint8_t*>(raw_data.data()) + header_len + headerPtr->srv_name_len, headerPtr->body_len) != headerPtr->checksum) {
            std::cerr << "crc32 check faied" << std::endl;
            return ERR;
        }

        return header_len + headerPtr->srv_name_len + headerPtr->body_len;
    }
};

} // namespace minirpc