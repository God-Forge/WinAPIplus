// 唯一实现源：时间
#include <winpp/sys/time.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <windows.h>

#include <cstdio>
#include <string>

namespace winpp {
namespace sys {

namespace detail {

inline system_time from_systemtime(const SYSTEMTIME& st) {
    system_time t;
    t.year = st.wYear;
    t.month = st.wMonth;
    t.day = st.wDay;
    t.hour = st.wHour;
    t.minute = st.wMinute;
    t.second = st.wSecond;
    t.millisecond = st.wMilliseconds;
    return t;
}

}  // namespace detail

WINPP_INLINE system_time local_time() {
    SYSTEMTIME st;
    ::GetLocalTime(&st);
    return detail::from_systemtime(st);
}

WINPP_INLINE system_time utc_time() {
    SYSTEMTIME st;
    ::GetSystemTime(&st);
    return detail::from_systemtime(st);
}

WINPP_INLINE unsigned long long now_unix_ms() {
    FILETIME ft;
    ::GetSystemTimeAsFileTime(&ft);

    ULARGE_INTEGER u;
    u.LowPart = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;

    // FILETIME 是 100ns 单位，起点 1601-01-01；
    // 转成毫秒后减去 1601 到 1970 的偏移。
    const unsigned long long ms = u.QuadPart / 10000ull;
    const unsigned long long epoch_offset = 11644473600000ull;
    return ms >= epoch_offset ? ms - epoch_offset : 0ull;
}

WINPP_INLINE std::string format_time(const system_time& t) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%04u-%02u-%02u %02u:%02u:%02u",
                  static_cast<unsigned>(t.year),
                  static_cast<unsigned>(t.month),
                  static_cast<unsigned>(t.day),
                  static_cast<unsigned>(t.hour),
                  static_cast<unsigned>(t.minute),
                  static_cast<unsigned>(t.second));
    return std::string(buf);
}

}  // namespace sys
}  // namespace winpp
