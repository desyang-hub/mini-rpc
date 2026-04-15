#pragma once

#include <functional>
#include <tuple>

template<typename T>
struct function_traits;


// ============================================================
// 🛠️ 新增：专门处理参数本身就是 std::tuple 的情况
// 目的：防止 tuple<int, int> 变成 tuple<tuple<int, int>>
// ============================================================
template<typename C, typename R, typename... Args>
struct function_traits<R(C::*)(std::tuple<Args...>) const> {
    using return_type = R;
    // 直接使用原本的 tuple，不再次包装
    using args_tuple = std::tuple<Args...>; 
};

// 版本 1：匹配 const 引用 (Lambda 最常见的形式)
template<typename C, typename R, typename... Args>
struct function_traits<R(C::*)(const std::tuple<Args...>&) const> {
    using return_type = R;
    using args_tuple = std::tuple<Args...>; // 直接解包，不再次包装
};

// template<class R, class T>
// struct function_traits<R(*)(T)> {
//     using return_type = R;
//     using param_type = typename std::decay<T>::type;
//     using args_tuple = std::tuple<param_type>;
// };

// 4. 处理普通函数指针
template<typename R, typename... Args>
struct function_traits<R(*)(Args...)> {
    using return_type = R;
    using args_tuple = std::tuple<std::decay_t<Args>...>;
};

// template<class R, class T>
// struct function_traits<std::function<R(T)>> {
//     using return_type = R;
//     using param_type = typename std::decay<T>::type;
//     using args_tuple = std::tuple<param_type>;
// };


// 特化：处理成员函数指针 R(C::*)(Arg)
// template <typename C, typename R, typename T>
// struct function_traits<R (C::*)(T)> {
//     using return_type = R;
//     using param_type = typename std::decay<T>::type;
//     using args_tuple = std::tuple<param_type>;
// };

// 3. 处理成员函数 (const) - Lambda 的 operator() 通常是 const 的
template<typename ClassType, typename R, typename... Args>
struct function_traits<R(ClassType::*)(Args...)> {
    using return_type = R;
    using args_tuple = std::tuple<std::decay_t<Args>...>;
};

// 3. Lambda / 仿函数 (处理 const 成员函数)
// template<typename C, typename R, typename T>
// struct function_traits<R(C::*)(T) const> {
//     using return_type = R;
//     using param_type = typename std::decay<T>::type;
//     using args_tuple = std::tuple<param_type>;
// };

// 2. 处理成员函数 (非 const)
template<typename ClassType, typename R, typename... Args>
struct function_traits<R(ClassType::*)(Args...) const> {
    using return_type = R;
    // 使用 std::decay_t 去除引用和 const，确保得到纯类型
    using args_tuple = std::tuple<std::decay_t<Args>...>;
};

// 针对 Lambda (通过推导其 operator())
template<typename T>
struct function_traits : public function_traits<decltype(&T::operator())> {};

// 辅助：获取第 N 个参数的类型
template<size_t N, typename Function>
using arg_type = typename std::tuple_element<N, typename function_traits<Function>::param_type>::type;