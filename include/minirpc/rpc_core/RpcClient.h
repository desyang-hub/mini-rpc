/**
 * @FilePath     : /mini-rpc/include/minirpc/rpc_core/RpcClient.h
 * @Description  :  
 * @Author       : desyang
 * @Date         : 2026-06-08 15:18:23
 * @LastEditors  : desyang
 * @LastEditTime : 2026-06-10 10:32:09
**/
#pragma once

#include <mutex>
#include <string>
#include <future>
#include <cstdint>
#include <unordered_map>
#include <unistd.h>

#include "minirpc/protocol/Encoder.h"
#include "minirpc/protocol/Decoder.h"
#include "minirpc/protocol/Serialize.h"
#include "minirpc/common/Response.h"
#include "minirpc/common/RpcException.h"
#include "minirpc/common/function_traits.h"
#include "minirpc/rpc_core/macro/rpc_service_stub.h"
#include "minirpc/net_muduo/TcpClient.h"

#include <muduo/net/InetAddress.h>
#include <atomic>
#include <Nacos.h>
#include <list>

// rpc client 需要有哪些功能
// 1. 通过一个宏用于服务函数，这个服务函数无需实现，只需要声明即可
// 2. 用户通过代理类调用服务函数，过程中，将函数名和参数进行序列化成字符串，并打包进行发送
// 3. 为函数预留一个future<R> 用于接收返回值，并将对应的promise，绑定到unordered_map中，用户端调用future<R>::get()阻塞，直到服务端返回结果，并将结果设置到promise中


namespace minirpc
{
    
class RpcClient
{
private:
    mutable std::mutex mutex_;
    uint64_t id_;
    // promise
    std::unordered_map<uint64_t, std::promise<Response>> promises_;

    TcpClient* tcpClient_;

    std::atomic_bool is_init;

    using ServiceSearchHandler = std::function<void(nacos::NamingService *)>;
    std::queue<ServiceSearchHandler> workers_;
    std::mutex wokers_mutex_;
    std::condition_variable condition_;
    std::thread searchServiceWorker_;
    bool is_close;

    // 搜索服务后台进程
    void ServiceSearchWorker() {
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
    
        // std::list <nacos::Instance> instances = namingSvc->getAllInstances("TestNamingService1");
        // cout << "getAllInstances from server:" << endl;
        // for (list<Instance>::iterator it = instances.begin();
        //      it != instances.end(); it++) {
        //     cout << "Instance:" << it->toString() << endl;
        // }
    }
    
public:
    // this可能会导致异常，如果有条件，尽量换成shared_ptr
    RpcClient() : id_(0), tcpClient_(nullptr), is_init(false), is_close(false), searchServiceWorker_(&RpcClient::ServiceSearchWorker, this) {
        std::thread tcpClientWorker([this]{
            TcpClient tcpClient(muduo::net::InetAddress("127.0.0.1", 8080), std::bind(&RpcClient::Handler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

            // 需要处理同步问题
            tcpClient_ = &tcpClient;

            is_init.store(true);

            tcpClient.Start();
        });

        tcpClientWorker.detach();
    }

    ~RpcClient() {
        {
            std::lock_guard<std::mutex> lock(wokers_mutex_);
            is_close = true;
        }    
        condition_.notify_all();

        if (searchServiceWorker_.joinable()) {
            searchServiceWorker_.join();
        }
    }

    std::list<nacos::Instance> getAllInstances(const std::string& name) {
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

    // 获取单实例
    static RpcClient& GetInstance();

    // 代理函数通过方法名和序列化结果作为参数，来调用RpcClient的Invock方法，
    template<class R>
    std::future<R> AsyncInvoke(const Bytes& bytes);

    template<class R>
    R Invoke(const Bytes& bytes);

    template<class R, class ...Args>
    R Call(const char* name, Args&& ...args) {
        Bytes bytes;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            ++id_;
            if constexpr (sizeof...(Args) == 0) {
                // bytes = Encoder::Encode(name, nullptr, 0);
                bytes = Encoder::EncodeReq(id_, name, nullptr, 0);
            }
            // 单参函数
            else if constexpr (sizeof...(Args) == 1) {
                // bytes = Encoder::Encode(name, Serialize::Serialization(args...));
                std::string body = Serialize::Serialization(args...);
                bytes = Encoder::EncodeReq(id_, name, body.c_str(), body.size());
            } else {
                auto args_tuple = std::make_tuple(std::forward<Args>(args)...);
                std::string body = Serialize::Serialization(args_tuple);
                bytes = Encoder::EncodeReq(id_, name, body.c_str(), body.size());
            }
        }

        return Invoke<R>(bytes);
    }


    void Handler(const muduo::net::TcpConnectionPtr& conn,
        muduo::net::Buffer* buf,
        muduo::Timestamp t) { // 这是回调函数，当有结果从服务端发送过来
            // 1. 尝试接收完整的package
            // 2. Decode package 成为 srvName, paramBody
            // 3. promise::set_value
            int pkg_len = Decoder::Decode(buf->peek(), buf->readableBytes());

            LOG_INFO("Rpc client recv data: ");

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
};

// inline RpcClient::RpcClient() : id_(0) {

// }

// inline RpcClient::~RpcClient() {

// }

// 获取单实例
inline RpcClient& RpcClient::GetInstance() {
    static RpcClient rpcClient;
    return rpcClient;
}

// 代理函数通过方法名和序列化结果作为参数，来调用RpcClient的Invock方法，
template<class R>
inline std::future<R> RpcClient::AsyncInvoke(const Bytes& bytes) {

    std::future<Response> f;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        promises_[id_] = std::promise<Response>();
        f = promises_[id_].get_future();
    }

    // 此处还应该为其添加唯一标识
    // Bytes bytes = Encoder::Encode(name, data, len);
    // 将数据发送出去，目前未完成回调
    // send(bytes);

    while (!is_init.load()) {
        std::cout << "circle" << std::endl;
        sleep(1);
    }

    if (tcpClient_) {
        LOG_INFO("tcpClient sendRequest");
        tcpClient_->sendRequest(bytes.data(), bytes.size());
    } else {
        LOG_INFO("tcpClient not sendRequest");
    }
        

    // 将f->get 封装成一个异步任务
    std::future<R> fut = std::async(std::launch::async, [f = std::move(f)]() mutable {
        Response res = f.get();
        if (res.state != SUCCESS) {
            // 默认如果失败的话res.data就装异常就好了
            throw RpcException(res.data);
        }

        return Serialize::Deserialization<R>(res.data.c_str(), res.data.size());
    });

    return fut;
}

template<class R>
inline R RpcClient::Invoke(const Bytes& bytes) {
    return AsyncInvoke<R>(bytes).get();
}

} // namespace minirpc