// WinAPI++ 路径工具
#pragma once

#include <winpp/compat/config.hpp>
#include <winpp/compat/export.hpp>
#include <winpp/compat/shims.hpp>
#include <winpp/core/error.hpp>
#include <winpp/core/result.hpp>

#include <string>

namespace winpp {
namespace fs {

// 拼接两段路径，自动补反斜杠；b 为绝对路径时直接返回 b。
WINPP_NODISCARD WINPP_API std::string join(string_view a, string_view b);

// 展开为绝对路径（GetFullPathNameW）。
WINPP_NODISCARD WINPP_API Result<std::string> full_path(string_view p);

// 文件或目录是否存在。
WINPP_NODISCARD WINPP_API bool exists(string_view p);

// 删除文件。
WINPP_NODISCARD WINPP_API Result<void> remove(string_view p);

// 取系统已知文件夹（CSIDL_APPDATA / CSIDL_LOCAL_APPDATA / CSIDL_PROFILE 等）。
WINPP_NODISCARD WINPP_API Result<std::string> known_folder(int csidl);

}  // namespace fs
}  // namespace winpp

#if defined(WINPP_HEADER_ONLY)
#  include <winpp/detail/path.inl>
#endif
