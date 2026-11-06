// 唯一实现源：环境变量
#include <winpp/sys/env.hpp>
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
namespace sys {

WINPP_INLINE Result<std::string> get_env(string_view name) {
    const winpp::wide_string wname(name);
    const DWORD need = ::GetEnvironmentVariableW(wname.c_str(), nullptr, 0);
    if (need == 0) {
        const DWORD err = ::GetLastError();
        if (err == ERROR_ENVVAR_NOT_FOUND) {
            return winpp::error(203u, "GetEnvironmentVariableW");  // ERROR_ENVVAR_NOT_FOUND
        }
        return winpp::error(static_cast<unsigned long>(err), "GetEnvironmentVariableW");
    }

    std::wstring buf(static_cast<std::size_t>(need), L'\0');
    const DWORD n = ::GetEnvironmentVariableW(wname.c_str(), &buf[0], need);
    if (n == 0) return winpp::last_error("GetEnvironmentVariableW");
    buf.resize(static_cast<std::size_t>(n));
    return winpp::to_utf8(winpp::wstring_view(buf.data(), buf.size()));
}

WINPP_INLINE Result<void> set_env(string_view name, string_view value) {
    const winpp::wide_string wname(name);
    const winpp::wide_string wvalue(value);
    if (!::SetEnvironmentVariableW(wname.c_str(), wvalue.c_str())) {
        return winpp::last_error("SetEnvironmentVariableW");
    }
    return Result<void>();
}

WINPP_INLINE Result<void> unset_env(string_view name) {
    const winpp::wide_string wname(name);
    if (!::SetEnvironmentVariableW(wname.c_str(), nullptr)) {
        return winpp::last_error("SetEnvironmentVariableW");
    }
    return Result<void>();
}

WINPP_INLINE std::string expand_env(string_view input) {
    const winpp::wide_string win(input);
    const DWORD need = ::ExpandEnvironmentStringsW(win.c_str(), nullptr, 0);
    if (need == 0) return std::string(input.data(), input.size());

    std::wstring buf(static_cast<std::size_t>(need), L'\0');
    const DWORD n = ::ExpandEnvironmentStringsW(win.c_str(), &buf[0], need);
    if (n == 0) return std::string(input.data(), input.size());
    buf.resize(static_cast<std::size_t>(n > 0 ? n - 1 : 0));
    return winpp::to_utf8(winpp::wstring_view(buf.data(), buf.size()));
}

}  // namespace sys
}  // namespace winpp
