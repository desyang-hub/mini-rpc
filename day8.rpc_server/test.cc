#include <vector>
#include <iostream>
#include <functional>
#include <unordered_map>
#include <utility>

#include "Encoder.h"
#include "Decoder.h"

#include "functioin_traits.h"

#include "Serialize.h"
#include "RpcServer.h"
#include "UserService.h"


int main(int argc, char const *argv[])
{

    std::cout <<  RpcServer::handlers_.size() << std::endl;

    auto add = [](const std::pair<int,int>& p) {
        return p.first + p.second;
    };

    // UserService uService;
    
    // 修复点：显式构造 std::function
    // 这样编译器就能根据参数类型 std::pair<int, int> 和返回类型 int 推导出模板参数
    std::function<int(const std::pair<int, int>&)> func(add);
    RpcServer::bind("add", add);

    // using MethodType = decltype(&UserService::Login);
    // using param_type = function_traits<MethodType>::param_type;
    // rpcServer.bind("UserService.Login", [&uService](param_type param) {
    //     return uService.Login(param);
    // });

    auto param = std::pair<int, int>{1, 2};

    auto res = RpcServer::call<int>("add", Serialize::Serialization(param));

    std::cout << res << std::endl;


    RpcServer::call("UserService.print", Serialize::Serialization(param));
    

    return 0;
}