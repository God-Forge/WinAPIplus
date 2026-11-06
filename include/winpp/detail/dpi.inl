// 唯一实现源：DPI（全部 inline）
#include <winpp/ui/dpi.hpp>

// GetProcAddress 返回 FARPROC，转换成具体函数指针在 GCC 下会触发
// -Wcast-function-type。这是动态加载的标准做法，局部抑制。
#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wcast-function-type"
#endif

namespace winpp {
namespace ui {

inline void enable_dpi_awareness() {
    typedef BOOL(WINAPI * SetProcessDpiAwarenessContextFn)(HANDLE);
    typedef BOOL(WINAPI * SetProcessDPIAwareFn)();

    const HMODULE user32 = ::GetModuleHandleW(L"user32.dll");
    if (user32 != nullptr) {
        SetProcessDpiAwarenessContextFn fn = reinterpret_cast<SetProcessDpiAwarenessContextFn>(
            ::GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        if (fn != nullptr) {
            // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 = (HANDLE)-4
            if (fn(reinterpret_cast<HANDLE>(static_cast<INT_PTR>(-4)))) return;
        }
    }

    const HMODULE huser32 = ::LoadLibraryW(L"user32.dll");
    if (huser32 != nullptr) {
        SetProcessDPIAwareFn fn = reinterpret_cast<SetProcessDPIAwareFn>(
            ::GetProcAddress(huser32, "SetProcessDPIAware"));
        if (fn != nullptr) fn();
        ::FreeLibrary(huser32);
    }
}

inline unsigned int dpi_for_window(HWND hwnd) {
    typedef UINT(WINAPI * GetDpiForWindowFn)(HWND);

    const HMODULE user32 = ::GetModuleHandleW(L"user32.dll");
    if (user32 != nullptr) {
        GetDpiForWindowFn fn = reinterpret_cast<GetDpiForWindowFn>(
            ::GetProcAddress(user32, "GetDpiForWindow"));
        if (fn != nullptr && hwnd != nullptr) return fn(hwnd);
    }

    const HDC dc = ::GetDC(hwnd);
    if (dc == nullptr) return 96u;
    const unsigned int dpi = static_cast<unsigned int>(::GetDeviceCaps(dc, LOGPIXELSX));
    ::ReleaseDC(hwnd, dc);
    return dpi == 0 ? 96u : dpi;
}

}  // namespace ui
}  // namespace winpp

#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic pop
#endif
