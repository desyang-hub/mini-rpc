#pragma once

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
    using traits_##Method = minirpc::function_traits<MethodType_##Method>; \
    using ReturnType_##Method = typename traits_##Method::return_type; \
    template<typename... Args> \
    ReturnType_##Method Method(Args&&... args) { \
        return minirpc::RpcClient::GetInstance().Call<ReturnType_##Method>(#Class, #Class "." #Method, std::forward<Args>(args)...); \
    }



// 2. 参数计数器（self-contained — 不依赖 bind.h）
#define _STUB_PP_NARG(...) \
    _STUB_PP_NARG_(__VA_ARGS__, _STUB_PP_RSEQ_N())
#define _STUB_PP_NARG_(...) \
    _STUB_PP_ARG_N(__VA_ARGS__)
#define _STUB_PP_ARG_N(                                     \
    _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,              \
    _11, _12, _13, _14, _15, _16, _17, _18, _19, _20,     \
    N, ...) N
#define _STUB_PP_RSEQ_N()                         \
    20, 19, 18, 17, 16, 15, 14, 13, 12, 11,       \
    10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

// 3. 定义不同参数数量的实现宏
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

#define _RPC_STUB_IMPL_6(Class, M1, M2, M3, M4, M5, M6) \
    _RPC_STUB_METHOD(Class, M1) \
    _RPC_STUB_METHOD(Class, M2) \
    _RPC_STUB_METHOD(Class, M3) \
    _RPC_STUB_METHOD(Class, M4) \
    _RPC_STUB_METHOD(Class, M5) \
    _RPC_STUB_METHOD(Class, M6)

#define _RPC_STUB_IMPL_7(Class, M1, M2, M3, M4, M5, M6, M7) \
    _RPC_STUB_METHOD(Class, M1) \
    _RPC_STUB_METHOD(Class, M2) \
    _RPC_STUB_METHOD(Class, M3) \
    _RPC_STUB_METHOD(Class, M4) \
    _RPC_STUB_METHOD(Class, M5) \
    _RPC_STUB_METHOD(Class, M6) \
    _RPC_STUB_METHOD(Class, M7)

#define _RPC_STUB_IMPL_8(Class, M1, M2, M3, M4, M5, M6, M7, M8) \
    _RPC_STUB_METHOD(Class, M1) \
    _RPC_STUB_METHOD(Class, M2) \
    _RPC_STUB_METHOD(Class, M3) \
    _RPC_STUB_METHOD(Class, M4) \
    _RPC_STUB_METHOD(Class, M5) \
    _RPC_STUB_METHOD(Class, M6) \
    _RPC_STUB_METHOD(Class, M7) \
    _RPC_STUB_METHOD(Class, M8)

#define _RPC_STUB_IMPL_9(Class, M1, M2, M3, M4, M5, M6, M7, M8, M9) \
    _RPC_STUB_METHOD(Class, M1) \
    _RPC_STUB_METHOD(Class, M2) \
    _RPC_STUB_METHOD(Class, M3) \
    _RPC_STUB_METHOD(Class, M4) \
    _RPC_STUB_METHOD(Class, M5) \
    _RPC_STUB_METHOD(Class, M6) \
    _RPC_STUB_METHOD(Class, M7) \
    _RPC_STUB_METHOD(Class, M8) \
    _RPC_STUB_METHOD(Class, M9)

#define _RPC_STUB_IMPL_10(Class, M1, M2, M3, M4, M5, M6, M7, M8, M9, M10) \
    _RPC_STUB_METHOD(Class, M1) \
    _RPC_STUB_METHOD(Class, M2) \
    _RPC_STUB_METHOD(Class, M3) \
    _RPC_STUB_METHOD(Class, M4) \
    _RPC_STUB_METHOD(Class, M5) \
    _RPC_STUB_METHOD(Class, M6) \
    _RPC_STUB_METHOD(Class, M7) \
    _RPC_STUB_METHOD(Class, M8) \
    _RPC_STUB_METHOD(Class, M9) \
    _RPC_STUB_METHOD(Class, M10)

#define _RPC_STUB_IMPL_11(Class, M1, M2, M3, M4, M5, M6, M7, M8, M9, M10, M11) \
    _RPC_STUB_METHOD(Class, M1) \
    _RPC_STUB_METHOD(Class, M2) \
    _RPC_STUB_METHOD(Class, M3) \
    _RPC_STUB_METHOD(Class, M4) \
    _RPC_STUB_METHOD(Class, M5) \
    _RPC_STUB_METHOD(Class, M6) \
    _RPC_STUB_METHOD(Class, M7) \
    _RPC_STUB_METHOD(Class, M8) \
    _RPC_STUB_METHOD(Class, M9) \
    _RPC_STUB_METHOD(Class, M10) \
    _RPC_STUB_METHOD(Class, M11)

#define _RPC_STUB_IMPL_12(Class, M1, M2, M3, M4, M5, M6, M7, M8, M9, M10, M11, M12) \
    _RPC_STUB_METHOD(Class, M1) \
    _RPC_STUB_METHOD(Class, M2) \
    _RPC_STUB_METHOD(Class, M3) \
    _RPC_STUB_METHOD(Class, M4) \
    _RPC_STUB_METHOD(Class, M5) \
    _RPC_STUB_METHOD(Class, M6) \
    _RPC_STUB_METHOD(Class, M7) \
    _RPC_STUB_METHOD(Class, M8) \
    _RPC_STUB_METHOD(Class, M9) \
    _RPC_STUB_METHOD(Class, M10) \
    _RPC_STUB_METHOD(Class, M11) \
    _RPC_STUB_METHOD(Class, M12)

// 4. 分发宏 (核心逻辑)
// 这里的逻辑是：_RPC_STUB_ALL(UserService, add, sub, print)
// PP_NARG(add, sub, print) 应该返回 3
// 然后拼接出 _RPC_STUB_IMPL_3(UserService, add, sub, print)

#define _RPC_STUB_ALL(Class, ...) \
    _RPC_STUB_DISPATCH(_RPC_STUB_IMPL_, Class, __VA_ARGS__)

// 这里的 N 是通过 PP_NARG 计算出来的方法数量
#define _RPC_STUB_DISPATCH(Func, Class, ...) \
    _RPC_STUB_DISPATCH_(_RPC_STUB_DISPATCH__, Func, _STUB_PP_NARG(__VA_ARGS__), Class, __VA_ARGS__)

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
        _RPC_STUB_ALL(Class, __VA_ARGS__) }; private:
    
