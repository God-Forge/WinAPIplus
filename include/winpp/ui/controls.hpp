// WinAPI++ 控件薄壳
//
// 只做创建、取文本、启停、显隐这类薄封装，不引入布局引擎。
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

#include <string>
#include <utility>

namespace winpp {
namespace ui {

class control {
public:
    control() noexcept : hwnd_(nullptr) {}
    control(control&& other) noexcept;
    control& operator=(control&& other) noexcept;
    control(const control&) = delete;
    control& operator=(const control&) = delete;
    ~control();

    bool valid() const noexcept;
    HWND native() const noexcept { return hwnd_; }

    void set_text(string_view text);
    std::string text() const;
    void enable(bool on);
    void show(int cmd = SW_SHOW);
    void hide() noexcept;

protected:
    void destroy() noexcept;
    HWND hwnd_;
};

class button : public control {
public:
    static Result<button> create(HWND parent, string_view text, int id,
                                 int x, int y, int w, int h);
};

class edit : public control {
public:
    static Result<edit> create(HWND parent, string_view text, int id,
                               int x, int y, int w, int h);
};

class listbox : public control {
public:
    static Result<listbox> create(HWND parent, int id, int x, int y, int w, int h);

    void        add_item(string_view text);
    int         count() const;
    std::string item(int index) const;
    int         selection() const;
    void        set_selection(int index);
};

}  // namespace ui
}  // namespace winpp

#include <winpp/detail/controls.inl>
