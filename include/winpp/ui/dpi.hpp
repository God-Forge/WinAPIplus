// WinAPI++ DPI 感知
//
// 通过 GetProcAddress 动态加载新 API，老系统自动降级。
#pragma once

#include <winpp/compat/config.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <windows.h>

namespace winpp {
namespace ui {

// 开启进程级 DPI 感知（优先 Per-Monitor V2，失败降级为系统级）。
void enable_dpi_awareness();

// 取窗口 DPI；老系统回退到 96。
unsigned int dpi_for_window(HWND hwnd);

}  // namespace ui
}  // namespace winpp

#include <winpp/detail/dpi.inl>
