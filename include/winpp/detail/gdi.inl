// 唯一实现源：GDI（全部 inline，三形态共用）
#include <winpp/gdi/dc.hpp>
#include <winpp/gdi/objects.hpp>
#include <winpp/core/string.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <windows.h>

namespace winpp {
namespace gdi {

// ---------------------------------------------------------------
// dc
// ---------------------------------------------------------------
inline dc::dc(dc&& other) noexcept
    : hdc_(other.hdc_), owner_(other.owner_), owns_(other.owns_) {
    other.hdc_ = nullptr;
    other.owner_ = nullptr;
    other.owns_ = false;
}

inline dc& dc::operator=(dc&& other) noexcept {
    if (this != &other) {
        release();
        hdc_ = other.hdc_;
        owner_ = other.owner_;
        owns_ = other.owns_;
        other.hdc_ = nullptr;
        other.owner_ = nullptr;
        other.owns_ = false;
    }
    return *this;
}

inline dc::~dc() {
    release();
}

inline void dc::release() noexcept {
    if (hdc_ == nullptr) return;
    if (owns_) {
        if (owner_ != nullptr) {
            ::ReleaseDC(owner_, hdc_);
        } else {
            ::DeleteDC(hdc_);
        }
    }
    hdc_ = nullptr;
    owner_ = nullptr;
    owns_ = false;
}

inline dc dc::borrow(HDC h) noexcept {
    dc d;
    d.hdc_ = h;
    d.owns_ = false;
    return d;
}

inline Result<dc> dc::from_window(HWND hwnd) {
    const HDC h = ::GetDC(hwnd);
    if (h == nullptr) return winpp::last_error("GetDC");

    dc d;
    d.hdc_ = h;
    d.owner_ = hwnd;
    d.owns_ = true;
    return d;
}

inline Result<dc> dc::create_compatible(HDC src) {
    const HDC h = ::CreateCompatibleDC(src);
    if (h == nullptr) return winpp::last_error("CreateCompatibleDC");

    dc d;
    d.hdc_ = h;
    d.owns_ = true;
    return d;
}

inline void dc::fill_rect(const RECT& rc, HBRUSH brush) noexcept {
    if (hdc_ != nullptr) ::FillRect(hdc_, &rc, brush);
}

inline void dc::text_out(int x, int y, string_view text) {
    if (hdc_ == nullptr) return;
    const winpp::wide_string w(text);
    ::TextOutW(hdc_, x, y, w.c_str(), static_cast<int>(w.str().size()));
}

inline void dc::text_rect(const RECT& rc, string_view text, unsigned format) {
    if (hdc_ == nullptr) return;
    const winpp::wide_string w(text);
    RECT r = rc;
    ::DrawTextW(hdc_, w.c_str(), static_cast<int>(w.str().size()), &r, format);
}

inline void dc::set_text_color(COLORREF color) noexcept {
    if (hdc_ != nullptr) ::SetTextColor(hdc_, color);
}

inline void dc::set_bk_mode(int mode) noexcept {
    if (hdc_ != nullptr) ::SetBkMode(hdc_, mode);
}

inline void dc::set_bk_color(COLORREF color) noexcept {
    if (hdc_ != nullptr) ::SetBkColor(hdc_, color);
}

inline void dc::rectangle(const RECT& rc) noexcept {
    if (hdc_ != nullptr) ::Rectangle(hdc_, rc.left, rc.top, rc.right, rc.bottom);
}

inline void dc::line(int x1, int y1, int x2, int y2) noexcept {
    if (hdc_ == nullptr) return;
    ::MoveToEx(hdc_, x1, y1, nullptr);
    ::LineTo(hdc_, x2, y2);
}

inline void dc::ellipse(const RECT& rc) noexcept {
    if (hdc_ != nullptr) ::Ellipse(hdc_, rc.left, rc.top, rc.right, rc.bottom);
}

// ---------------------------------------------------------------
// back_buffer
// ---------------------------------------------------------------
inline back_buffer::back_buffer() noexcept
    : memdc_(nullptr), bitmap_(nullptr), old_bitmap_(nullptr),
      target_(nullptr), width_(0), height_(0) {}

inline back_buffer::back_buffer(back_buffer&& other) noexcept
    : memdc_(other.memdc_), bitmap_(other.bitmap_), old_bitmap_(other.old_bitmap_),
      target_(other.target_), width_(other.width_), height_(other.height_) {
    other.memdc_ = nullptr;
    other.bitmap_ = nullptr;
    other.old_bitmap_ = nullptr;
    other.target_ = nullptr;
    other.width_ = 0;
    other.height_ = 0;
}

inline back_buffer& back_buffer::operator=(back_buffer&& other) noexcept {
    if (this != &other) {
        destroy();
        memdc_ = other.memdc_;
        bitmap_ = other.bitmap_;
        old_bitmap_ = other.old_bitmap_;
        target_ = other.target_;
        width_ = other.width_;
        height_ = other.height_;
        other.memdc_ = nullptr;
        other.bitmap_ = nullptr;
        other.old_bitmap_ = nullptr;
        other.target_ = nullptr;
        other.width_ = 0;
        other.height_ = 0;
    }
    return *this;
}

inline back_buffer::~back_buffer() {
    destroy();
}

inline void back_buffer::destroy() noexcept {
    if (memdc_ != nullptr) {
        if (old_bitmap_ != nullptr) ::SelectObject(memdc_, old_bitmap_);
        ::DeleteDC(memdc_);
    }
    if (bitmap_ != nullptr) ::DeleteObject(bitmap_);
    memdc_ = nullptr;
    bitmap_ = nullptr;
    old_bitmap_ = nullptr;
    target_ = nullptr;
    width_ = 0;
    height_ = 0;
}

inline Result<back_buffer> back_buffer::create(HDC target, int width, int height) {
    if (target == nullptr || width <= 0 || height <= 0) {
        return winpp::error(87u, "back_buffer::create");  // ERROR_INVALID_PARAMETER
    }

    const HDC mem = ::CreateCompatibleDC(target);
    if (mem == nullptr) return winpp::last_error("CreateCompatibleDC");

    const HBITMAP bmp = ::CreateCompatibleBitmap(target, width, height);
    if (bmp == nullptr) {
        ::DeleteDC(mem);
        return winpp::last_error("CreateCompatibleBitmap");
    }

    const HGDIOBJ old = ::SelectObject(mem, bmp);

    back_buffer bb;
    bb.memdc_ = mem;
    bb.bitmap_ = bmp;
    bb.old_bitmap_ = old;
    bb.target_ = target;
    bb.width_ = width;
    bb.height_ = height;
    return bb;
}

inline void back_buffer::flush(int x, int y) noexcept {
    if (memdc_ == nullptr || target_ == nullptr) return;
    ::BitBlt(target_, x, y, width_, height_, memdc_, 0, 0, SRCCOPY);
}

// ---------------------------------------------------------------
// GDI 对象工厂
// ---------------------------------------------------------------
inline Result<pen> create_pen(COLORREF color, int width, int style) {
    const HPEN h = ::CreatePen(style, width, color);
    if (h == nullptr) return winpp::last_error("CreatePen");
    return pen(h);
}

inline Result<brush> create_solid_brush(COLORREF color) {
    const HBRUSH h = ::CreateSolidBrush(color);
    if (h == nullptr) return winpp::last_error("CreateSolidBrush");
    return brush(h);
}

inline Result<font> create_font(string_view face, int height, int weight) {
    const winpp::wide_string wface(face);
    const HFONT h = ::CreateFontW(
        height, 0, 0, 0, weight, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, wface.c_str());
    if (h == nullptr) return winpp::last_error("CreateFontW");
    return font(h);
}

}  // namespace gdi
}  // namespace winpp
