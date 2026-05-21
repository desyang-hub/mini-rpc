#pragma once

#include "minirpc/common/Buffer.h"
#include "minirpc/common/logger.h"
#include "minirpc/protocol/Decoder.h"
#include "minirpc/protocol/Protocol.h"

#include <vector>
#include <string>
#include <cstring>

namespace minirpc
{

// 共享的缓冲区读取和解码逻辑，供Conn和RpcConnection复用
class BufferedConnection {
private:
    RingBuffer buf_;
    int pkg_len_ = -1;
    ProtocolHeader header_;

public:
    BufferedConnection() = default;

    // 从fd读取一次数据
    // -1 表示断开或异常，0表示接收结束(EAGAIN)，> 0 表示读取了数据
    int readMsg(int fd) {
        int errno_code;
        int byteNum = buf_.read_fd(fd, &errno_code);

        if (byteNum <= 0) {
            if (byteNum == 0) {
                return -1; // 正常断开
            }
            if (errno_code == EAGAIN || errno_code == EWOULDBLOCK) {
                return 0; // 读完了，等待下一次触发
            }
            LOG_ERROR("read error (%s)", strerror(errno_code));
            return -1;
        }

        // 判断包头是否存在
        if (pkg_len_ == -1 && buf_.readable_bytes() >= sizeof(ProtocolHeader)) {
            buf_.peek_data(&header_, sizeof(header_));
            pkg_len_ = header_.body_len + header_.srv_name_len + sizeof(header_);
        }

        return byteNum;
    }

    // 解码完整包（包含service name和body）
    bool decode(std::string& body, std::string& srv_name, ProtocolHeader& header) {
        if (pkg_len_ == -1 || buf_.readable_bytes() < pkg_len_) return false;

        std::vector<uint8_t> bytes(pkg_len_);
        buf_.get_package_data(bytes.data(), pkg_len_);

        bool is_success = Decoder::Decode(bytes, header, srv_name, body);
        pkg_len_ = -1;
        return is_success;
    }

    // 解码包（仅body，不含service name）
    bool decode(ProtocolHeader& header, std::string& body) {
        if (pkg_len_ == -1 || buf_.readable_bytes() < pkg_len_) return false;

        std::vector<uint8_t> bytes(pkg_len_);
        buf_.get_package_data(bytes.data(), pkg_len_);

        bool is_success = Decoder::Decode(bytes, header, body);
        pkg_len_ = -1;
        return is_success;
    }
};

} // namespace minirpc
