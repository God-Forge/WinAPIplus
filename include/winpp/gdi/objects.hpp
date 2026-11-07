// WinAPI++ GDI 对象（画笔 / 画刷 / 字体）
#pragma once

#include <winpp/compat/config.hpp>
#include <winpp/compat/shims.hpp>
#include <winpp/core/error.hpp>
#include <winpp/core/handle.hpp>
#include <winpp/core/result.hpp>
#include <winpp/core/traits.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <windows.h>

namespace winpp {
namespace gdi {

typedef winpp::pen_handle   pen;
typedef winpp::brush_handle brush;
typedef winpp::font_handle  font;

WINPP_NODISCARD Result<pen>   create_pen(COLORREF color, int width = 1, int style = PS_SOLID);
WINPP_NODISCARD Result<brush> create_solid_brush(COLORREF color);
WINPP_NODISCARD Result<font>  create_font(string_view face, int height,
                                          int weight = FW_NORMAL);

// 把 GDI 对象选进 DC，析构时自动恢复原对象
class object_selector {
public:
    object_selector(HDC hdc, HGDIOBJ obj) noexcept : hdc_(hdc), old_(nullptr) {
        if (hdc_ != nullptr) old_ = ::SelectObject(hdc_, obj);
    }

    ~object_selector() {
        if (hdc_ != nullptr && old_ != nullptr) ::SelectObject(hdc_, old_);
    }

    object_selector(const object_selector&) = delete;
    object_selector& operator=(const object_selector&) = delete;

    HGDIOBJ old() const noexcept { return old_; }

private:
    HDC     hdc_;
    HGDIOBJ old_;
};

}  // namespace gdi
}  // namespace winpp
