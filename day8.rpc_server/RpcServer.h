#pragma once

#include <vector>
#include <iostream>
#include <functional>
#include <unordered_map>
#include <utility>
#include <assert.h>

#include "Encoder.h"
#include "Decoder.h"

#include "Serialize.h"
#include "functioin_traits.h"


/**
 * @brief 这里实现了RpcServer服务自动注册
 * 
 * @micro: RPC_SERVICE_BIND(Class, ...)
 * 
 * @usage:  在类定义域内末尾添加 RPC_SERVICE_BIND(Class, Method1, Method2, ...)
 * 
 * 
 * @micro: RPC_SERVICE_REGISTER(Class)
 * @usage: 在.h 对应的.cpp文件中使用该宏才能够实现注册
 */




// ===========================================================
// --- 宏重载实现 ---
// ===========================================================

// 1. 参数计数器
#define PP_NARG(...) \
    PP_NARG_(__VA_ARGS__, PP_RSEQ_N())
#define PP_NARG_(...) \
    PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N( \
    _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
    _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
    N, ...) N
#define PP_RSEQ_N() \
    20, 19, 18, 17, 16, 15, 14, 13, 12, 11, \
    10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

// 2. 单个方法的注册逻辑
#define _RPC_BIND_METHOD(Class, Method) \
    do { \
        auto instance = Class::GetInstance(); \
        using MethodType = decltype(&Class::Method); \
        using param_type = typename function_traits<MethodType>::param_type; \
        using PureParamType = typename std::remove_reference<typename std::remove_const<param_type>::type>::type; \
        RpcServer::bind(#Class "." #Method, [&instance](param_type param) { \
            return instance.Method(param); \
        }); \
    } while(0)

// 3. 定义不同参数数量的实现宏
// 注意：这里的 N 代表总参数个数（包括 Class）
#define _RPC_BIND_IMPL_2(Class, M1) \
    _RPC_BIND_METHOD(Class, M1)

#define _RPC_BIND_IMPL_3(Class, M1, M2) \
    _RPC_BIND_METHOD(Class, M1); \
    _RPC_BIND_METHOD(Class, M2)

#define _RPC_BIND_IMPL_4(Class, M1, M2, M3) \
    _RPC_BIND_METHOD(Class, M1); \
    _RPC_BIND_METHOD(Class, M2); \
    _RPC_BIND_METHOD(Class, M3)

#define _RPC_BIND_IMPL_5(Class, M1, M2, M3, M4) \
    _RPC_BIND_METHOD(Class, M1); \
    _RPC_BIND_METHOD(Class, M2); \
    _RPC_BIND_METHOD(Class, M3); \
    _RPC_BIND_METHOD(Class, M4)

// 4. 分发宏
// 关键修改：直接将 Class 和 __VA_ARGS__ 一起传给计数器和分发器
// 4. 分发宏
// 关键修改：增加中间层强制展开 PP_NARG

#define _RPC_BIND_ALL(Class, ...) \
    _RPC_BIND_DISPATCH(_RPC_BIND_IMPL_, Class, __VA_ARGS__)

// 第一层：接收参数，调用计数器
#define _RPC_BIND_DISPATCH(Func, ...) \
    _RPC_BIND_DISPATCH_(_RPC_BIND_DISPATCH__, Func, PP_NARG(__VA_ARGS__), __VA_ARGS__)

// 第二层：拼接宏名，并传递所有参数
#define _RPC_BIND_DISPATCH_(CALLBACK, Func, N, ...) \
    CALLBACK(Func, N, __VA_ARGS__)

// 第三层：执行最终的拼接和调用
// 这里 Func 是 _RPC_BIND_IMPL_，N 是数字，__VA_ARGS__ 是参数列表
#define _RPC_BIND_DISPATCH__(Func, N, ...) \
    Func##N(__VA_ARGS__)


// 5. 最终使用的宏
#define RPC_SERVICE_BIND(Class, ...) \
private: \
    Class() { \
        std::cout << #Class " Init" << std::endl; \
    } \
    /* 核心修改：增加一个静态启动器类 */ \
    class _AutoInit { \
        public: \
            _AutoInit() { \
                Class::Init(); \
            } \
    }; \
    \
    /* 5. 定义一个全局静态变量，利用其构造函数自动触发 Init */ \
    /* 注意：这里利用宏拼接生成唯一的变量名，防止命名冲突 */ \
    static _AutoInit _auto_init_instance_##Class; \
public: \
    static Class& GetInstance() { \
        static Class cls; \
        return cls; \
    }   \
        \
    static void Init() {    \
        Class::GetInstance();   \
        _RPC_BIND_ALL(Class, __VA_ARGS__);  \
    } \
private:


// ===========================================================
// --- 辅助宏：连接两个 Token (修正版) ---
// ===========================================================
// 第一层：直接拼接
#define RPC_CONCAT_IMPL(A, B) A##B

// 第二层：强制展开参数后再拼接
// 如果不加这一层，直接传 RPC_CONCAT(A, B) 给其他宏时，A 和 B 不会被展开
#define RPC_CONCAT(A, B) RPC_CONCAT_IMPL(A, B)

