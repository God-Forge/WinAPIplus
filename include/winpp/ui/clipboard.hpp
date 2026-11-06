// WinAPI++ 剪贴板
#pragma once

#include <winpp/compat/config.hpp>
#include <winpp/compat/export.hpp>
#include <winpp/compat/shims.hpp>
#include <winpp/core/error.hpp>
#include <winpp/core/result.hpp>

#include <string>

namespace winpp {
namespace ui {

WINPP_NODISCARD WINPP_API Result<std::string> clipboard_text();
WINPP_NODISCARD WINPP_API Result<void> set_clipboard_text(string_view text);

}  // namespace ui
}  // namespace winpp

#if defined(WINPP_HEADER_ONLY)
#  include <winpp/detail/clipboard.inl>
#endif
