/**
 * @FilePath     : /mini-rpc/include/minirpc/core/RpcClient.h
 * @Description  : RPC Client 类定义文件
 * @Author       : desyang
 * @Date         : 2026-06-08 15:18:23
 * @LastEditors  : desyang
 * @LastEditTime : 2026-06-18 17:55:36
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
#include "minirpc/common/utils.h"
#include "minirpc/common/Response.h"
#include "minirpc/common/RpcException.h"
#include "minirpc/common/function_traits.h"
#include "minirpc/net/TcpClient.h"
#include "minirpc/net/ConnectionManager.h"
#include "minirpc/core/macro/rpc_service_stub.h"
#include "minirpc/core/PendingRequest.h"
#include "minirpc/common/ThreadPool.h"

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
    class ServiceInstanceListener;

    // this 可能会导致异常，如果有条件，尽量换成 shared_ptr
    RpcClient();

    ~RpcClient();

    std::list<nacos::Instance> getAllInstances(const std::string& name);

    // 获取单实例
    static RpcClient& GetInstance();

    // 代理函数通过方法名和序列化结果作为参数，来调用 RpcClient 的 Invock 方法，
    // template<class R>
    std::future<Response> AsyncInvoke(const char* name, const Bytes& bytes, uint64_t request_id);

    template<class R>
    R Invoke(const char* name, const Bytes& bytes, uint64_t request_id);

    template<class R, class ...Args>
    R Call(const char* serviceName, const char* name, Args&& ...args);

private:
    mutable std::mutex mutex_;
    std::atomic<uint64_t> id_;
    // uint64_t id_;
    // promise
    std::unordered_map<uint64_t, PendingRequest> promises_;

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
R RpcClient::Call(const char* serviceName, const char* name, Args&& ...args) {
    Bytes bytes;

    // int request_id;
    // {
    //     std::lock_guard<std::mutex> lock(mutex_);
    uint64_t request_id = id_.fetch_add(1, std::memory_order_relaxed);
    if constexpr (sizeof...(Args) == 0) {
        bytes = Encoder::EncodeReq(request_id, name, nullptr, 0);
    }
    // 单参函数
    else if constexpr (sizeof...(Args) == 1) {
        std::string body = Serialize::Serialization(args...);
        bytes = Encoder::EncodeReq(request_id, name, body.c_str(), body.size());
    } else {
        auto args_tuple = std::make_tuple(std::forward<Args>(args)...);
        std::string body = Serialize::Serialization(args_tuple);
        bytes = Encoder::EncodeReq(request_id, name, body.c_str(), body.size());
    }
    // }

    // std::cout << "send req id: " << request_id << std::endl;

    auto r = Invoke<R>(serviceName, bytes, request_id);

    return r;
}

inline std::future<Response> RpcClient::AsyncInvoke(const char* name, const Bytes& bytes, uint64_t request_id) {
    std::list<nacos::Instance> instances;
    // nacos::Instance instance;
    // instance.ip = "127.0.0.1";
    // instance.port = 8083;
    // instances.push_back(std::move(instance));


    // 获取可用实例
    try
    {
        instances = std::move(getAllInstances(name));
    }
    catch(const std::exception& e)
    {
        LOG_ERROR("getAllInstances err: %s", e.what());
        LOG_INFO("Switch to mock server");
        nacos::Instance instance;
        instance.ip = "127.0.0.1";
        instance.port = 8083;
        instances.push_back(std::move(instance));
    }
     
    std::vector<EndPoint> eps;
    eps.reserve(instances.size());

    for (auto it = instances.begin(); it != instances.end(); ++it) {
        // LOG_INFO("valid Instance: %s:%d", it->ip.c_str(), it->port);
        eps.emplace_back(it->ip, it->port);
    }

    TcpClientPtr tcpClientPtr = connMgr_.getConnection(eps);
    std::unique_lock<std::mutex> lock(mutex_);
    promises_[request_id] = PendingRequest{tcpClientPtr, std::promise<Response>()};
    std::future<Response> f = promises_[request_id].promise.get_future();
    lock.unlock();

    tcpClientPtr->sendRequest(bytes.data(), bytes.size());

    return f;

    // connMgr_.recovery(std::move(tcpClientPtr)); // 不能立马归还连接

    // 将 f->get 封装成一个异步任务
    // std::future<R> fut = std::async(std::launch::async, [f = std::move(f)]() mutable {
    //     Response res = f.get();
    //     if (res.state != SUCCESS) {
    //         // 默认如果失败的话 res.data 就装异常就好了
    //         throw RpcException(res.data);
    //     }

    //     return Serialize::Deserialization<R>(res.data.c_str(), res.data.size());
    // });

    // return fut;
}

template<class R>
R RpcClient::Invoke(const char* name, const Bytes& bytes, uint64_t request_id) {
    auto fut = AsyncInvoke(name, bytes, request_id);
    Response res = get_with_timeout(fut, std::chrono::milliseconds(200));
    // Response res = AsyncInvoke(name, bytes, request_id).get();
    if (res.state != SUCCESS) {
        // 默认如果失败的话 res.data 就装异常就好了
        throw RpcException(res.data);
    }

    return Serialize::Deserialization<R>(res.data.c_str(), res.data.size());
    // auto fut = AsyncInvoke<R>(name, bytes, request_id);
    // return fut.get();
    // return get_with_timeout<R>(fut, std::chrono::milliseconds(100));
}

} // namespace minirpc
