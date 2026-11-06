// WinAPI++ 窗口与消息循环
//
// 设计要点（见 Plan0 §6.5）：
//   - WndProc 用 GWLP_USERDATA 绑定 this，未注册的消息走 DefWindowProcW。
//   - 消息处理器用运行时 unordered_map 注册（M0 决策 Q3）。
//   - GUI 层全部头文件内联，三形态行为一致，不需要导出符号。
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

#include <functional>
#include <string>
#include <unordered_map>
#include <utility>

namespace winpp {
namespace ui {

struct window_options {
    string_view   title;
    int           width  = 800;
    int           height = 600;
    unsigned long style  = WS_OVERLAPPEDWINDOW;
    bool          center = true;
};

class window {
public:
    typedef std::function<LRESULT(window&, UINT, WPARAM, LPARAM)> handler;

    window() noexcept : hwnd_(nullptr) {}
    window(window&& other) noexcept;
    window& operator=(window&& other) noexcept;
    window(const window&) = delete;
    window& operator=(const window&) = delete;
    ~window();

    static Result<window> create(const window_options& opts);

    bool valid() const noexcept;
    HWND native() const noexcept { return hwnd_; }

    void show(int cmd = SW_SHOW) noexcept;
    void hide() noexcept;
    void close() noexcept;
    void set_title(string_view title);

    // 注册 / 注销某条消息的处理器
    void on(UINT msg, handler h);
    void off(UINT msg);

    // 局部消息循环，直到收到 WM_QUIT
    int run();

private:
    static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    LRESULT dispatch(UINT msg, WPARAM wp, LPARAM lp);

    HWND hwnd_;
    std::unordered_map<UINT, handler> handlers_;
};

// 全局消息循环（不需要绑定某个窗口时使用）
int message_loop();

}  // namespace ui
}  // namespace winpp

// GUI 层实现全部内联，无条件包含
#include <winpp/detail/window.inl>
