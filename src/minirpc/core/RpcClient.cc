#include "minirpc/core/RpcClient.h"

#include "minirpc/core/IConnection.h"
#include "minirpc/core/IConnectionPoolFactory.h"

#include <thread>
#include <sys/eventfd.h>


namespace minirpc
{

namespace {
    std::mutex& local_service_address_mutex() {
        static std::mutex instance;
        return instance;
    }
    std::unordered_map<std::string, std::string>& local_service_addresses() {
        static std::unordered_map<std::string, std::string> instance;
        return instance;
    }
}

void RpcClient::setLocalServiceAddress(const std::string& service_name, const std::string& address) {
    std::lock_guard<std::mutex> lock(local_service_address_mutex());
    local_service_addresses()[service_name] = address;
}

std::string RpcClient::getLocalServiceAddress(const std::string& service_name) {
    std::lock_guard<std::mutex> lock(local_service_address_mutex());
    auto it = local_service_addresses().find(service_name);
    if (it != local_service_addresses().end()) {
        return it->second;
    }
    return {};
}

RpcClient::RpcClient()
    : request_id_(0),
    connection_pool_factory_(IConnectionPoolFactory::CreateConnectionPoolFactory()) {
    connection_pool_factory_->setMessageHandler(
        [this](IConnection* c) { messageHandler(c); });
}


RpcClient::~RpcClient() {
}

static std::unique_ptr<RpcClient> s_rpc_client_instance;

RpcClient& RpcClient::GetInstance() {
    if (!s_rpc_client_instance) {
        s_rpc_client_instance = std::make_unique<RpcClient>();
    }
    return *s_rpc_client_instance;
}

void RpcClient::ResetInstance() {
    s_rpc_client_instance.reset();
}

/// @brief 发送数据，复用连接池中的连接
/// @param bytes data
/// @param service_name 服务名
/// @param group_name 组名
/// @return 
bool RpcClient::send(const Bytes& bytes, const std::string& service_name, const std::string &group_name) {
    // 发送消息
    try
    {
        IConnectionPool* pool = connection_pool_factory_->getConnectionPool(service_name, group_name);
        if (pool == nullptr) {
            LOG_ERROR("connection pool not found for service %s@%s", service_name.c_str(), group_name.c_str());
            return false;
        }
        IConnectionPtr conn = pool->getConnection();
        conn->send(bytes);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR("%s", e.what());
        return false;
    }
    return true;
}


/**
 * RpcClient类的消息处理函数
 * 用于处理客户端连接上的IO事件，默认使用边缘触发(ET)模式
 * @param c 指向连接对象的指针，包含了连接相关的信息和方法
 */
void RpcClient::messageHandler(IConnection* c) {
    IConnection::processConnection(c, [this](ProtocolHeader& header, const std::string& body) {
        Response resp{header.code, body};
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = promiseMap_.find(header.request_id);
            if (it != promiseMap_.end()) {
                it->second.set_value(resp);
                promiseMap_.erase(it);
            }
        }
        return true;
    });
}
    
} // namespace minirpc
