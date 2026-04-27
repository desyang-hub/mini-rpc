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
#include "minirpc/core/RpcServer.h"
#include "minirpc/net/utils.h"
#include "minirpc/net/Conn.h"


#include <iostream>
#include <cstdint>   // 添加这行
#include <cstddef>   // 可选，提供 size_t
#include <tuple>
#include <vector>
#include <unordered_map>
#include <future>
#include <atomic>
#include <sys/epoll.h>

// rpc client 用于为rpc调用的实现，ServiceCli调用函数的底层代理就会由RpcClient来实现

/**
 * @brief 暂时就不加通信了，直接就是模拟，将函数名和参数包(tuple)传入，接着就会开始封包，发送
 * 
 * 
 */

// ===========================================================
// 修正后的 _RPC_STUB_METHOD (支持可变参数调用)
// ===========================================================
#define _RPC_STUB_METHOD(Class, Method) \
    /* 类型萃取 */ \
    using MethodType_##Method = decltype(&Class::Method); \
    using ReturnType_##Method = typename minirpc::function_traits<MethodType_##Method>::return_type; \
    using ArgsTuple_##Method = typename minirpc::function_traits<MethodType_##Method>::args_tuple; \
    \
    /* 辅助类型：如果是 void 则用 int 占位，避免定义 void 变量 */ \
    using RetType_##Method = typename std::conditional<std::is_void<ReturnType_##Method>::value, int, ReturnType_##Method>::type; \
    \
    /* ✅ 修改点：使用可变参数模板 (Args&&... args)，允许用户直接传参 */ \
    template<typename... Args> \
    ReturnType_##Method Method(Args&&... args) { \
        static_assert(std::is_convertible_v<std::tuple<std::decay_t<Args>...>, ArgsTuple_##Method>, "Parameter types mismatch for method " #Method); \
        std::string srvName = #Class "." #Method; \
        \
        /* ✅ 修改点：在函数内部手动打包成 tuple */ \
        ArgsTuple_##Method args_tuple = std::make_tuple(std::forward<Args>(args)...); \
        \
        RetType_##Method ret_val; \
        \
        if constexpr (std::is_void_v<ReturnType_##Method>) { \
            uint8_t code = minirpc::RpcClient::GetInstance().call(srvName, args_tuple); \
            if (code != minirpc::SUCCESS) { \
                throw minirpc::RpcException("RPC Call Failed: err_code=" + std::to_string(code)); \
            } \
        } else { \
            uint8_t code = minirpc::RpcClient::GetInstance().call(srvName, args_tuple, ret_val); \
            if (code != minirpc::SUCCESS) { \
                throw minirpc::RpcException("RPC Call Failed: err_code=" + std::to_string(code)); \
            } \
        } \
        \
        if constexpr (!std::is_void_v<ReturnType_##Method>) { \
            return static_cast<ReturnType_##Method>(ret_val); \
        } \
    }



// 2. 定义不同参数数量的实现宏
// 注意：第一个参数永远是 Class，后面才是方法
#define _RPC_STUB_IMPL_1(Class, M1) \
    _RPC_STUB_METHOD(Class, M1)

#define _RPC_STUB_IMPL_2(Class, M1, M2) \
    _RPC_STUB_METHOD(Class, M1) \
    _RPC_STUB_METHOD(Class, M2)

#define _RPC_STUB_IMPL_3(Class, M1, M2, M3) \
    _RPC_STUB_METHOD(Class, M1) \
    _RPC_STUB_METHOD(Class, M2) \
    _RPC_STUB_METHOD(Class, M3)

#define _RPC_STUB_IMPL_4(Class, M1, M2, M3, M4) \
    _RPC_STUB_METHOD(Class, M1) \
    _RPC_STUB_METHOD(Class, M2) \
    _RPC_STUB_METHOD(Class, M3) \
    _RPC_STUB_METHOD(Class, M4)

#define _RPC_STUB_IMPL_5(Class, M1, M2, M3, M4, M5) \
    _RPC_STUB_METHOD(Class, M1) \
    _RPC_STUB_METHOD(Class, M2) \
    _RPC_STUB_METHOD(Class, M3) \
    _RPC_STUB_METHOD(Class, M4) \
    _RPC_STUB_METHOD(Class, M5)

// 3. 分发宏 (核心逻辑)
// 这里的逻辑是：_RPC_STUB_ALL(UserService, add, sub, print)
// PP_NARG(add, sub, print) 应该返回 3
// 然后拼接出 _RPC_STUB_IMPL_3(UserService, add, sub, print)

#define _RPC_STUB_ALL(Class, ...) \
    _RPC_STUB_DISPATCH(_RPC_STUB_IMPL_, Class, __VA_ARGS__)

