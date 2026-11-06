// WinAPI++ 字符串转换
//
// 规则（见 Plan0 §6.4）：公开 API 入参一律 string_view（UTF-8），
// 返回值一律 std::string（UTF-8）。内部与 Win32 交界处统一转 UTF-16。
#pragma once

#include <winpp/compat/config.hpp>
#include <winpp/compat/export.hpp>
#include <winpp/compat/shims.hpp>

#include <string>

namespace winpp {

WINPP_NODISCARD WINPP_API std::string  to_utf8(wstring_view w);
WINPP_NODISCARD WINPP_API std::wstring to_wide(string_view u8);

// 轻量适配器：把 UTF-8 临时转成 wchar_t*，调用点不必写 .c_str()。
// 注意：它持有转换结果，生命周期跟随自身；不要保存 c_str() 到对象销毁之后。
class wide_string {
public:
    explicit wide_string(string_view u8) : buf_(to_wide(u8)) {}

    const wchar_t* c_str() const noexcept { return buf_.c_str(); }
    operator const wchar_t*() const noexcept { return buf_.c_str(); }

    const std::wstring& str() const noexcept { return buf_; }

private:
    std::wstring buf_;
};

}  // namespace winpp

#if defined(WINPP_HEADER_ONLY)
#  include <winpp/detail/string.inl>
#endif
