/**
 * @FilePath     : /mini-rpc/src/minirpc/core/RpcServer.cc
 * @Description  :
 * @Author       : desyang
 * @Date         : 2026-06-08 16:19:14
 * @LastEditors  : desyang
 * @LastEditTime : 2026-06-18 14:57:49
**/

#include "minirpc/core/RpcServer.h"
#include "minirpc/common/function_traits.h"
#include "minirpc/common/tuple_helper.h"
#include "minirpc/common/logger.h"
#include "minirpc/common/RpcException.h"
#include "minirpc/protocol/Serialize.h"
#include "minirpc/core/macro/rpc_service_bind.h"
#include "minirpc/protocol/Decoder.h"
#include "minirpc/protocol/Encoder.h"
#include "minirpc/net/TcpServer.h"
#include <Nacos.h>

#include <muduo/net/TcpConnection.h>
#include <muduo/net/Buffer.h>
#include <muduo/base/Timestamp.h>

#include <memory>

namespace minirpc
{

RpcServer::RpcServer() : is_close_(false) {
}

RpcServer::~RpcServer() {
    {
        std::lock_guard<std::mutex> lock(close_mutex_);
        is_close_ = true;
    }
    condition_.notify_all();

    if (registerWorker_.joinable()) {
        registerWorker_.join();
    }
}

RpcServer& RpcServer::GetInstance() {
    static RpcServer rpcServer;
    return rpcServer;
}

void RpcServer::Start(int port, const char* name) {
    port_ = port;
    // 启用后台常驻注册程序
    registerWorker_ = std::thread(&RpcServer::ServiceRegisterWorker, this);

    tcpServer_ = std::make_unique<TcpServer>(port, name);
    tcpServer_->setMessageCallback(std::bind(&RpcServer::MessageHandler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    tcpServer_->Start();

    std::unique_lock<std::mutex> lock(close_mutex_);
    close_condition_.wait(lock, [this]{
        return is_close_;
    });
}

void RpcServer::Stop() {
    tcpServer_->Stop();
    {
        std::lock_guard<std::mutex> lock(close_mutex_);
        is_close_ = true;
    }
    close_condition_.notify_all();
}

void RpcServer::addServiceInstance(const char* name, const char* groupName, const char* clusterName) {
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

/// @brief 调用 rpc service
/// @param srvName 服务名
/// @param req 请求参数包，未序列化
/// @param resp 返回序列化后的结果
/// return
bool RpcServer::Invock(const std::string& srvName, const std::string& req, std::string& resp) {
    RpcHandler handler; // 假设你的 handler 支持拷贝或移动
    bool found = false;

    {
        // 1. 极小范围的读锁：仅仅用来拷贝出函数指针/对象
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto it = handlers_.find(srvName);
        if (it != handlers_.end()) {
            handler = it->second;
            found = true;
        }
    } // 2. 出了这个括号，锁就立刻释放了！

    // 3. 在锁的外面执行真正的业务逻辑！
    if (found) {
        try {
            handler(req, resp);
            return true;
        } catch (const std::exception& e) {
            resp = e.what();
            LOG_ERROR("Service %s invoke err: %s", srvName.c_str(), resp.c_str());
            return false;
        }
    } else {
        LOG_ERROR("RPC service not found: %s", srvName.c_str());
        return false;
    }
}

void RpcServer::ShowAllService() {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    for (const auto& srv : service_names_) {
        LOG_INFO("server name: %s", srv.c_str());
    }
}

void RpcServer::MessageHandler(const muduo::net::TcpConnectionPtr& conn,
    muduo::net::Buffer* buf,
    muduo::Timestamp t) {
    // 这是回调函数，当有请求消息从用户端发送过来
    // 1. 尝试接收完整的 package
    // 2. Decode package 成为 srvName, paramBody
    // 3. Invock(srvName, paramBody)

    while (buf->readableBytes()) {
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
            // LOG_INFO("request id: %lu", rid);
            buf->retrieve(pkg_len);


            threadPool_.enqueue([srvName = std::move(srvName), body = std::move(body), this, conn, rid]{
                std::string resp;
                bool isSuccess = Invock(srvName, body, resp);

                Bytes bytes;
                if (isSuccess) {
                    bytes = Encoder::SuccessRes(rid, resp.c_str(), resp.size());
                } else {
                    bytes = Encoder::ErrorRes(rid, ERR, resp.c_str());
                }
                conn->send(bytes.data(), bytes.size());
            });
        }
    }
}

// 此处实现注册逻辑
void RpcServer::ServiceRegisterWorker() {
    // 启动注册服务
    nacos::Properties configProps;
    configProps[nacos::PropertyKeyConst::SERVER_ADDR] = "127.0.0.1"; // 注册中心地址，后续使用配置文件 + 域名来替换
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

} // namespace minirpc
