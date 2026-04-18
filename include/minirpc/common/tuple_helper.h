#pragma once

#include <tuple>
#include <utility> // std::index_sequence, std::make_index_sequence
#include <iostream>

namespace minirpc
{

    // --- 步骤 1: 实现 apply 的核心逻辑 ---

    // 内部实现：利用索引序列 I... (例如 0, 1, 2) 来解包
    template <typename Func, typename Tuple, size_t... I>
    auto apply_impl(Func &&f, Tuple &&t, std::index_sequence<I...>)
        -> decltype(std::forward<Func>(f)(std::get<I>(std::forward<Tuple>(t))...))
    {
        // std::get<I>(t)... 会展开成 std::get<0>(t), std::get<1>(t), ...
        return std::forward<Func>(f)(std::get<I>(std::forward<Tuple>(t))...);
    }

    // 外部接口：自动推导索引序列
    template <typename Func, typename Tuple>
    auto rpc_apply(Func &&f, Tuple &&t)
        -> decltype(apply_impl(std::forward<Func>(f), std::forward<Tuple>(t),
                               std::make_index_sequence<std::tuple_size<typename std::decay<Tuple>::type>::value>{}))
    {
        // 生成 0 到 tuple_size-1 的索引序列
        using Indices = std::make_index_sequence<std::tuple_size<typename std::decay<Tuple>::type>::value>;
        return apply_impl(std::forward<Func>(f), std::forward<Tuple>(t), Indices{});
    }

} // namespace minirpc