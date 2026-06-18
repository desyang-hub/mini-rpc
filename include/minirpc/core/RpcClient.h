#pragma once

#include <atomic>
#include <cstdint>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "minirpc/common/Response.h"
#include "minirpc/common/RpcException.h"
#include "minirpc/common/function_traits.h"
#include "minirpc/common/utils.h"
#include "minirpc/core/PendingRequest.h"
#include "minirpc/core/ServiceInstanceCache.h"
#include "minirpc/core/macro/rpc_service_stub.h"
#include "minirpc/net/ConnectionManager.h"
#include "minirpc/net/TcpClient.h"
#include "minirpc/protocol/Encoder.h"
#include "minirpc/protocol/Serialize.h"

#include <muduo/net/Buffer.h>
#include <muduo/net/TcpClient.h>

namespace minirpc
{

class RpcClient
{
public:
    RpcClient();
    ~RpcClient();

    // Initialize with Nacos registry address (call once before any invocation)
    void init(const std::string& nacosAddr);

    static RpcClient& GetInstance();

    // Synchronous RPC call - the main entry point for stub macros
    template<class R, class... Args>
    R Call(const char* serviceName, const char* method, Args&&... args);

private:
    std::future<Response> asyncInvoke(const char* name, const Bytes& bytes, uint64_t id);

    template<class R>
    R invoke(const char* name, const Bytes& bytes, uint64_t id);

    void MessageHandler(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf);

    mutable std::mutex mutex_;
    std::atomic<uint64_t> id_{0};
    std::unordered_map<uint64_t, PendingRequest> promises_;
    ConnectionManager connMgr_;
    std::unique_ptr<ServiceInstanceCache> serviceCache_;
};

// --- Template implementations ---

template<class R, class... Args>
R RpcClient::Call(const char* /*serviceName*/, const char* method, Args&&... args)
{
    uint64_t requestId = id_.fetch_add(1, std::memory_order_relaxed);

    Bytes bytes;
    if constexpr (sizeof...(Args) == 0) {
        bytes = Encoder::encodeRequest(method, "", requestId);
    } else if constexpr (sizeof...(Args) == 1) {
        std::string body = Serialize::Serialization(args...);
        bytes = Encoder::encodeRequest(method, body, requestId);
    } else {
        auto tuple = std::make_tuple(std::forward<Args>(args)...);
        std::string body = Serialize::Serialization(tuple);
        bytes = Encoder::encodeRequest(method, body, requestId);
    }

    return invoke<R>(method, bytes, requestId);
}

template<class R>
R RpcClient::invoke(const char* name, const Bytes& bytes, uint64_t id)
{
    auto fut = asyncInvoke(name, bytes, id);
    Response res = get_with_timeout<Response>(fut, std::chrono::milliseconds(200));
    if (res.state != SUCCESS) {
        throw RpcException(res.data);
    }
    return Serialize::Deserialization<R>(res.data.c_str(), res.data.size());
}

inline std::future<Response> RpcClient::asyncInvoke(const char* name, const Bytes& bytes, uint64_t id)
{
    if (!serviceCache_) {
        throw RpcException("RpcClient not initialized. Call RpcClient::init() first.");
    }
    serviceCache_->subscribeService(name);

    auto instances = serviceCache_->getInstances(name);
    if (instances.empty()) {
        LOG_ERROR("No instances for service: %s, using fallback", name);
        nacos::Instance fallback;
        fallback.ip = "127.0.0.1";
        fallback.port = 8083;
        instances.push_back(std::move(fallback));
    }

    std::vector<EndPoint> eps;
    eps.reserve(instances.size());
    for (auto& inst : instances) {
        eps.emplace_back(inst.ip, inst.port);
    }

    TcpClientPtr client = connMgr_.getConnection(eps);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        promises_[id] = PendingRequest{client, std::promise<Response>()};
    }
    auto fut = promises_[id].promise.get_future();

    client->sendRequest(bytes.data(), bytes.size());
    return fut;
}

} // namespace minirpc
