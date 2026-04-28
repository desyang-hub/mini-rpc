#pragma once


#include "minirpc/common/Type.h"
#include "minirpc/common/logger.h"
#include "minirpc/common/functioin_traits.h"
#include "minirpc/common/tuple_helper.h"
#include "minirpc/protocol/Serialize.h"
#include "minirpc/protocol/Decoder.h"
#include "minirpc/protocol/Encoder.h"
#include "minirpc/core/macro/rpc_service_bind.h"


#include <iostream>
#include <unordered_map>
#include <utility>
#include <assert.h>
#include <stdexcept>
#include <shared_mutex>


namespace minirpc
{

class RpcServer
{
private:
    std::shared_mutex hanelers_mutex_;
    std::unordered_map<std::string, RpcHandler> handlers_;

    std::vector<std::string> services_;

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
    static void RegisterService(const std::string& clsNmae) {
        GetInstance().registerService(clsNmae);
    }

    void registerService(const std::string& clsNmae);

    static std::vector<std::string>& GetServices() {
        return GetInstance().getServices();
    }

    std::vector<std::string>& getServices() {
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

        using param_type = typename function_traits<DecayFunc>::args_tuple;
        // 去除引用
        using T = typename std::decay<param_type>::type;

        using return_type = typename function_traits<DecayFunc>::return_type;
        using R = typename std::remove_reference<return_type>::type;


        // 使用写锁
        std::unique_lock<std::shared_mutex> lock(hanelers_mutex_);

        // std::cout << handlers_.size() << std::endl;

        // 确保注册的名称是不一样的，为了确保多重不一致，可以选择将域也作为名称前缀
        // assert(handlers_.count(name) == 0 && "RpcServer name ambiguity.");

        handlers_[name] = [f = std::forward<Func>(fun)](const std::string &body, std::string &res)
        {
            // 反序列化
            auto param = Serialize::Deserialization<T>(body);

            call_and_serialize(f, param, res);
        };

        LOG_INFO("RpcServer bind %s", name.c_str());
    }


    static bool Call(const std::string &name, const std::string &body, std::string &res) {
        return GetInstance().call(name, body, res);
    }

    bool call(const std::string &name, const std::string &body, std::string &res);
    
};

} // namespace minirpc