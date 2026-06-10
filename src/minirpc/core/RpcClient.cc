/**
 * @FilePath     : /mini-rpc/src/minirpc/core/RpcClient.cc
 * @Description  :
 * @Author       : desyang
 * @Date         : 2026-06-08 15:18:23
 * @LastEditors  : desyang
 * @LastEditTime : 2026-06-10 17:40:19
**/

#include "minirpc/core/RpcClient.h"
#include "minirpc/protocol/Encoder.h"
#include "minirpc/protocol/Decoder.h"
#include "minirpc/protocol/Serialize.h"
#include "minirpc/common/RpcException.h"
#include "minirpc/net/TcpClient.h"
#include "minirpc/net/ConnectionManager.h"
#include "minirpc/common/logger.h"

#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/Callbacks.h>

#include <Nacos.h>

namespace minirpc
{

RpcClient& RpcClient::GetInstance() {
    static RpcClient rpcClient;
    return rpcClient;
}

// 构造函数
RpcClient::RpcClient() : id_(0), is_close(false),
    searchServiceWorker_(&RpcClient::ServiceSearchWorker, this), connMgr_() {
    connMgr_.setMessageCallback(
        [this](const muduo::net::TcpConnectionPtr& conn,
               muduo::net::Buffer* buf,
               muduo::Timestamp ts) {
            this->MessageHandler(conn, buf, ts);
        });
}

// 析构函数
RpcClient::~RpcClient() {
    {
        std::lock_guard<std::mutex> lock(wokers_mutex_);
        is_close = true;
    }
    condition_.notify_all();

    if (searchServiceWorker_.joinable()) {
        searchServiceWorker_.join();
    }
}

// 获取所有实例
std::list<nacos::Instance> RpcClient::getAllInstances(const std::string& name) {
    // 1. 创建 shared_ptr<packaged_task>
    auto task = std::make_shared<std::packaged_task<std::list<nacos::Instance>(nacos::NamingService*)>>(
        [name](nacos::NamingService* namingSvc) {
            return namingSvc->getAllInstances(name);
        }
    );

    // 2. 获取 future
    auto fut = task->get_future();

    {
        std::lock_guard<std::mutex> lock(wokers_mutex_);

        if (is_close) {
            throw RpcException("Call getAllInstances during program close");
        }

        // 3. 提交可复制的 lambda 到队列
        workers_.emplace([task](nacos::NamingService* namingSvc) {
            (*task)(namingSvc); // 执行任务，触发 set_value
        });
    }

    condition_.notify_one(); // 唤醒工作线程

    // 4. 同步等待结果
    return fut.get();
}

// 消息回调函数
void RpcClient::MessageHandler(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp t) {
    // 这是回调函数，当有结果从服务端发送过来
    // 1. 尝试接收完整的 package
    // 2. Decode package 成为 srvName, paramBody
    // 3. promise::set_value
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

        Response resp;
        int id = Decoder::Decode(buf->peek(), resp);

        LOG_INFO("rid: %d", id);
        buf->retrieve(pkg_len);

        std::lock_guard<std::mutex> lock(mutex_);
        if (promises_.count(id) == 0) {
            throw RpcException("promise id not exists.");
        }
        promises_[id].set_value(std::move(resp));
        promises_.erase(id);
    }
}

// 搜索服务后台进程
void RpcClient::ServiceSearchWorker() {
    nacos::Properties configProps;
    configProps[nacos::PropertyKeyConst::SERVER_ADDR] = "127.0.0.1";
    nacos::INacosServiceFactory *factory = nacos::NacosFactoryFactory::getNacosFactory(configProps);
    nacos::ResourceGuard <nacos::INacosServiceFactory> _guardFactory(factory);
    nacos::NamingService *namingSvc = factory->CreateNamingService();
    nacos::ResourceGuard <nacos::NamingService> _guardService(namingSvc);

    while (true) {
        ServiceSearchHandler work;

        {
            std::unique_lock<std::mutex> lock(wokers_mutex_);
            condition_.wait(lock, [this]{
                return !workers_.empty() || is_close;
            });

            if (!workers_.empty()) {
                work = std::move(workers_.front());
                workers_.pop();
            }

            if (is_close) {
                break;
            }
        }

        work(namingSvc);
    }
}

} // namespace minirpc
