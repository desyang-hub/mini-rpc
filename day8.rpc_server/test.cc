#include <vector>
#include <iostream>
#include <functional>
#include <unordered_map>
#include <utility>
#include <tuple>

#include "Encoder.h"
#include "Decoder.h"

#include "functioin_traits.h"

#include "Serialize.h"
#include "RpcServer.h"
#include "UserService.h"

class DefaultService {
RPC_SERVICE_BIND(DefaultService, add);
public:
    int add(int a, int b) const {
        return a + b;
    }
};
RPC_SERVICE_REGISTER(DefaultService);


int main(int argc, char const *argv[])
{

    // auto add = [](const std::pair<int,int>& p) {
    //     return p.first + p.second;
    // };

    // 修复点：显式构造 std::function
    // 这样编译器就能根据参数类型 std::pair<int, int> 和返回类型 int 推导出模板参数
    // std::function<int(const std::pair<int, int>&)> func(add);
    // RpcServer::bind("add", add);

    std::pair<int, int> p{1, 2};
    auto str = Serialize::Serialization(p);
    auto p1 = Serialize::Deserialization<std::pair<int, int>>(str);
    std::cout << p1.first << " " << p1.second << std::endl;

    auto param = std::tuple<int, int>{1, 2};


    std::tuple<int, int> tp{1, 2};
    RpcServer::call<int>("DefaultService.add", Serialize::Serialization(tp));

    std::string body = Serialize::Serialization(param);

    std::cout << "Methods: " << RpcServer::handlers_.size() << std::endl;

    auto res = RpcServer::call<int>("UserService.add", body);
    std::cout << res << std::endl;

    std::cout << "Methods: " << RpcServer::handlers_.size() << std::endl;

    res = RpcServer::call<int>("UserService.sub", body);
    std::cout << res << std::endl;
    
    RpcServer::call("UserService.print", body);

    using param_type = std::tuple<int, int>;
    param_type param1{1, 2};

    std::string tuple_str = Serialize::Serialization(param1);
    auto param_decode = Serialize::Deserialization<param_type>(tuple_str);



    return 0;
}