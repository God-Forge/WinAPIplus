// WinAPI++ 错误类型
//
// 设计要点（见 Plan0 §6.1）：
//   - message() 延迟格式化：正常路径零开销，只有真要打印时才调 FormatMessageW。
//   - error 本身保持 header-only（内联），只依赖导出的 format_message，
//     这样跨 DLL 边界传递也不会踩模板实例化的坑。
#pragma once

#include <winpp/compat/config.hpp>
#include <winpp/compat/export.hpp>
#include <winpp/compat/shims.hpp>

#include <string>
#include <system_error>

namespace winpp {

// 把 Win32 错误码格式化成 UTF-8 消息。
WINPP_NODISCARD WINPP_API std::string format_message(unsigned long code);

struct error {
    unsigned long   code    = 0;   // Win32 错误码（0 表示无 Win32 来源）
    std::error_code sys;           // 可选的 std::error_code
    string_view     context;       // 出错位置，如 "CreateFileW"

    error() {}

    error(unsigned long c, string_view ctx = string_view())
        : code(c), context(ctx) {}

    explicit error(std::error_code ec, string_view ctx = string_view())
        : sys(ec), context(ctx) {}

    // 延迟格式化：调用时才展开成人类可读消息。
    WINPP_NODISCARD std::string message() const {
        if (code != 0) return format_message(code);
        if (sys) return sys.message();
        return std::string();
    }

    explicit operator bool() const noexcept {
        return code != 0 || static_cast<bool>(sys);
    }
};

// 捕获当前线程的 GetLastError()。
WINPP_NODISCARD WINPP_API error last_error(string_view context = string_view()) noexcept;

// 高频错误码判定
inline bool is_not_found(const error& e) noexcept      { return e.code == 2u;   }  // ERROR_FILE_NOT_FOUND
inline bool is_access_denied(const error& e) noexcept  { return e.code == 5u;   }  // ERROR_ACCESS_DENIED
inline bool is_already_exists(const error& e) noexcept { return e.code == 183u; }  // ERROR_ALREADY_EXISTS

}  // namespace winpp

#if defined(WINPP_HEADER_ONLY)
#  include <winpp/detail/error.inl>
#endif