// ===========================================================
// --- 注册宏 (修正版) ---
// ===========================================================
#define RPC_SERVICE_REGISTER(Class) \
    namespace { \
        /* 这里使用了 RPC_CONCAT，它会被正确展开为 _AutoInit_UserService */ \
        struct RPC_CONCAT(_AutoInit_, Class) { \
            RPC_CONCAT(_AutoInit_, Class)() { \
                Class::Init(); \
            } \
        }; \
        /* 这里同理，展开为 static _AutoInit_UserService g_auto_init_UserService; */ \
        static RPC_CONCAT(_AutoInit_, Class) RPC_CONCAT(g_auto_init_, Class); \
    }

// #define RPC_SERVICE_BIND(Class, ...) \
// private: \
//     Class() { std::cout << #Class " Init" << std::endl; } \
// public: \
//     static Class& GetInstance() { \
//         static Class cls; \
//         return cls; \
//     } \
//     static void Init() { \
//         GetInstance(); /* 确保单例被创建 */ \
//         _RPC_BIND_ONE(Class, __VA_ARGS__) /* 这里展开注册逻辑 */ \
//     }

// ===========================================================


// v2

// --- 宏定义 ---

// 1. 执行注册的辅助宏
// #define _RPC_BIND_ONE(Class, Method, ...) \
//     _RegisterMethod(#Class "." #Method, &Class::Method); \
//     _RPC_EXPAND_ARGS(Class, __VA_ARGS__)

// // 2. 递归控制宏
// #define _RPC_EXPAND_ARGS(Class, ...) \
//     _RPC_GET_ARG_2(Class, ##__VA_ARGS__)

// // 3. 参数提取宏
// #define _RPC_GET_ARG_2(Class, first, ...) \
//     _RPC_BIND_ONE(Class, first, ##__VA_ARGS__)

// // 4. 入口宏（放在类内部）
// #define RPC_SERVICE_BIND(Class, ...) \
// private: \
//     Class() { std::cout << #Class " 构造函数被调用" << std::endl; } \
//     \
//     static void _RegisterMethod(const std::string& name, void* ptr) { \
//         auto instance = GetInstance(); \
//         RpcServer::bind(name, std::bind(ptr, &instance)); \
//         std::cout << "注册成功: " << name << std::endl; \
//     } \
//     \
//     /* 核心修改：增加一个静态启动器类 */ \
//     class _AutoInit { \
//     public: \
//         _AutoInit() { \
//             Class::Init(); \
//         } \
//     }; \
//     \
//     /* 5. 定义一个全局静态变量，利用其构造函数自动触发 Init */ \
//     /* 注意：这里利用宏拼接生成唯一的变量名，防止命名冲突 */ \
//     static _AutoInit _auto_init_instance_##Class; \
//     \
// public: \
//     static Class& GetInstance() { \
//         static Class cls; \
//         return cls; \
//     } \
//     \
//     static void Init() { \
//         GetInstance(); \
//         _RPC_BIND_ONE(Class, __VA_ARGS__) \
//     }

// ===========================================================


using Bytes = std::vector<uint8_t>;
using RpcHandler = std::function<void(const std::string&, std::string&)>;
using Handler = std::function<void(const std::string&, std::string&)>;

class RpcServer
{
public:
    static std::unordered_map<std::string, Handler> handlers_;
public:
    // template<class R, class T>
    // void bind(const std::string& name, std::function<R(const T&)> f) {
    //     // body 是从用户端接收的数据
    //     handlers_[name] = [f](const std::string& body, std::string& res) {
    //         // 反序列化
    //         auto param = Serialize::Deserialization<T>(body);
    //         res = Serialize::Serialization(f(param));
    //     };
    // }

    // 辅助函数：处理返回值为 void 的情况
    template <typename F, typename T>
    static typename std::enable_if<std::is_void<typename std::result_of<F(T)>::type>::value>::type
    call_and_serialize(F f, T param, std::string& res) {
        f(param);
        // void 返回值不需要序列化，res 保持不变或置空
    }

    // 辅助函数：处理返回值非 void 的情况
    template <typename F, typename T>
    static typename std::enable_if<!std::is_void<typename std::result_of<F(T)>::type>::value>::type
    call_and_serialize(F f, T param, std::string& res) {
        res = Serialize::Serialization(f(param));
    }

    template<class Func>
    static void bind(const std::string& name, Func&& fun) {
        
        using DecayFunc = typename std::decay<Func>::type;

        using param_type = typename function_traits<DecayFunc>::param_type;
        // 去除引用
        using T = typename std::remove_reference<param_type>::type;

        using R = typename function_traits<DecayFunc>::return_type;

        // 确保注册的名称是不一样的，为了确保多重不一致，可以选择将域也作为名称前缀
        assert(handlers_.count(name) == 0 && "RpcServer name ambiguity.");

        handlers_[name] = [f = std::forward<Func>(fun)](const std::string& body, std::string& res) {
            // 反序列化
            auto param = Serialize::Deserialization<T>(body);

            call_and_serialize(f, param, res);
        };

        std::cout << "RpcServer bind " << name << std::endl;
    }

    template<class T>
    static T call(const std::string& name, const std::string& body) {
        std::string res;
        handlers_.find(name)->second(body, res);

        return Serialize::Deserialization<T>(res);
    }

    static void call(const std::string& name, const std::string& body) {
        std::string res;
        handlers_.find(name)->second(body, res);
    }

    template<typename ...Args>
    void func(Args&& ...args) {

    }
};