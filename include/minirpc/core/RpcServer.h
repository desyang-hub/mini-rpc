#pragma once

#include <functional>
#include <memory>
#include <queue>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <muduo/net/Buffer.h>
#include <muduo/net/TcpConnection.h>

#include "minirpc/common/ThreadPool.h"
#include "minirpc/common/function_traits.h"
#include "minirpc/common/tuple_helper.h"
#include "minirpc/core/macro/rpc_service_bind.h"
#include "minirpc/protocol/Encoder.h"
#include "minirpc/protocol/Serialize.h"
#include "minirpc/net/TcpServer.h"

#include <Nacos.h>

namespace minirpc
{

class RpcServer
{
    using RequestHandler = std::function<void(const std::string&, std::string&)>;

public:
    RpcServer();
    ~RpcServer();

    void Start(int port = 8080, const char* name = "RpcServer", const char* nacosAddr = "127.0.0.1");
    void Stop();

    static RpcServer& GetInstance();

    // Register a service instance with Nacos
    void addServiceInstance(const char* name, const char* groupName = "DefaultGroup",
                            const char* clusterName = "DefaultCluster");

    // Invoke a registered handler
    bool Invock(const std::string& srvName, const std::string& req, std::string& resp);

    void ShowAllService();

    // Message callback (called by TcpServer)
    void MessageHandler(const muduo::net::TcpConnectionPtr& conn,
                        muduo::net::Buffer* buf,
                        muduo::Timestamp);

    // --- Template methods (implemented below) ---

    template<class R, class F, typename... Args>
    void RegisterService(const char* className, const char* name, F&& f);

private:
    void ServiceRegisterWorker();

    template<class R, class F, class Param>
    void call_and_serialize(F&& f, Param&& param, std::string& res);

    std::unordered_map<std::string, RequestHandler> handlers_;
    std::vector<std::string> service_names_;
    mutable std::shared_mutex mutex_;

    ThreadPool threadPool_;
    std::unique_ptr<TcpServer> tcpServer_;
    int port_ = 0;
    std::string nacos_addr_;

    std::thread registerWorker_;
    std::queue<nacos::Instance> instances_;
    bool is_close_ = false;
    mutable std::mutex instance_mutex_;
    std::condition_variable condition_;

    mutable std::mutex close_mutex_;
    std::condition_variable close_condition_;
};

// --- Template implementations ---

template<class R, class F, class Param>
inline void RpcServer::call_and_serialize(F&& f, Param&& param, std::string& res)
{
    if constexpr (std::is_void_v<R>) {
        std::forward<F>(f)(std::forward<Param>(param));
    } else {
        auto result = std::forward<F>(f)(std::forward<Param>(param));
        res = Serialize::Serialization(result);
    }
}

template<class R, class F, typename... Args>
inline void RpcServer::RegisterService(const char* className, const char* name, F&& f)
{
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        service_names_.emplace_back(className, strlen(className));
        handlers_[name] = [func = std::forward<F>(f), this](const std::string& req, std::string& resp) {
            using trait = function_traits<F>;

            if constexpr (trait::is_single_arg) {
                using arg_type = typename trait::first_arg;
                arg_type param = Serialize::Deserialization<arg_type>(req);
                call_and_serialize<R>(func, param, resp);
            } else {
                using args_tuple = typename trait::args_tuple;
                args_tuple param = Serialize::Deserialization<args_tuple>(req);
                call_and_serialize<R>(func, param, resp);
            }
        };
    }
}

template<class Class, typename MethodPtr>
inline void bind_rpc_method_impl(const char* className, const char* name, MethodPtr methodPtr)
{
    using type_trait = function_traits<MethodPtr>;
    using return_type = typename type_trait::return_type;

    LOG_INFO("Method register: %s", name);

    if constexpr (type_trait::is_single_arg) {
        using arg_type = typename type_trait::first_arg;
        RpcServer::GetInstance().RegisterService<return_type>(className, name,
            [methodPtr](arg_type arg) -> auto {
                if constexpr (std::is_void_v<return_type>) {
                    (Class::GetInstance().*methodPtr)(arg);
                } else {
                    return (Class::GetInstance().*methodPtr)(arg);
                }
            });
    } else {
        using args_tuple = typename type_trait::args_tuple;
        RpcServer::GetInstance().RegisterService<return_type>(className, name,
            [methodPtr](args_tuple args) -> auto {
                if constexpr (std::is_void_v<return_type>) {
                    rpc_apply([methodPtr](auto&&... a) {
                        (Class::GetInstance().*methodPtr)(std::forward<decltype(a)>(a)...);
                    }, args);
                } else {
                    return rpc_apply([methodPtr](auto&&... a) {
                        return (Class::GetInstance().*methodPtr)(std::forward<decltype(a)>(a)...);
                    }, args);
                }
            });
    }
}

} // namespace minirpc
