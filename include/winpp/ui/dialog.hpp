// WinAPI++ 简单对话框
#pragma once

#include <winpp/compat/config.hpp>
#include <winpp/compat/shims.hpp>
#include <winpp/core/string.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <windows.h>

namespace winpp {
namespace ui {

// 消息框。返回值语义与 MessageBoxW 一致（IDOK / IDCANCEL / IDYES 等）。
inline int message_box(HWND owner, string_view text, string_view caption,
                       unsigned int type = MB_OK | MB_ICONINFORMATION) {
    const winpp::wide_string wtext(text);
    const winpp::wide_string wcaption(caption);
    return static_cast<int>(::MessageBoxW(owner, wtext.c_str(), wcaption.c_str(), type));
}

}  // namespace ui
}  // namespace winpp
