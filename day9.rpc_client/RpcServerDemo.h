#pragma once

#include <iostream>
#include <vector>

#include "protocol.h"
#include "Decoder.h"
#include "RpcServer.h"

class RpcServerDemo
{
public:
    static std::vector<uint8_t> RecvMsg(const std::vector<uint8_t>& bytes) {
        // 远程解析数据
        ProtocolHeader header;
        std::string srvName;
        std::string body;
        bool is_success = Decoder::Decode(bytes, header, srvName, body);

        std::string res;
        if  (is_success) {
            is_success = RpcServer::call(srvName, body, res);
        }

        if (!is_success) {
            header.request_id = 300;
        }

        header.magic = MAGIC_NUMBER;
        header.srv_name_len = 0;

        return Encoder::Encode(header, res, MSG_RESPONSE);
    }
};