// 这里的 N 是通过 PP_NARG 计算出来的方法数量
#define _RPC_STUB_DISPATCH(Func, Class, ...) \
    _RPC_STUB_DISPATCH_(_RPC_STUB_DISPATCH__, Func, PP_NARG(__VA_ARGS__), Class, __VA_ARGS__)

#define _RPC_STUB_DISPATCH_(CALLBACK, Func, N, Class, ...) \
    CALLBACK(Func, N, Class, __VA_ARGS__)

// 最终拼接：Func##N -> _RPC_STUB_IMPL_3
#define _RPC_STUB_DISPATCH__(Func, N, Class, ...) \
    Func##N(Class, __VA_ARGS__)

// 4. 最终用户宏
/**
 * @def RPC_SERVICE_STUB(Class, ...)
 * @brief 生成客户端代理类
 * @param Class 服务类名
 * @param ... 方法名列表，必须与 RPC_SERVICE_BIND 中的方法列表一致
 * 
 * @note 生成的代理类名为 Class##_Stub
 */
#define RPC_SERVICE_STUB(Class, ...) \
    public: /* ✅ 修复点：确保生成的类是 public 的 */ \
    class Class##_Stub { \
    public: \
        Class##_Stub() {minirpc::RpcClient::GetInstance();} \
        /* 展开具体的方法 */ \
        _RPC_STUB_ALL(Class, __VA_ARGS__) \
    }; \
private:



namespace minirpc
{
const int MAX_PACKAGE_LEN = 2048;



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
    int sockfd_;
    int epollfd_;
    uint64_t request_id_;
    int wakeup_fd_;   // eventfd 或 pipe 读端
    // 这里最好设计成 {bool , string}
    std::unordered_map<uint64_t, std::promise<Response>> promiseMap_; // <request—id, promise>
    ThreadPool threadPool_;

    std::mutex send_lock_;
    std::atomic_bool is_running_ = true;

    std::thread loop_woker_;

    std::unordered_map<int, Conn*> connMap_;

    // 私有构造，只允许单例，仅在第一个Stub创建时初始化
    RpcClient();

    
    ~RpcClient();

    void removeConn(Conn* c) {
        auto it = connMap_.find(c->fd());
    
        // not exists
        if (it == connMap_.end()) {
            close(c->fd());
            return;
        }
    
        if (epoll_ctl(epollfd_, EPOLL_CTL_DEL, c->fd(), nullptr) == -1) {
            LOG_ERROR("epoll ctl error");
        }
        close(c->fd());
        connMap_.erase(it);
        delete c;
    }


    void MessageHandler(Conn* c) {
        // 这里做的就是io处理逻辑，默认使用ET触发模式，需要一次处理完数据
        int cli_fd = c->fd();
    
        // ET, 必须要一次处理完，直到缓存空了
        // 接收用户消息，并写回
        while (true) {
            int len = c->readMsg();
            
            // 出错了，或断开了
            if (len < 0) {
                LOG_DEBUG("User %d disconnected.", c->fd());
                removeConn(c);
                break;
            }
            else if (len == 0) {
                LOG_DEBUG("continue recv message.");
                break;
            }
            else {
                // TODO: 这里理论上应该放回调，当读入了消息就该回调接口
                // 解码读取消息，拷贝消息
                std::string body;
                std::string srv_name;
    
                ProtocolHeader header;

                bool is_success = c->decode(body, srv_name, header);
                if (is_success) {
                    auto it = promiseMap_.find(header.request_id);
                    assert(it != promiseMap_.end());
                    it->second.set_value({header.code, body});                    
                }
            }
        }
    }

public:
    static RpcClient& GetInstance();

