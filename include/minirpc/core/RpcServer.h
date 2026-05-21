#pragma once


#include "minirpc/common/Type.h"
#include "minirpc/common/logger.h"
#include "minirpc/common/function_traits.h"
#include "minirpc/common/tuple_helper.h"
#include "minirpc/common/ThreadPool.h"
#include "minirpc/protocol/Serialize.h"
#include "minirpc/protocol/Decoder.h"
#include "minirpc/protocol/Encoder.h"
#include "minirpc/core/macro/rpc_service_bind.h"


#include <unordered_map>
#include <utility>
#include <cassert>
#include <stdexcept>
#include <shared_mutex>


namespace minirpc
{

class RpcServer
{
private:
    std::shared_mutex handlers_mutex_;
    std::unordered_map<std::string, RpcHandler> handlers_;

    std::vector<std::string> services_;

    ThreadPool pool_;

    // 辅助函数：处理返回值为 void 的情况
    template <typename F, typename T>
    static typename std::enable_if<std::is_void<typename std::result_of<F(T)>::type>::value>::type
    call_and_serialize(F f, T param, std::string &res)
    {
        f(param);
        // void 返回值不需要序列化，res 保持不变或置空
    }

    // 辅助函数：处理返回值非 void 的情况
    template <typename F, typename T>
    static typename std::enable_if<!std::is_void<typename std::result_of<F(T)>::type>::value>::type
    call_and_serialize(F f, T param, std::string &res)
    {
        res = Serialize::Serialization(f(param));
    }

    static RpcServer& GetInstance() {
        static RpcServer server;
        return server;
    }
public:
    static void RegisterService(const std::string& className) {
        GetInstance().registerService(className);
    }

    void registerService(const std::string& className);

    static const std::vector<std::string>& GetServices() {
        return GetInstance().getServices();
    }

    const std::vector<std::string>& getServices() const {
        return services_;
    }

    template <class Func>
    static void Bind(const std::string &name, Func &&fun)
    {
        GetInstance().bind(name, std::forward<Func>(fun));
    }

    template <class Func>
    void bind(const std::string &name, Func &&fun)
    {
        using DecayFunc = typename std::decay<Func>::type;
        using return_type = typename function_traits<DecayFunc>::return_type;

        if constexpr (function_traits<DecayFunc>::is_single_arg) {
            using param_type = typename function_traits<DecayFunc>::first_arg;
            using T = typename std::decay<param_type>::type;

            std::unique_lock<std::shared_mutex> lock(handlers_mutex_);
            handlers_[name] = [f = std::forward<Func>(fun)](const std::string &body, std::string &res)
            {
                auto param = Serialize::Deserialization<T>(body);
                call_and_serialize(f, param, res);
            };
        } else {
            using param_type = typename function_traits<DecayFunc>::args_tuple;
            using T = typename std::decay<param_type>::type;

            std::unique_lock<std::shared_mutex> lock(handlers_mutex_);
            handlers_[name] = [f = std::forward<Func>(fun)](const std::string &body, std::string &res)
            {
                auto param = Serialize::Deserialization<T>(body);
                call_and_serialize(f, param, res);
            };
        }

        LOG_INFO("RpcServer bind %s", name.c_str());
    }


    static bool Call(const std::string &name, const std::string &body, std::string &res) {
        
        return GetInstance().call(name, body, res);
    }

    bool call(const std::string &name, const std::string &body, std::string &res);
    
};


// 辅助模板函数
template<typename Class, typename MethodPtr>
inline void bind_rpc_method_impl(const std::string& name, MethodPtr method_ptr) {
    using traits = minirpc::function_traits<MethodPtr>;
    using return_type = typename traits::return_type;

    auto& instance = Class::GetInstance();
    auto instancePtr = &instance;

    if constexpr (traits::is_single_arg) {
        using arg_type = typename traits::first_arg;
        minirpc::RpcServer::Bind(
            name, 
            [instancePtr, method_ptr](arg_type arg) -> return_type {
                if constexpr (std::is_void_v<return_type>) {
                    (instancePtr->*method_ptr)(std::forward<arg_type>(arg));
                } else {
                    return (instancePtr->*method_ptr)(std::forward<arg_type>(arg));
                }
            }
        );
    } else {
        using param_tuple = typename traits::args_tuple;
        minirpc::RpcServer::Bind(
            name,
            [instancePtr, method_ptr](const param_tuple& received_tuple) -> return_type {
                if constexpr (std::is_void_v<return_type>) {
                    minirpc::rpc_apply([instancePtr, method_ptr](auto&&... args) {
                        (instancePtr->*method_ptr)(std::forward<decltype(args)>(args)...);
                    }, received_tuple);
                } else {
                    return minirpc::rpc_apply([instancePtr, method_ptr](auto&&... args) -> return_type {
                        return (instancePtr->*method_ptr)(std::forward<decltype(args)>(args)...);
                    }, received_tuple);
                }
            }
        );
    }
}

} // namespace minirpc