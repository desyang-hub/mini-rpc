#pragma once

#include <functional>

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


// 特化：处理成员函数指针 R(C::*)(Arg)
template <typename C, typename R, typename T>
struct function_traits<R (C::*)(T)> {
    using return_type = R;
    using param_type = T; // 直接提取参数类型
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