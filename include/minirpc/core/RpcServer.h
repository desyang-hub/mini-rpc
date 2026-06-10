/**
 * @FilePath     : /mini-rpc/include/minirpc/core/RpcServer.h
 * @Description  : RPC Server 类定义文件
 * @Author       : desyang
 * @Date         : 2026-06-08 16:19:14
 * @LastEditors  : desyang
 * @LastEditTime : 2026-06-10 17:21:30
**/
#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <functional>
#include <shared_mutex>
#include <queue>
#include <thread>

#include <muduo/net/TcpConnection.h>
#include <muduo/net/Buffer.h>
#include <muduo/base/Timestamp.h>

#include "minirpc/common/function_traits.h"
#include "minirpc/common/tuple_helper.h"
#include "minirpc/common/logger.h"
#include "minirpc/common/RpcException.h"
#include "minirpc/protocol/Serialize.h"
#include "minirpc/core/macro/rpc_service_bind.h"
#include "minirpc/protocol/Decoder.h"
#include "minirpc/protocol/Encoder.h"
#include "minirpc/net/TcpServer.h"

#include <memory>
// #include "Nacos.h"
#include "nacos/Nacos.h"

namespace minirpc
{

// 需要为 RpcServer 提供一个后台常驻程序，常驻程序是一个循环，只有当程序结束才停止，循环内的任务就是将<待注册服务注册到服务中心>

class RpcServer
{
    using RequestHandler = std::function<void(const std::string&, std::string&)>;
private:
    std::unordered_map<std::string, RequestHandler> handlers_;
    std::vector<std::string> service_names_;
    mutable std::shared_mutex mutex_;
    std::unique_ptr<TcpServer> tcpServer_;
    int port_;

    std::thread registerWorker_;
    std::queue<nacos::Instance> instances_;
    bool is_close_;
    mutable std::mutex instance_mutex_;
    std::condition_variable condition_;

    // 此处实现注册逻辑
    void ServiceRegisterWorker();

    template<class R, class F, class Param>
    void call_and_serialize(F&& f, Param&& param, std::string& res);

public:
    RpcServer();

    ~RpcServer();

    void Start(int port = 8080, const char* name = "TcpServer");

    static RpcServer& GetInstance();

    void addServiceInstance(const char* name, const char* groupName = "DefaultGroup", const char* clusterName = "DefaultCluster");

    template<class R, class F, typename ...Args>
    void RegisterService(const char* classNmae, const char* name, F&& f);

    /// @brief 调用 rpc service
    /// @param srvName 服务名
    /// @param req 请求参数包，未序列化
    /// @param resp 返回序列化后的结果
    /// return
    bool Invock(const std::string& srvName, const std::string& req, std::string& resp);

    void ShowAllService();

    void Handler(const muduo::net::TcpConnectionPtr& conn,
        muduo::net::Buffer* buf,
        muduo::Timestamp t);
};

// ==================== 模板函数实现 ====================

template<class R, class F, class Param>
inline void RpcServer::call_and_serialize(F&& f, Param&& param, std::string& res) {
    // 如果返回空类型，则如何
    if constexpr (std::is_void_v<R>) {
        std::forward<F>(f)(std::forward<Param>(param));
    } else { // 否则如何
        auto result = std::forward<F>(f)(std::forward<Param>(param));
        res = Serialize::Serialization(result);
    }
}

template<class R, class F, typename ...Args>
inline void RpcServer::RegisterService(const char* classNmae, const char* name, F&& f) {
    // 写锁
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        service_names_.emplace_back(classNmae, strlen(classNmae));
        handlers_[name] = [func = std::forward<F>(f), this](const std::string& req, std::string& resp){
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
inline void bind_rpc_method_impl(const char* className, const char* name, MethodPtr methodPtr) {
    using type_trait = function_traits<MethodPtr>;
    using return_type = typename type_trait::return_type;

    if constexpr (type_trait::is_single_arg) {
        using arg_type = typename type_trait::first_arg;

        RpcServer::GetInstance().RegisterService<return_type>(className, name, [methodPtr](arg_type arg) -> auto {
            if constexpr (std::is_void_v<return_type>) {
                (Class::GetInstance().*methodPtr)(arg);
            } else {
                return (Class::GetInstance().*methodPtr)(arg);
            }
        });

    } else {
        using args_typle = typename type_trait::args_tuple;
        RpcServer::GetInstance().RegisterService<return_type>(className, name, [methodPtr](args_typle args) -> auto {
            if constexpr (std::is_void_v<return_type>) {
                rpc_apply([methodPtr](auto&& ...args){
                    (Class::GetInstance().*methodPtr)(std::forward<decltype(args)>(args)...);
                }, args);
            } else {
                return rpc_apply([methodPtr](auto&& ...args){
                    return (Class::GetInstance().*methodPtr)(std::forward<decltype(args)>(args)...);
                }, args);
            }
        });
    }
}

} // namespace minirpc
