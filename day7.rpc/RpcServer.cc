#include <vector>
#include <iostream>
#include <functional>
#include <unordered_map>
#include <utility>

#include "Encoder.h"
#include "Decoder.h"

// #include "UserService.h"

#include "Serialize.h"

using Bytes = std::vector<uint8_t>;
using RpcHandler = std::function<void(const std::string&, std::string&)>;


// ServiceMap（哈希表），Key是"服务名.方法名"，Value是函数指针/回调

// void server() {



// }

// void request_mapper(const Bytes& request, Bytes& response) {
//     // 1. 解析协议
//     // 2. 解析body
//     // 3. call func
//     // 4. 将结果封装后写入到reponse中

//     std::string body;
//     ProtocolHeader header;
//     bool is_success = Decoder::Decode(request, header, body); // body解析成字符串，接着字符串由func内部自行解析成想要的数据

//     // 查找注册的服务程序
    


//     // 拿到程序调用结果


// }


template<typename T>
struct function_traits;

template<class R, class T>
struct function_traits<R(*)(T)> {
    using return_type = R;
    using param_type = T;
};

template<class R, class T>
struct function_traits<std::function<R(T)>> {
    using return_type = R;
    using param_type = T;
};


// 3. Lambda / 仿函数 (处理 const 成员函数)
template<typename C, typename R, typename T>
struct function_traits<R(C::*)(T) const> {
    using return_type = R;
    using param_type = T;
};

// 针对 Lambda (通过推导其 operator())
template<typename T>
struct function_traits : public function_traits<decltype(&T::operator())> {};


using Handler = std::function<void(const std::string&, std::string&)>;

class RpcServer
{
private:
    std::unordered_map<std::string, Handler> handlers_;
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

    template<class Func>
    void bind(const std::string& name, Func&& fun) {
        
        using DecayFunc = typename std::decay<Func>::type;

        using param_type = typename function_traits<DecayFunc>::param_type;
        // 去除引用
        using T = typename std::remove_reference<param_type>::type;

        using R = typename function_traits<DecayFunc>::return_type;

        handlers_[name] = [f = std::forward<Func>(fun)](const std::string& body, std::string& res) {
            // 反序列化
            auto param = Serialize::Deserialization<T>(body);
            res = Serialize::Serialization(f(param));
        };
    }

    template<class T>
    T call(const std::string& name, const std::string& body) const {
        std::string res;
        handlers_.find(name)->second(body, res);

        return Serialize::Deserialization<T>(res);
    }
};



int main(int argc, char const *argv[])
{

    auto add = [](const std::pair<int,int>& p) {
        return p.first + p.second;
    };

    RpcServer rpcServer;
    
    // 修复点：显式构造 std::function
    // 这样编译器就能根据参数类型 std::pair<int, int> 和返回类型 int 推导出模板参数
    std::function<int(const std::pair<int, int>&)> func(add);
    rpcServer.bind("add", add);

    auto param = std::pair<int, int>{1, 2};

    auto res = rpcServer.call<int>("add", Serialize::Serialization(param));

    std::cout << res << std::endl;
    

    return 0;
}