// WinAPI++ 环境变量
#pragma once

#include <winpp/compat/config.hpp>
#include <winpp/compat/export.hpp>
#include <winpp/compat/shims.hpp>
#include <winpp/core/error.hpp>
#include <winpp/core/result.hpp>

#include <string>

namespace winpp {
namespace sys {

WINPP_NODISCARD WINPP_API Result<std::string> get_env(string_view name);
WINPP_NODISCARD WINPP_API Result<void> set_env(string_view name, string_view value);
WINPP_NODISCARD WINPP_API Result<void> unset_env(string_view name);

// 展开 %VAR% 形式的环境变量引用
WINPP_NODISCARD WINPP_API std::string expand_env(string_view input);

}  // namespace sys
}  // namespace winpp

#if defined(WINPP_HEADER_ONLY)
#  include <winpp/detail/env.inl>
#endif
