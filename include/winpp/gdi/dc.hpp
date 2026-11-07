// WinAPI++ GDI 设备上下文
//
// 设计要点：
//   - 窗口 DC 与内存 DC 统一由 dc 管理，析构时按来源释放。
//   - back_buffer 提供双缓冲，避免重绘闪烁。
#pragma once

#include <winpp/compat/config.hpp>
#include <winpp/compat/shims.hpp>
#include <winpp/core/error.hpp>
#include <winpp/core/result.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <windows.h>

namespace winpp {
namespace gdi {

class dc {
public:
    dc() noexcept : hdc_(nullptr), owner_(nullptr), owns_(false) {}
    explicit dc(HDC h) noexcept : hdc_(h), owner_(nullptr), owns_(false) {}

    dc(dc&& other) noexcept;
    dc& operator=(dc&& other) noexcept;
    dc(const dc&) = delete;
    dc& operator=(const dc&) = delete;
    ~dc();

    bool valid() const noexcept { return hdc_ != nullptr; }
    HDC  native() const noexcept { return hdc_; }

    // 借用外部 DC（如 BeginPaint 拿到的），析构不释放
    static dc borrow(HDC h) noexcept;

    // 窗口 DC（GetDC / ReleaseDC）；hwnd 传 nullptr 表示屏幕 DC
    static Result<dc> from_window(HWND hwnd);

    // 内存 DC（CreateCompatibleDC / DeleteDC）
    static Result<dc> create_compatible(HDC src);

    void release() noexcept;

    // ---- 绘制 ----
    void fill_rect(const RECT& rc, HBRUSH brush) noexcept;
    void text_out(int x, int y, string_view text);
    void text_rect(const RECT& rc, string_view text, unsigned format = DT_LEFT);
    void set_text_color(COLORREF color) noexcept;
    void set_bk_mode(int mode) noexcept;
    void set_bk_color(COLORREF color) noexcept;
    void rectangle(const RECT& rc) noexcept;
    void line(int x1, int y1, int x2, int y2) noexcept;
    void ellipse(const RECT& rc) noexcept;

private:
    HDC  hdc_;
    HWND owner_;
    bool owns_;
};

// 双缓冲：先画到内存位图，再一次性贴回目标 DC
class back_buffer {
public:
    back_buffer() noexcept;
    back_buffer(back_buffer&& other) noexcept;
    back_buffer& operator=(back_buffer&& other) noexcept;
    back_buffer(const back_buffer&) = delete;
    back_buffer& operator=(const back_buffer&) = delete;
    ~back_buffer();

    static Result<back_buffer> create(HDC target, int width, int height);

    bool valid() const noexcept { return memdc_ != nullptr; }
    HDC  native() const noexcept { return memdc_; }

    void flush(int x = 0, int y = 0) noexcept;

private:
    void destroy() noexcept;

    HDC     memdc_;
    HBITMAP bitmap_;
    HGDIOBJ old_bitmap_;
    HDC     target_;
    int     width_;
    int     height_;
};

}  // namespace gdi
}  // namespace winpp

#include <winpp/detail/gdi.inl>
