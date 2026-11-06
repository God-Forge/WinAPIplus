// WinAPI++ 托盘图标
#pragma once

#include <winpp/compat/config.hpp>
#include <winpp/compat/shims.hpp>
#include <winpp/core/error.hpp>
#include <winpp/core/result.hpp>
#include <winpp/core/string.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>

namespace winpp {
namespace ui {

class tray_icon {
public:
    tray_icon() noexcept;
    ~tray_icon();

    tray_icon(const tray_icon&) = delete;
    tray_icon& operator=(const tray_icon&) = delete;

    // 添加托盘图标。owner 接收 uCallbackMessage（WM_APP + 1）。
    Result<void> add(HWND owner, unsigned int id, string_view tooltip, HICON icon = nullptr);

    void remove();
    void set_tooltip(string_view tooltip);

    bool added() const noexcept { return added_; }

private:
    NOTIFYICONDATAW data_;
    bool            added_;
};

}  // namespace ui
}  // namespace winpp

#include <winpp/detail/tray.inl>
