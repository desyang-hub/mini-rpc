#pragma once

#include <iostream>

template<class T>
struct double_param
{
    T param1;
    T param2;
};

// TODO: 我们来实现一个宏，这样的话，只要用户定义了服务类，那么就会自动将服务注册到中心，供程序调用

class UserService
{
public:
    int add(const std::string& request, std::string& response) const {
        double_param<int> params;
    }

    int sub(const double_param<int>& params) const {
        return params.param1 - params.param2;
    }
};