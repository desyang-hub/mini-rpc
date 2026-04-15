#pragma once

#include <iostream>

#include "RpcServer.h"

// TODO: 我们来实现一个宏，这样的话，只要用户定义了服务类，那么就会自动将服务注册到中心，供程序调用

// 这里的服务端已经做好了，那么客户端呢？客户端的调用需要做成两种接口，同步调用接口和异步调用接口
/**
 * @idea: 还是模仿rpc server 注册的宏来实现客户端调用函数
 * 
 * 1. 该宏必须要读取类的服务函数，接着通过函数类型萃取函数参数生成供客户端调用的函数
 * 2. 拿到函数名和参数类型就好办了，直接组装函数名作为调用name，并将参数包装进body，发起rpc请求，接收到结果后返回。
 * 3. 如果连接可复用就更好了，不需要每次都进行重新连接
 */


class UserService
{
RPC_SERVICE_BIND(UserService, add, sub, print)
public:
    int add(const std::pair<int, int>& tuple) const {
        return tuple.first + tuple.second;
    }

    int sub(const std::pair<int, int>& tuple) const {
        return tuple.first - tuple.second;
    }

    void print(const std::pair<int, int>& tuple) const {
        std::cout << "Call print without return param" << std::endl;
        std::cout << (tuple.first + tuple.second) << std::endl;
    }
};