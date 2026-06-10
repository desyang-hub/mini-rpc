/**
 * @FilePath     : /mini-rpc/include/minirpc/core/RpcClient.h
 * @Description  : RPC Client 类定义文件
 * @Author       : desyang
 * @Date         : 2026-06-08 15:18:23
 * @LastEditors  : desyang
 * @LastEditTime : 2026-06-10 17:39:49
**/
#pragma once

#include <mutex>
#include <string>
#include <future>
#include <cstdint>
#include <unordered_map>
#include <unistd.h>
#include <queue>
#include <thread>

#include "minirpc/protocol/Encoder.h"
#include "minirpc/protocol/Decoder.h"
#include "minirpc/protocol/Serialize.h"
#include "minirpc/common/Response.h"
#include "minirpc/common/RpcException.h"
#include "minirpc/common/function_traits.h"
#include "minirpc/core/macro/rpc_service_stub.h"
#include "minirpc/net/TcpClient.h"
#include "minirpc/net/ConnectionManager.h"

#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/EventLoop.h>
#include <atomic>
#include <Nacos.h>

// rpc client 需要有哪些功能
// 1. 通过一个宏用于服务函数，这个服务函数无需实现，只需要声明即可
// 2. 用户通过代理类调用服务函数，过程中，将函数名和参数进行序列化成字符串，并打包进行发送
// 3. 为函数预留一个 future<R> 用于接收返回值，并将对应的 promise，绑定到 unordered_map 中，用户端调用 future<R>::get() 阻塞，直到服务端返回结果，并将结果设置到 promise 中

namespace minirpc
{

class RpcClient
{
public:
    // this 可能会导致异常，如果有条件，尽量换成 shared_ptr
    RpcClient();

    ~RpcClient();

    std::list<nacos::Instance> getAllInstances(const std::string& name);

    // 获取单实例
    static RpcClient& GetInstance();

    // 代理函数通过方法名和序列化结果作为参数，来调用 RpcClient 的 Invock 方法，
    template<class R>
    std::future<R> AsyncInvoke(const char* name, const Bytes& bytes);

    template<class R>
    R Invoke(const char* name, const Bytes& bytes);

    template<class R, class ...Args>
    R Call(const char* serviceName, const char* name, Args&& ...args);

private:
    mutable std::mutex mutex_;
    uint64_t id_;
    // promise
    std::unordered_map<uint64_t, std::promise<Response>> promises_;

    ConnectionManager connMgr_;

    using ServiceSearchHandler = std::function<void(nacos::NamingService *)>;
    std::queue<ServiceSearchHandler> workers_;
    std::mutex wokers_mutex_;
    std::condition_variable condition_;
    std::thread searchServiceWorker_;
    bool is_close;

    // 搜索服务后台进程
    void ServiceSearchWorker();

    // 消息回调函数
    void MessageHandler(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp t);
};

// ==================== 模板函数实现 ====================

template<class R, class ...Args>
inline R RpcClient::Call(const char* serviceName, const char* name, Args&& ...args) {
    Bytes bytes;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++id_;
        if constexpr (sizeof...(Args) == 0) {
            // bytes = Encoder::Encode(name, nullptr, 0);
            bytes = Encoder::EncodeReq(id_, name, nullptr, 0);
        }
        // 单参函数
        else if constexpr (sizeof...(Args) == 1) {
            // bytes = Encoder::Encode(name, Serialize::Serialization(args...));
            std::string body = Serialize::Serialization(args...);
            bytes = Encoder::EncodeReq(id_, name, body.c_str(), body.size());
        } else {
            auto args_tuple = std::make_tuple(std::forward<Args>(args)...);
            std::string body = Serialize::Serialization(args_tuple);
            bytes = Encoder::EncodeReq(id_, name, body.c_str(), body.size());
        }
    }

    return Invoke<R>(serviceName, bytes);
}

template<class R>
inline std::future<R> RpcClient::AsyncInvoke(const char* name, const Bytes& bytes) {
    std::future<Response> f;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        promises_[id_] = std::promise<Response>();
        f = promises_[id_].get_future();
    }

    // 获取可用实例
    std::list<nacos::Instance> instances = getAllInstances(name);
    std::vector<EndPoint> eps;
    eps.reserve(instances.size());

    for (auto it = instances.begin(); it != instances.end(); ++it) {
        LOG_INFO("valid Instance: %s:%d", it->ip.c_str(), it->port);
        eps.emplace_back(it->ip, it->port);
    }

    TcpClientPtr tcpClientPtr = connMgr_.getConnection(eps);
    tcpClientPtr->sendRequest(bytes.data(), bytes.size());

    // 将 f->get 封装成一个异步任务
    std::future<R> fut = std::async(std::launch::async, [f = std::move(f)]() mutable {
        Response res = f.get();
        if (res.state != SUCCESS) {
            // 默认如果失败的话 res.data 就装异常就好了
            throw RpcException(res.data);
        }

        return Serialize::Deserialization<R>(res.data.c_str(), res.data.size());
    });

    return fut;
}

template<class R>
inline R RpcClient::Invoke(const char* name, const Bytes& bytes) {
    return AsyncInvoke<R>(name, bytes).get();
}

} // namespace minirpc
