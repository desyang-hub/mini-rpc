#pragma once

#include "minirpc/common/functioin_traits.h"
#include "minirpc/common/RpcException.h"
#include "minirpc/common/Buffer.h"
#include "minirpc/common/Response.h"
#include "minirpc/protocol/Serialize.h"
#include "minirpc/protocol/Encoder.h"
#include "minirpc/protocol/Decoder.h"
#include "minirpc/core/RpcServer.h"
#include "minirpc/net/utils.h"


#include <iostream>
#include <cstdint>   // 添加这行
#include <cstddef>   // 可选，提供 size_t
#include <tuple>
#include <vector>
#include <unordered_map>
#include <future>

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
        static_assert(std::is_same_v<ArgsTuple_##Method, std::tuple<std::decay_t<Args>...>>, "Parameter types mismatch for method " #Method); \
        std::string srvName = #Class "." #Method; \
        \
        /* ✅ 修改点：在函数内部手动打包成 tuple */ \
        ArgsTuple_##Method args_tuple = std::make_tuple(std::forward<Args>(args)...); \
        \
        RetType_##Method ret_val; \
        \
        if constexpr (std::is_void_v<ReturnType_##Method>) { \
            if (!minirpc::RpcClient::call(srvName, args_tuple)) { \
                throw RpcException("RPC Call Failed: " + srvName); \
            } \
        } else { \
            if (!minirpc::RpcClient::call(srvName, args_tuple, ret_val)) { \
                throw RpcException("RPC Call Failed: " + srvName); \
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
#define RPC_SERVICE_STUB(Class, ...) \
    public: /* ✅ 修复点：确保生成的类是 public 的 */ \
    class Class##_Stub { \
    public: \
        /* 展开具体的方法 */ \
        _RPC_STUB_ALL(Class, __VA_ARGS__) \
    }; \
private:



namespace minirpc
{
    
class RpcServerDemo
{
public:
    static std::vector<uint8_t> RecvMsg(const std::vector<uint8_t>& bytes) {
        // 远程解析数据
        ProtocolHeader header;
        std::string srvName;
        std::string body;
        bool is_success = Decoder::Decode(bytes, header, srvName, body);

        std::string res;
        if  (is_success) {
            is_success = RpcServer::call(srvName, body, res);
        }

        if (!is_success) {
            header.request_id = 300;
        }

        header.magic = MAGIC_NUMBER;
        header.srv_name_len = 0;

        return Encoder::Encode(header, res, MSG_RESPONSE);
    }
};


const int MAX_PACKAGE_LEN = 2048;

class RpcClient
{

private:
    static int fd_;
    static uint64_t request_id_;
    // 这里最好设计成 {bool , string}
    static std::unordered_map<uint64_t, std::promise<Response>> promiseMap_; // <request—id, promise>

    static std::mutex send_lock_;

public:
    // 此处实现一个Rpc用户端读循环，用于不断从io流读取数据
    static void ReadLoop() {
        int port = 8080;

        {
            std::lock_guard<std::mutex> lock(send_lock_); 
            fd_ = Dial(port);
        }
        
        int save_errno;
        ProtocolHeader header;
        int header_len = sizeof(header);
        int pkg_len = -1;
        std::vector<uint8_t> data(MAX_PACKAGE_LEN);

        RingBuffer buf(1024);

        // 不断读取消息
        while (true) {
            int len = buf.read_fd(fd_, &save_errno);

            // 读取了数据
            if (len > 0) {
                if (pkg_len == -1 && buf.readable_bytes() >= header_len) {
                    buf.peek_data(&header, header_len);
                    pkg_len = header_len + header.body_len + header.srv_name_len;
                }

                // 可以读数据
                if (buf.readable_bytes() >= pkg_len) {
                    buf.get_package_data(data.data(), pkg_len);
                    
                    std::string body;
                    bool is_success = Decoder::Decode(data, header, body);

                    // 成功了的话将结果返回回去
                    assert(promiseMap_.find(header.request_id) != promiseMap_.end() && "request id not exists.");

                    promiseMap_[header.request_id].set_value({is_success, body});
                }
            } // 连接断开，尝试重连
            else if (len == 0) {
                std::lock_guard<std::mutex> lock(send_lock_); 

                
                close(fd_);
                size_t retry_cnt = 0;
                while (retry_cnt < 5) {
                    try
                    {
                        fd_ = Dial(port);
                        LOG_ERROR("Retry Connect success fd=%d", fd_);
                        break;
                    }
                    catch(const std::exception& e)
                    {
                        LOG_ERROR("Retry Connect failed: %s, id=%lu", e.what(), retry_cnt);
                        std::this_thread::sleep_for(std::chrono::seconds(3));
                    }
                    ++retry_cnt;
                }

                // 重连超过5次直接退出
                if (retry_cnt == 5) {
                    LOG_FATAL("Retry Connect failed %lu times, System exit", retry_cnt);
                }
            }
            else {
                    // 3. 发生错误，需要检查 errno
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // 3.1 正常情况：非阻塞模式下暂时无数据，等待下次再试。
                } else if (errno == EINTR) {
                    // 3.2 正常情况：被信号中断，应该立即重试 read。
                } else {
                    // 3.3 异常情况：发生了真正的错误（如 ECONNRESET, EPIPE 等）。
                    // 应该记录错误日志，并关闭连接，清理资源。
                    // perror("read error");
                    close(fd_);
                    LOG_FATAL("read error");
                }
            }
        }
    }

    static bool send(std::vector<uint8_t>& bytes) {
        // 发送消息
        if (::send(fd_, bytes.data(), bytes.size(), 0) == -1) {
            uint64_t id = reinterpret_cast<ProtocolHeader*>(bytes.data())->request_id = request_id_;
            promiseMap_.erase(id);
            return false;
        }
        
        return true;
    }

    // static bool send(const std::vector<uint8_t>& bytes) {
    //     // 模拟发送消息
    //     auto bytes_recv = RpcServerDemo::RecvMsg(bytes);

    //     ProtocolHeader header;
    //     std::string res;
    //     Decoder::Decode(bytes_recv, header, res);

    //     if (header.request_id != 200) {
    //         return false;
    //     }
        
    //     return true;
    // }
public:
    template<class R, typename ...Args>
    static bool call(const std::string& srvName, const std::tuple<Args...>& args, R& ret) {

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
        Response r = f.get();

        if (r.state) {
            ret = Serialize::Deserialization<R>(r.data);
        }

        return r.state;
    }

    template<typename ...Args>
    static bool call(const std::string& srvName, const std::tuple<Args...>& args) {
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