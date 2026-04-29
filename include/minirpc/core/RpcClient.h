#pragma once

#include "minirpc/common/functioin_traits.h"
#include "minirpc/common/RpcException.h"
#include "minirpc/common/Buffer.h"
#include "minirpc/common/Response.h"
#include "minirpc/common/ThreadPool.h"
#include "minirpc/protocol/Serialize.h"
#include "minirpc/protocol/Encoder.h"
#include "minirpc/protocol/Decoder.h"
#include "minirpc/protocol/Protocol.h"
#include "minirpc/net/utils.h"
#include "minirpc/net/Conn.h"
#include "minirpc/core/macro/rpc_service_stub.h"
#include "minirpc/core/IConnectionPoolFactory.h"
#include "minirpc/core/IConnection.h"


#include <iostream>
#include <cstdint>   // 添加这行
#include <cstddef>   // 可选，提供 size_t
#include <tuple>
#include <vector>
#include <unordered_map>
#include <future>
#include <atomic>
#include <sys/epoll.h>


namespace minirpc
{

namespace {
    const int MAX_PACKAGE_LEN = 2048;
}

class IConnection;

/**
 * @class RpcClient
 * @brief RPC 客户端核心类，采用单例模式
 * 
 * 该类负责：
 * - 管理与服务器的连接
 * - 发送 RPC 请求并接收响应
 * - 处理异步调用和超时
 * 
 * 使用示例：
 * @code
 * UserService::UserService_Stub stub;
 * auto result = stub.login("user", "pass");
 * @endcode
 * 
 * @note 该类通过宏 RPC_SERVICE_STUB 自动生成代理类
 */
class RpcClient
{

private:
    uint64_t request_id_;
    std::unordered_map<uint64_t, std::promise<Response>> promiseMap_; // <request—id, promise>
    ThreadPool thread_pool_;

    std::mutex mutex_;

    // 连接池
    IConnectionPoolFactoryPtr connnection_pool_factory_ = nullptr;

    // 私有构造，只允许单例，仅在第一个Stub创建时初始化
    RpcClient();
public:    
    ~RpcClient();

    static RpcClient& GetInstance();

    void messageHandler(IConnection* c);

    bool send(const Bytes& bytes, const std::string& service_name, const std::string &group_name = "DEFAULT_GROUP");

    template<class R, typename ...Args>
    inline uint8_t call(const std::string& srvName, const std::tuple<Args...>& args, R& ret);

    template<typename ...Args>
    inline bool call(const std::string& srvName, const std::tuple<Args...>& args);


    template<class T, typename ...Args> 
    inline std::future<T> async_call(const std::string& srvName, const std::tuple<Args...>& args);

};


template<class R, typename ...Args>
uint8_t RpcClient::call(const std::string& srvName, const std::tuple<Args...>& args, R& ret) {

    std::string body = Serialize::Serialization(args);
    // 需要设置Request_id
    auto bytes = Encoder::Encode(srvName, body);

    int id = srvName.find('.');
    std::string name = srvName.substr(0, id);


    int rid;
    std::future<Response> f;
    // 发送数据
    {
        std::lock_guard<std::mutex> lock(mutex_);
        rid = request_id_;
        reinterpret_cast<ProtocolHeader*>(bytes.data())->request_id = rid;
        promiseMap_[rid] = std::promise<Response>();
        f = promiseMap_[rid].get_future();
        ++request_id_;
        if (!send(bytes, name)) return false;
    }

    // 设置超时返回
    auto status = f.wait_for(std::chrono::seconds(5));
    if (status == std::future_status::timeout) {
        promiseMap_.erase(rid);
        return TIMEOUT; // 定义超时错误码
    }

    // 阻塞获取结果
    Response r = f.get();

    if (r.state == SUCCESS) {
        ret = Serialize::Deserialization<R>(r.data);
    }

    return r.state;
}


template<typename ...Args>
bool RpcClient::call(const std::string& srvName, const std::tuple<Args...>& args) {
    std::string body = Serialize::Serialization(args);
    // 需要设置Request_id
    auto bytes = Encoder::Encode(srvName, body);

    int id = srvName.find('.');
    std::string name = srvName.substr(0, id);

    int rid;
    std::future<Response> f;
    // 发送数据
    {
        std::lock_guard<std::mutex> lock(mutex_);
        rid = request_id_;
        reinterpret_cast<ProtocolHeader*>(bytes.data())->request_id = rid;
        promiseMap_[rid] = std::promise<Response>();
        f = promiseMap_[rid].get_future();
        ++request_id_;
        if (!send(bytes, name)) return false;
    }

    // 阻塞获取结果
    return f.get().state;
}


template<class T, typename ...Args> 
std::future<T> RpcClient::async_call(const std::string& srvName, const std::tuple<Args...>& args) {
    // set{bool, promise}
    std::future<T> f = std::async(std::launch::async, [&srvName, &args](){
        // 判断类型是不是空
        bool is_success;
        if constexpr (std::is_void_v<T>) {
            is_success = call(srvName, args);
            return;
        }
        else {
            T res;
            is_success = call(srvName, args, res);
            return res;
        }
    });

    return f;
}


} // namespace minirpc