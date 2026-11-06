// WinAPI++ 错误传播宏
//
// WINPP_TRY(var, expr)：
//   执行 expr（必须返回 Result<T>）；
//   失败则把 error 直接 return 给调用者（依赖 Result<T> 的隐式构造）；
//   成功则把值移动构造到 var。
//
// 用法：
//   winpp::Result<int> twice() {
//       WINPP_TRY(v, parse_number("42"));
//       return winpp::ok(v * 2);
//   }
#pragma once

#include <winpp/core/result.hpp>

#include <utility>

#define WINPP_TRY_CONCAT_(a, b) a##b
#define WINPP_TRY_CONCAT(a, b) WINPP_TRY_CONCAT_(a, b)

#define WINPP_TRY(var, expr)                                              \
    auto WINPP_TRY_CONCAT(winpp_try_, __LINE__) = (expr);                 \
    if (!WINPP_TRY_CONCAT(winpp_try_, __LINE__))                          \
        return WINPP_TRY_CONCAT(winpp_try_, __LINE__).error();            \
    auto var = std::move(WINPP_TRY_CONCAT(winpp_try_, __LINE__).value())

// Result<void> 没有值可绑定，用这个只做短路传播。
#define WINPP_TRY_VOID(expr)                                              \
    auto WINPP_TRY_CONCAT(winpp_try_, __LINE__) = (expr);                 \
    if (!WINPP_TRY_CONCAT(winpp_try_, __LINE__))                          \
        return WINPP_TRY_CONCAT(winpp_try_, __LINE__).error()
