#include "minirpc/core/RpcClient.h"

#include "minirpc/core/IConnection.h"
#include "minirpc/core/IConnectionPoolFactory.h"

#include <thread>
#include <sys/eventfd.h>


namespace minirpc
{
    
RpcClient::RpcClient()
    : request_id_(0),
    connnection_pool_factory_(IConnectionPoolFactory::CreateConnectionPoolFactory()) {}


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
    // 这里做的就是io处理逻辑，默认使用ET触发模式，需要一次处理完数据
    int cli_fd = c->fd();  // 获取连接的文件描述符

    // ET, 必须要一次处理完，直到缓存空了
    // 接收用户消息，并写回
    while (true) {
        int len = c->readMsg();  // 尝试从连接中读取消息
        
        // 出错了，或断开了
        if (len < 0) {
            LOG_DEBUG("User %d disconnected.", c->fd());  // 记录用户断开连接的日志
            c->close();  // 关闭连接
            break;  // 退出循环
        }
        else if (len == 0) {
            LOG_DEBUG("continue recv message.");  // 记录继续接收消息的日志
            break;  // 退出循环
        }
        else {
            // TODO: 这里理论上应该放回调，当读入了消息就该回调接口
            // 解码读取消息，拷贝消息
            std::string body;

            ProtocolHeader header;

            bool is_success = c->decode(header, body);
            if (is_success) {
                auto it = promiseMap_.find(header.request_id);
                assert(it != promiseMap_.end() && "Assert error: promise not found");
                it->second.set_value({header.code, body});                    
            }
        }
    }
}
    
} // namespace minirpc
