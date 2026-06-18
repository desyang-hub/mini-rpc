/**
 * @FilePath     : /mini-rpc/src/minirpc/core/RpcClient.cc
 * @Description  : RpcClient 实现 — 使用 Nacos subscribe 模式替代 getAllInstances
 * @Author       : desyang
 * @Date         : 2026-06-08 15:18:23
 * @LastEditors  : desyang
 * @LastEditTime : 2026-06-18
 **/

#include "minirpc/core/RpcClient.h"
#include "minirpc/protocol/Encoder.h"
#include "minirpc/protocol/Decoder.h"
#include "minirpc/protocol/Serialize.h"
#include "minirpc/common/RpcException.h"
#include "minirpc/common/Random.h"
#include "minirpc/net/TcpClient.h"
#include "minirpc/net/ConnectionManager.h"
#include "minirpc/common/logger.h"

#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/Callbacks.h>
#include <memory>
#include <iostream>

namespace minirpc
{

RpcClient& RpcClient::GetInstance() {
    static RpcClient rpcClient;
    return rpcClient;
}

// 构造函数
RpcClient::RpcClient() : id_(0), connMgr_() {
    connMgr_.setMessageCallback(
        [this](const muduo::net::TcpConnectionPtr& conn,
               muduo::net::Buffer* buf,
               muduo::Timestamp ts) {
            this->MessageHandler(conn, buf, ts);
        });
}

// 析构函数
RpcClient::~RpcClient() = default;

void RpcClient::init(const std::string& nacosAddr) {
    serviceCache_ = std::make_unique<ServiceInstanceCache>(nacosAddr);
}

// 消息回调函数 — 由 muduo IO 线程调用
void RpcClient::MessageHandler(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp t) {
    // 1. 尝试接收完整的 package
    // 2. Decode package 成为 srvName, paramBody
    // 3. promise::set_value

    while (buf->readableBytes()) {  // 一次可能有多个包
        int pkg_len = Decoder::Decode(buf->peek(), buf->readableBytes());

        // 出异常了，应该退出
        if (pkg_len == ERR) {
            throw RpcException("recv pkg msg exception");
        } else if (pkg_len == UN_FINISH) {
            return;
        } else { // 接收到完整的数据了
            // 调用函数并发送结果
            Response resp;
            int id = Decoder::Decode(buf->peek(), resp);

            // LOG_INFO("rid: %d", id);
            buf->retrieve(pkg_len);


            std::unique_lock<std::mutex> lock(mutex_);
            if (promises_.count(id) == 0) {
                throw RpcException("promise id not exists. ");
            }
            auto p = std::move(promises_[id]);
            promises_.erase(id);
            lock.unlock();

            p.promise.set_value(std::move(resp));
            p.conn->recovery();

        }
    }
}

} // namespace minirpc
