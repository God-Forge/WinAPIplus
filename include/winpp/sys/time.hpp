// WinAPI++ 时间
#pragma once

#include <winpp/compat/config.hpp>
#include <winpp/compat/export.hpp>
#include <winpp/compat/shims.hpp>

#include <string>

namespace winpp {
namespace sys {

struct system_time {
    unsigned short year;
    unsigned short month;
    unsigned short day;
    unsigned short hour;
    unsigned short minute;
    unsigned short second;
    unsigned short millisecond;
};

WINPP_NODISCARD WINPP_API system_time local_time();
WINPP_NODISCARD WINPP_API system_time utc_time();

// Unix 毫秒时间戳（1970-01-01 起）
WINPP_NODISCARD WINPP_API unsigned long long now_unix_ms();

// 格式化为 "YYYY-MM-DD HH:MM:SS"
WINPP_NODISCARD WINPP_API std::string format_time(const system_time& t);

}  // namespace sys
}  // namespace winpp

#if defined(WINPP_HEADER_ONLY)
#  include <winpp/detail/time.inl>
#endif
