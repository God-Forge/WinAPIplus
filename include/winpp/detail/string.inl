// 唯一实现源：UTF-8 <-> UTF-16 转换
// header-only 模式由 string.hpp 尾部 include；库模式由 src/string.cpp include。
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

WINPP_INLINE std::string to_utf8(wstring_view w) {
    if (w.empty()) return std::string();

    const int n = ::WideCharToMultiByte(
        CP_UTF8, 0,
        w.data(), static_cast<int>(w.size()),
        nullptr, 0, nullptr, nullptr);
    if (n <= 0) return std::string();

    std::string out(static_cast<std::size_t>(n), '\0');
    ::WideCharToMultiByte(
        CP_UTF8, 0,
        w.data(), static_cast<int>(w.size()),
        &out[0], n, nullptr, nullptr);
    return out;
}

WINPP_INLINE std::wstring to_wide(string_view u8) {
    if (u8.empty()) return std::wstring();

    const int n = ::MultiByteToWideChar(
        CP_UTF8, 0,
        u8.data(), static_cast<int>(u8.size()),
        nullptr, 0);
    if (n <= 0) return std::wstring();

    std::wstring out(static_cast<std::size_t>(n), L'\0');
    ::MultiByteToWideChar(
        CP_UTF8, 0,
        u8.data(), static_cast<int>(u8.size()),
        &out[0], n);
    return out;
}

}  // namespace winpp
