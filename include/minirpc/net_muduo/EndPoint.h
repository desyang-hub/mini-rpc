/**
 * @FilePath     : /mini-rpc/include/minirpc/net_muduo/EndPoint.h
 * @Description  :  
 * @Author       : desyang
 * @Date         : 2026-06-10 11:48:37
 * @LastEditors  : desyang
 * @LastEditTime : 2026-06-10 14:45:37
**/
#pragma once

#include <string>

struct EndPoint
{
    int port;
    std::string host;

    bool operator==(const EndPoint& rhs) const {
        return port == rhs.port && host == rhs.host;
    }

    EndPoint(const std::string& host, int port) : port(port), host(host) {}
};

// 🔑 必须实现：特化 std::hash
namespace std {
    template<>
    struct hash<EndPoint> {
        size_t operator()(const EndPoint& ep) const {
            // 组合 host 的哈希值和 port
            size_t h1 = std::hash<std::string>{}(ep.host);
            size_t h2 = std::hash<uint16_t>{}(ep.port);
            
            // 经典的哈希组合算法（boost::hash_combine 的简化版）
            // 避免简单异或导致的对称性碰撞 (a,b) == (b,a)
            return h1 ^ (h2 << 16) ^ (h2 >> 16);
        }
    };
}
