#pragma once

#include "minirpc/common/functioin_traits.h"
#include "minirpc/common/RpcException.h"
#include "minirpc/protocol/Serialize.h"
#include "minirpc/protocol/Encoder.h"
#include "minirpc/protocol/Decoder.h"
#include "minirpc/core/RpcServer.h"

#include <iostream>
#include <tuple>
#include <vector>

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



class RpcClient
{

private:
    
    template<class R>
    static bool send(const std::vector<uint8_t>& bytes, R& ret) {
        // 模拟发送消息
        auto bytes_recv = RpcServerDemo::RecvMsg(bytes);


        ProtocolHeader header;
        std::string res;
        Decoder::Decode(bytes_recv, header, res);

        if (header.request_id != 200) {
            return false;
        }

        try
        {
            ret = Serialize::Deserialization<R>(res);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            return false;
        }
        
        return true;
    }

    static bool send(const std::vector<uint8_t>& bytes) {
        // 模拟发送消息
        auto bytes_recv = RpcServerDemo::RecvMsg(bytes);

        ProtocolHeader header;
        std::string res;
        Decoder::Decode(bytes_recv, header, res);

        if (header.request_id != 200) {
            return false;
        }
        
        return true;
    }
public:
    template<class R, typename ...Args>
    static bool call(const std::string& srvName, const std::tuple<Args...>& args, R& ret) {
        std::string body = Serialize::Serialization(args);
        auto bytes = Encoder::Encode(srvName, body);

        if (!send(bytes, ret)) return false;

        return true;
    }

    template<typename ...Args>
    static bool call(const std::string& srvName, const std::tuple<Args...>& args) {
        std::string body = Serialize::Serialization(args);
        auto bytes = Encoder::Encode(srvName, body);

        if (!send(bytes)) return false;

        return true;
    }

};



} // namespace minirpc