#include "minirpc/core/RpcClient.h"
#include "minirpc/protocol/Encoder.h"
#include "minirpc/protocol/Decoder.h"
#include "minirpc/protocol/Serialize.h"
#include "minirpc/common/RpcException.h"
#include "minirpc/common/Random.h"
#include "minirpc/common/logger.h"
#include "minirpc/net/ConnectionManager.h"

#include <muduo/net/Buffer.h>

namespace minirpc
{

RpcClient& RpcClient::GetInstance()
{
    static RpcClient instance;
    return instance;
}

RpcClient::RpcClient() : id_(0)
{
    connMgr_.setMessageCallback(
        [this](const muduo::net::TcpConnectionPtr& conn,
               muduo::net::Buffer* buf,
               muduo::Timestamp) {
            MessageHandler(conn, buf);
        });
}

RpcClient::~RpcClient() = default;

void RpcClient::init(const std::string& nacosAddr)
{
    serviceCache_ = std::make_unique<ServiceInstanceCache>(nacosAddr);
}

void RpcClient::MessageHandler(const muduo::net::TcpConnectionPtr&, muduo::net::Buffer* buf)
{
    while (buf->readableBytes()) {
        int pkg_len = Decoder::Decode(buf->peek(), buf->readableBytes());

        if (pkg_len == ERR) {
            throw RpcException("recv pkg msg exception");
        }
        if (pkg_len == UN_FINISH) {
            return;
        }

        Response resp;
        uint64_t rid = Decoder::Decode(buf->peek(), resp);
        buf->retrieve(pkg_len + 4);  // +4 for check_num

        PendingRequest req;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            auto it = promises_.find(rid);
            if (it == promises_.end()) {
                LOG_ERROR("promise id %lu not found", rid);
                return;
            }
            req = std::move(it->second);
            promises_.erase(it);
        }

        req.promise.set_value(std::move(resp));
        req.conn->recovery();
    }
}

} // namespace minirpc