    // 此处实现一个Rpc用户端读循环，用于不断从io流读取数据
    void ReadLoop() {
        // int port = 8080;

        // {
        //     std::lock_guard<std::mutex> lock(send_lock_); 
        //     sockfd_ = Dial(port);
        // }

        set_nonblocking(sockfd_);
        
        int save_errno;
        ProtocolHeader header;
        int header_len = sizeof(header);
        int pkg_len = -1;
        std::vector<uint8_t> data(MAX_PACKAGE_LEN);


        // 使用epoll管理用户端
        // 1. 创建epoll
        epollfd_ = epoll_create1(EPOLL_CLOEXEC);
        if (epollfd_ < 0) {
            // perror("epoll create error");
            close(sockfd_);
            LOG_FATAL("epoll create error");
        }

        // 2. 注册fd监听到epoll
        epoll_event ev;
        connMap_[sockfd_] = new Conn(sockfd_);
        ev.events = EPOLLIN | EPOLLET; // ET 触发，只要通知了就要一直读，直到空了(EAGAIN, EWOULDBLOCK)
        ev.data.ptr = connMap_[sockfd_];
        
        LOG_INFO("Epoll ET Mode epfd=%d", epollfd_);
        // std::cout << "Epoll ET Mode epfd=" << epollfd << std::endl;

        if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, sockfd_, &ev) < 0) {
            // perror("epoll add sockfd error");
            close(sockfd_);
            close(epollfd_);
            // exit(-1);
            LOG_FATAL("epoll add sockfd error");
        }


        epoll_event wake_ev;
        wake_ev.events = EPOLLIN | EPOLLET;
        wake_ev.data.ptr = new Conn(wakeup_fd_);
        if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, wakeup_fd_, &wake_ev) < 0) {
            LOG_FATAL("epoll add wakeup_fd error");
        }

        // 3. 进行事件监听
        // epoll_event events[1024];
        std::vector<epoll_event> events(128);

        while (is_running_.load()) {
            // 阻塞等待，返回触发的事件个数
            int numEvents = epoll_wait(epollfd_, events.data(), events.size(), -1); // -1 表示无限等待

            LOG_INFO("client numEvelents %d", numEvents);

            std::cout << "is_running_: " << is_running_.load() << std::endl;

            int saveErrno = errno;

            if (numEvents == -1) {
                if (errno == EINTR) continue;
                if (errno == EBADF) {
                    // fd 已经被关闭了，直接退出
                    break;
                }
                perror("epoll wait error");
                break;
            }

            if (numEvents == 0) {
                // 检查一下 fd 是否还有效
                // 如果 fd 已经无效，直接退出
                if (sockfd_ <= 0) {
                    break;
                }
                continue;
            }
            
            bool is_close = false;
            // 处理被激活的事件
            for (int i = 0; i < numEvents; ++i) {
                Conn* c = static_cast<Conn*>(events[i].data.ptr);
                int fd = c->fd();

                if (fd == wakeup_fd_) {
                    uint64_t val;
                    read(wakeup_fd_, &val, sizeof(val)); // 清空事件
                    // 唤醒后检查退出标志，若为 false 则跳出循环
                    if (!is_running_.load()) {
                        is_close = true;
                        break;
                    }
                    continue;
                }

                // 有消息可以读取了
                // std::thread worker(&RpcClient::MessageHandler, this, c);
                // worker.detach();
                threadPool_.submit(std::bind(&RpcClient::MessageHandler, this, c));
            }

            if (is_close) break;

            // 如果事件数量太多了，自动进行扩容
            if (numEvents == events.size()) {
                events.resize(2 * numEvents);
            }
        }
    }

    bool send(std::vector<uint8_t>& bytes) {
        // 发送消息
        if (::send(sockfd_, bytes.data(), bytes.size(), 0) == -1) {
            uint64_t id = reinterpret_cast<ProtocolHeader*>(bytes.data())->request_id = request_id_;
            promiseMap_.erase(id);
            LOG_ERROR("client send msg error %d", sockfd_);
            return false;
        }
        
        return true;
    }


public:
    template<class R, typename ...Args>
    uint8_t call(const std::string& srvName, const std::tuple<Args...>& args, R& ret) {

        std::string body = Serialize::Serialization(args);
        // 需要设置Request_id
        auto bytes = Encoder::Encode(srvName, body);


        int rid;
        std::future<Response> f;
        // 发送数据
        {
            std::lock_guard<std::mutex> lock(send_lock_);
            rid = request_id_;
            reinterpret_cast<ProtocolHeader*>(bytes.data())->request_id = rid;
            promiseMap_[rid] = std::promise<Response>();
            f = promiseMap_[rid].get_future();
            ++request_id_;
            if (!send(bytes)) return false;
        }

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
    bool call(const std::string& srvName, const std::tuple<Args...>& args) {
        std::string body = Serialize::Serialization(args);
        // 需要设置Request_id
        auto bytes = Encoder::Encode(srvName, body);


        int rid;
        std::future<Response> f;
        // 发送数据
        {
            std::lock_guard<std::mutex> lock(send_lock_);
            rid = request_id_;
            reinterpret_cast<ProtocolHeader*>(bytes.data())->request_id = rid;
            promiseMap_[rid] = std::promise<Response>();
            f = promiseMap_[rid].get_future();
            ++request_id_;
            if (!send(bytes)) return false;
        }

        // 阻塞获取结果
        return f.get().state;
    }


    template<class T, typename ...Args> 
    std::future<T> async_call(const std::string& srvName, const std::tuple<Args...>& args) {
        // set{bool, promise}
        std::future<T> f = std::async([&srvName, &args](){
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

};



} // namespace minirpc