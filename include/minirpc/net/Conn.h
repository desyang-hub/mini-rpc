#pragma once

#include "minirpc/net/BufferedConnection.h"

#include <string>
#include <sys/socket.h>

namespace minirpc
{

class Conn
{
private:
    int fd_;
    BufferedConnection buf_conn_;

public:
    Conn(int fd) : fd_(fd) {}

    int fd() const {
        return fd_;
    }

    int readMsg() {
        return buf_conn_.readMsg(fd_);
    }

    bool decode(std::string& body, std::string& srv_name, ProtocolHeader& header) {
        return buf_conn_.decode(body, srv_name, header);
    }

    bool sendMsg(const std::string& msg) {
        if (::send(fd_, msg.data(), msg.size(), 0) == -1) {
            return false;
        }
        return true;
    }
};

} // namespace minirpc
