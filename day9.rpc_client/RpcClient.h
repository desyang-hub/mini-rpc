#pragma once

#include "Encoder.h"
#include "Decoder.h"
#include "Serialize.h"
#include "RpcServerDemo.h"

#include <iostream>
#include <tuple>
#include <vector>

// rpc client 用于为rpc调用的实现，ServiceCli调用函数的底层代理就会由RpcClient来实现

/**
 * @brief 暂时就不加通信了，直接就是模拟，将函数名和参数包(tuple)传入，接着就会开始封包，发送
 * 
 * 
 */

class RpcClient
{

private:
    
    template<class R>
    bool send(const std::vector<uint8_t>& bytes, R& ret) {
        // 模拟发送消息
        auto bytes_recv = RpcServerDemo::RecvMsg(bytes);


        ProtocolHeader header;
        std::string res;
        Decoder::Decode(bytes_recv, header, res);

        if (header.request_id != 200) {
            return false;
        }

        try
        {
            ret = Serialize::Deserialization<R>(res);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            return false;
        }
        
        return true;
    }

    bool send(const std::vector<uint8_t>& bytes) {
        // 模拟发送消息
        auto bytes_recv = RpcServerDemo::RecvMsg(bytes);

        ProtocolHeader header;
        std::string res;
        Decoder::Decode(bytes_recv, header, res);

        if (header.request_id != 200) {
            return false;
        }
        
        return true;
    }
public:
    template<class R, typename ...Args>
    bool call(const std::string& srvName, const std::tuple<Args...>& args, R& ret) {
        std::string body = Serialize::Serialization(args);
        auto bytes = Encoder::Encode(srvName, body);

        if (!send(bytes, ret)) return false;

        return true;
    }

    template<typename ...Args>
    bool call(const std::string& srvName, const std::tuple<Args...>& args) {
        std::string body = Serialize::Serialization(args);
        auto bytes = Encoder::Encode(srvName, body);

        if (!send(bytes)) return false;

        return true;
    }

};
