#pragma once

#include "minirpc/common/logger.h"
#include "minirpc/common/Buffer.h"
#include "minirpc/protocol/Decoder.h"
#include "minirpc/protocol/Protocol.h"

#include <iostream>
#include <cstdint>   // 添加这行
#include <cstddef>   // 可选，提供 size_t
#include <unistd.h>
#include <vector>
#include <sys/socket.h>

namespace minirpc
{

class Conn
{
private:
    int fd_;
    RingBuffer buf_;
    int buf_used_len = 0;
    int pkg_len_ = -1;

    ProtocolHeader header_;

public:

    

    Conn(int fd) : fd_(fd) {}

    int fd() const {
        return fd_;
    }

    // 从管道中读取一次数据
    // -1 表示断开或异常，0表示接收结束，> 0 表示消息完整可读取
    int readMsg() {
        int errno_code;
        int byteNum = buf_.read_fd(fd_, &errno_code);

        // 读数据
        if (byteNum <= 0) {
            if (byteNum == 0) {
                // 正常断开
                return -1;
            }

            // 读完了，等待下一次触发
            if (errno_code == EAGAIN || errno_code == EWOULDBLOCK) {
                return 0;
            }

            // 发生了异常
            // perror("read error");
            LOG_ERROR("read error");
            return -1;
        }

        // 还没有计算head 判断包头是否存在
        if (pkg_len_ == -1 && buf_.readable_bytes() >= sizeof(ProtocolHeader)) {
            buf_.peek_data(&header_, sizeof(header_));
            pkg_len_ = header_.body_len + header_.srv_name_len + sizeof(header_);
        }

        return byteNum;

        // 如果包完整了，就读取数据
        // if (buf_.readable_bytes() >= pkg_len_) {
        //     return pkg_len_;
        // }

        // // 出错了出错了=======================================》》》》》
        // return 1;
    }

    bool decode(std::string& body, std::string& srv_name, ProtocolHeader& header) {
        if (pkg_len_ == -1 || buf_.readable_bytes() < pkg_len_) return false;
        
        std::vector<uint8_t> bytes(pkg_len_);
        
        buf_.get_package_data(bytes.data(), pkg_len_);

        bool is_success = Decoder::Decode(bytes, header, srv_name, body);
        // 标记为-1
        pkg_len_ = -1;

        return is_success;
    }

    // 发送信息
    bool sendMsg(const std::string& msg) {

        if (::send(fd_, msg.data(), msg.size(), 0) == -1) {
            return false;
        }

        return true;
    }
};


} // namespace minirpc
