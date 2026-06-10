/**
 * @FilePath     : /mini-rpc/include/minirpc/rpc_core/RpcServer.h
 * @Description  :  
 * @Author       : desyang
 * @Date         : 2026-06-08 16:19:14
 * @LastEditors  : desyang
 * @LastEditTime : 2026-06-10 14:56:38
**/
#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <functional>
#include <shared_mutex>
#include <queue>

#include <muduo/net/TcpConnection.h>
#include <muduo/net/Buffer.h>
#include <muduo/base/Timestamp.h>

#include "minirpc/common/function_traits.h"
#include "minirpc/common/tuple_helper.h"
#include "minirpc/common/logger.h"
#include "minirpc/common/RpcException.h"
#include "minirpc/protocol/Serialize.h"
#include "minirpc/rpc_core/macro/rpc_service_bind.h"
#include "minirpc/protocol/Decoder.h"
#include "minirpc/protocol/Encoder.h"
#include "minirpc/net_muduo/TcpServer.h"
#include "minirpc/common/logger.h"

#include "Nacos.h"

#include <memory>

namespace minirpc
{

// 需要为RpcServer提供一个后台常驻程序，常驻程序是一个循环，只有当程序结束才停止，循环内的任务就是将<待注册服务注册到服务中心>
    
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
    void ServiceRegisterWorker() {
        // 启动注册服务
        nacos::Properties configProps;
        configProps[nacos::PropertyKeyConst::SERVER_ADDR] = "127.0.0.1"; // 注册中心地址，后续使用配置文件+域名来替换
        nacos::INacosServiceFactory *factory = nacos::NacosFactoryFactory::getNacosFactory(configProps);
        nacos::ResourceGuard<nacos::INacosServiceFactory> _guardFactory(factory);

        auto g_namingSvc = factory->CreateNamingService();
        nacos::ResourceGuard<nacos::NamingService> _serviceGuard(g_namingSvc);

        // 注册逻辑
        while (true) {
            // 等待实例并注册
            nacos::Instance instance;

            {
                std::unique_lock<std::mutex> lock(instance_mutex_);
                condition_.wait(lock, [this]{
                    return !instances_.empty() || is_close_;
                });

                // 如果程序已经退出，那么立即停止
                if (is_close_) {
                    break;
                } else {
                    instance = std::move(instances_.front());
                    instances_.pop();
                }
            }
            
            // 将实例注册到服务中心
            try {
                instance.port = port_;
                
                // std::string serviceName = instance.clusterName + "@" + instance.groupName + "::" + instance.serviceName;
                g_namingSvc->registerInstance(instance.serviceName, instance);
            } catch (nacos::NacosException &e) {
                LOG_INFO("Nacos registration failed: %s", e.what());
                // throw std::runtime_error(std::string("Nacos registration failed: ") + e.what());
            }
        }
    }

    template<class R, class F, class Param>
    void call_and_serialize(F&& f, Param&& param, std::string& res) {
        // 如果返回空类型，则如何
        if constexpr (std::is_void_v<R>) {
            std::forward<F>(f)(std::forward<Param>(param));
        } else { // 否则如何
            auto result = std::forward<F>(f)(std::forward<Param>(param));
            res = Serialize::Serialization(result);
        }
    }

public:
    RpcServer() : is_close_(false) {
    }

    ~RpcServer() {
        {
            std::lock_guard<std::mutex> lock(instance_mutex_);
            is_close_ = true;
        }
        condition_.notify_all();

        if (registerWorker_.joinable()) {
            registerWorker_.join();
        }
    }

    void Start(int port = 8080, const char* name = "TcpServer") {
        port_ = port;
        // 启用后台常驻注册程序
        registerWorker_ = std::thread(&RpcServer::ServiceRegisterWorker, this);

        tcpServer_ = std::make_unique<TcpServer>(port, name);
        tcpServer_->setMessageCallback(std::bind(&RpcServer::Handler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        tcpServer_->Start();
    }

    static RpcServer& GetInstance();

    void addServiceInstance(const char* name, const char* groupName = "DefaultGroup", const char* clusterName = "DefaultCluster") {
        nacos::Instance instance;
        instance.clusterName = clusterName;
        instance.groupName = groupName;
        instance.serviceName = name;
        instance.ip = "127.0.0.1"; // 暂定
        instance.ephemeral = true;

        {
            std::lock_guard<std::mutex> lock(instance_mutex_);
            instances_.emplace(std::move(instance));
        }
        condition_.notify_one();
    }

    template<class R, class F, typename ...Args>
    void RegisterService(const char* classNmae, const char* name, F&& f) {
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

    /// @brief 调用rpc service
    /// @param srvName 服务名
    /// @param req 请求参数包，未序列化
    /// @param resp 返回序列化后的结果
    /// return
    bool Invock(const std::string& srvName, const std::string& req, std::string& resp) {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        // handlers_[srvName](req, resp);
        try {
            handlers_.at(srvName)(req, resp); // .at() 会在 key 不存在时抛出 std::out_of_range
            return true;
        } catch (const std::exception& e) {
            resp = e.what();
            LOG_ERROR("RPC service not found: %s", srvName.c_str());
            // 可以在这里构造一个错误响应包发回给客户端
            // throw RpcException("Service not found: " + srvName);
            return false;
        }
    }

    void ShowAllService() {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        for (const auto& srv : service_names_) {
            LOG_INFO("server name: %s", srv.c_str());
        }
    }

    void Handler(const muduo::net::TcpConnectionPtr& conn,
        muduo::net::Buffer* buf,
        muduo::Timestamp t) { // 这是回调函数，当有请求消息从用户端发送过来
            // 1. 尝试接收完整的package
            // 2. Decode package 成为 srvName, paramBody
            // 3. Invock(srvName, paramBody)
            int pkg_len = Decoder::Decode(buf->peek(), buf->readableBytes());

            // 出异常了，应该退出
            if (pkg_len == ERR) {
                throw RpcException("recv pkg msg exception");
            } else if (pkg_len == UN_FINISH) {
                return;
            } else { // 接收到完整的数据了
                // 调用函数并发送结果

                std::string srvName;
                std::string body;
                std::string resp;

                uint64_t rid = Decoder::Decode(buf->peek(), srvName, body);
                buf->retrieve(pkg_len);

                bool isSuccess = Invock(srvName, body, resp);
                Bytes bytes;

                if (isSuccess) {
                    bytes = Encoder::SuccessRes(rid, resp.c_str(), resp.size());
                } else {
                    bytes = Encoder::ErrorRes(rid, ERR, resp.c_str());
                }
                conn->send(bytes.data(), bytes.size());
            }
    }
};


inline RpcServer& RpcServer::GetInstance() {
    static RpcServer rpcServer;
    return rpcServer;
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