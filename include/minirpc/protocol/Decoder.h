#pragma once

#include "minirpc/protocol/Protocol.h"
#include "minirpc/common/utils.h"
#include "minirpc/common/Type.h"

#include <vector>
#include <cstring>
#include <cstdint>
#include <cstddef>
#include <iostream>

namespace minirpc
{

constexpr uint32_t MAX_BODY_SIZE = 64 * 1024 * 1024; // 64MB

enum DecodeState : int8_t {
    ERR = -1,
    UN_FINISH = 0,
    FINISHED = 1
};

class Decoder
{
private:
    // 共享的头部校验逻辑，返回已验证的头部指针，失败返回nullptr
    static const ProtocolHeader* validateHeader(const Bytes& raw_data, int& error_code) {
        constexpr int header_len = sizeof(ProtocolHeader);

        // 1. 长度不够
        if (raw_data.size() < header_len) {
            error_code = UN_FINISH;
            return nullptr;
        }

        // 2. 零拷贝头部解析
        const ProtocolHeader* headerPtr = reinterpret_cast<const ProtocolHeader*>(raw_data.data());

        // 3. 校验魔数
        if (headerPtr->magic != MAGIC_NUMBER) {
            std::cerr << "magic check error" << std::endl;
            error_code = ERR;
            return nullptr;
        }

        // 3.5 限制最大包体大小，防止内存溢出
        if (headerPtr->body_len > MAX_BODY_SIZE) {
            std::cerr << "body too large: " << headerPtr->body_len << std::endl;
            error_code = ERR;
            return nullptr;
        }

        // 4. body 未完全接入
        if (raw_data.size() < header_len + headerPtr->srv_name_len + headerPtr->body_len) {
            error_code = UN_FINISH;
            return nullptr;
        }

        // 5. 校验CRC32（覆盖srv_name + body）
        if (simple_crc32(raw_data.data() + header_len, headerPtr->srv_name_len + headerPtr->body_len) != headerPtr->checksum) {
            std::cerr << "crc32 check failed" << std::endl;
            error_code = ERR;
            return nullptr;
        }

        error_code = 0;
        return headerPtr;
    }

public:
    // 解码完整包（包含service name和body）
    static int Decode(const Bytes& raw_data, ProtocolHeader& header, std::string& srvName, std::string& body) {
        int error_code;
        const ProtocolHeader* headerPtr = validateHeader(raw_data, error_code);
        if (!headerPtr) return error_code;

        constexpr int header_len = sizeof(ProtocolHeader);

        // 拷贝service name
        srvName = std::string(reinterpret_cast<const char*>(raw_data.data() + header_len), headerPtr->srv_name_len);

        // 拷贝body
        header = *headerPtr;
        body = std::string(reinterpret_cast<const char*>(raw_data.data() + header_len + headerPtr->srv_name_len), headerPtr->body_len);

        return FINISHED;
    }

    // 解码包（仅body，不含service name）
    static bool Decode(const Bytes& raw_data, ProtocolHeader& header, std::string& body) {
        int error_code;
        const ProtocolHeader* headerPtr = validateHeader(raw_data, error_code);
        if (!headerPtr) return error_code == UN_FINISH ? false : false;

        constexpr int header_len = sizeof(ProtocolHeader);

        // 拷贝body
        header = *headerPtr;
        body = std::string(reinterpret_cast<const char*>(raw_data.data() + header_len + headerPtr->srv_name_len), headerPtr->body_len);

        return true;
    }

    // 仅校验包完整性，返回包总长度或错误码
    static int Check(const Bytes& raw_data) {
        int error_code;
        const ProtocolHeader* headerPtr = validateHeader(raw_data, error_code);
        if (!headerPtr) return error_code;

        constexpr int header_len = sizeof(ProtocolHeader);
        return header_len + headerPtr->srv_name_len + headerPtr->body_len;
    }


    /// @brief 对数据进行解码
    /// @param data 数据首地址
    /// @param len 数据长度
    /// @return -1 | 0 | > 0 => error, unfinish, accept
    static int Decode(void* data, int len) {
        int header_len = sizeof(ProtocolHeader);
        // 头长度是否足够
        if (len < header_len) {
            return UN_FINISH;
        }

        // 头部解析
        ProtocolHeader* headerPtr = reinterpret_cast<ProtocolHeader*>(data);

        // 检验魔数
        if (headerPtr->magic != MAGIC_NUMBER) {
            // 魔数校验不匹配
            return ERR;
        }

        // 计算包长度
        int pkg_len = header_len + headerPtr->srv_name_len + headerPtr->body_len;

        // 整个数据包完整性验证
        if (pkg_len + 4 > len) { // 末尾四字节包含check_num
            return UN_FINISH;
        }

        // CRC整个数据包校验
        if (simple_crc32(data, pkg_len) != *(reinterpret_cast<uint32_t*>((char*)data + pkg_len))) {
            // crc数据校验不通过，数据出错，这个应该要触发重传才对
            return ERR;
        }

        return pkg_len + 4;
    }
};

} // namespace minirpc
