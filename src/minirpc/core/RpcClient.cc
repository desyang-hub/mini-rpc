#include "minirpc/core/RpcClient.h"

#include "minirpc/core/IConnection.h"
#include "minirpc/core/IConnectionPoolFactory.h"

#include <thread>
#include <sys/eventfd.h>


namespace minirpc
{
    
RpcClient::RpcClient()
    : request_id_(0),
    connnection_pool_factory_(IConnectionPoolFactory::CreateConnectionPoolFactory()) {
    connnection_pool_factory_->setMessageHandler(
        [this](IConnection* c) { messageHandler(c); });
}


RpcClient::~RpcClient() {
}

RpcClient& RpcClient::GetInstance() {
    static RpcClient instance;
    return instance;
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
        IConnectionPool* pool = connnection_pool_factory_->getConnectionPool(service_name, group_name);
        IConnectionPtr conn = pool->getConnection();
        conn->send(bytes);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR("%s", e.what())
        std::cerr << e.what() << '\n';
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
        auto it = promiseMap_.find(header.request_id);
        if (it != promiseMap_.end()) {
            it->second.set_value({header.code, body});
        }
        return true;
    });
    c->close();
}
    
} // namespace minirpc
