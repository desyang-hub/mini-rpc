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

#include <memory>

namespace minirpc
{

RpcServer::RpcServer() : is_close_(false) {}

RpcServer::~RpcServer()
{
    {
        std::lock_guard<std::mutex> lock(close_mutex_);
        is_close_ = true;
    }
    condition_.notify_all();

    if (registerWorker_.joinable()) {
        registerWorker_.join();
    }
}

RpcServer& RpcServer::GetInstance()
{
    static RpcServer instance;
    return instance;
}

void RpcServer::Start(int port, const char* name, const char* nacosAddr)
{
    port_      = port;
    nacos_addr_ = nacosAddr;

    // Background service registration thread
    registerWorker_ = std::thread(&RpcServer::ServiceRegisterWorker, this);

    tcpServer_ = std::make_unique<TcpServer>(port, name);
    tcpServer_->setMessageCallback(
        std::bind(&RpcServer::MessageHandler, this,
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    tcpServer_->Start();

    std::unique_lock<std::mutex> lock(close_mutex_);
    close_condition_.wait(lock, [this]{ return is_close_; });
}

void RpcServer::Stop()
{
    tcpServer_->Stop();
    {
        std::lock_guard<std::mutex> lock(close_mutex_);
        is_close_ = true;
    }
    close_condition_.notify_all();
}

void RpcServer::addServiceInstance(const char* name, const char* groupName, const char* clusterName)
{
    nacos::Instance instance;
    instance.clusterName = clusterName;
    instance.groupName   = groupName;
    instance.serviceName = name;
    instance.ip          = "127.0.0.1";
    instance.ephemeral   = true;

    {
        std::lock_guard<std::mutex> lock(instance_mutex_);
        instances_.emplace(std::move(instance));
    }
    condition_.notify_one();
}

bool RpcServer::Invock(const std::string& srvName, const std::string& req, std::string& resp)
{
    RpcHandler handler;
    bool found = false;

    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto it = handlers_.find(srvName);
        if (it != handlers_.end()) {
            handler = it->second;
            found = true;
        }
    }

    if (found) {
        try {
            handler(req, resp);
            return true;
        } catch (const std::exception& e) {
            resp = e.what();
            LOG_ERROR("Service %s invoke err: %s", srvName.c_str(), resp.c_str());
            return false;
        }
    }

    LOG_ERROR("RPC service not found: %s", srvName.c_str());
    return false;
}

void RpcServer::ShowAllService()
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    for (const auto& srv : service_names_) {
        LOG_INFO("server name: %s", srv.c_str());
    }
}

void RpcServer::MessageHandler(const muduo::net::TcpConnectionPtr& conn,
                                muduo::net::Buffer* buf,
                                muduo::Timestamp)
{
    while (buf->readableBytes()) {
        int pkg_len = Decoder::Decode(buf->peek(), buf->readableBytes());

        if (pkg_len == ERR) {
            throw RpcException("recv pkg msg exception");
        }
        if (pkg_len == UN_FINISH) {
            return;
        }

        std::string srvName;
        std::string body;
        uint64_t rid = Decoder::Decode(buf->peek(), srvName, body);
        buf->retrieve(pkg_len);  // +4 for check_num

        threadPool_.enqueue([this, conn, rid, srvName = std::move(srvName), body = std::move(body)]() {
            std::string resp;
            bool ok = Invock(srvName, body, resp);

            Bytes result;
            if (ok) {
                result = Encoder::successResponse(rid, resp);
            } else {
                result = Encoder::errorResponse(rid, FAILED, resp);
            }
            conn->send(result.data(), result.size());
        });
    }
}

void RpcServer::ServiceRegisterWorker()
{
    nacos::Properties configProps;
    configProps[nacos::PropertyKeyConst::SERVER_ADDR] =
        nacos_addr_.empty() ? "127.0.0.1" : nacos_addr_;

    nacos::INacosServiceFactory* factory =
        nacos::NacosFactoryFactory::getNacosFactory(configProps);
    nacos::ResourceGuard<nacos::INacosServiceFactory> guardFactory(factory);

    auto namingSvc = factory->CreateNamingService();
    nacos::ResourceGuard<nacos::NamingService> guardNaming(namingSvc);

    while (true) {
        nacos::Instance instance;

        {
            std::unique_lock<std::mutex> lock(instance_mutex_);
            condition_.wait(lock, [this]{ return !instances_.empty() || is_close_; });

            if (is_close_) break;

            instance = std::move(instances_.front());
            instances_.pop();
        }

        instance.port = port_;

        try {
            namingSvc->registerInstance(instance.serviceName, instance);
        } catch (nacos::NacosException& e) {
            LOG_ERROR("Nacos registration failed: %s", e.what());
        }
    }
}

} // namespace minirpc
