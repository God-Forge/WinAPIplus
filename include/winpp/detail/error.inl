// 唯一实现源：error 相关
// header-only 模式由 error.hpp 尾部 include；库模式由 src/error.cpp include。
#include <winpp/core/error.hpp>
#include <winpp/core/string.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <windows.h>

#include <string>

namespace winpp {

WINPP_INLINE std::string format_message(unsigned long code) {
    wchar_t buf[512];
    const DWORD n = ::FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        static_cast<DWORD>(code),
        0,
        buf,
        static_cast<DWORD>(sizeof(buf) / sizeof(buf[0])),
        nullptr);

    if (n == 0) {
        return "未知错误（Win32 错误码 " + std::to_string(code) + "）";
    }

    std::wstring w(buf, n);
    while (!w.empty() && (w.back() == L'\r' || w.back() == L'\n')) {
        w.pop_back();
    }
    return to_utf8(wstring_view(w.data(), w.size()));
}

WINPP_INLINE error last_error(string_view context) noexcept {
    return error(static_cast<unsigned long>(::GetLastError()), context);
}

}  // namespace winpp
