// WinAPI++ 标准检测
//
// 支持矩阵（见 Plan0 §3.2）：
//   MSVC   : 仅 C++17
//   MinGW  : C++11 / C++17 / C++23
//
// 注意：MSVC 默认 __cplusplus 恒为 199711L（除非 /Zc:__cplusplus），
// 因此必须优先读取 _MSVC_LANG。
#pragma once

#if defined(_MSVC_LANG)
#  define WINPP_CPLUSPLUS _MSVC_LANG
#else
#  define WINPP_CPLUSPLUS __cplusplus
#endif

#if WINPP_CPLUSPLUS >= 202302L
#  define WINPP_CXX_STD 23
#elif WINPP_CPLUSPLUS >= 201703L
#  define WINPP_CXX_STD 17
#elif WINPP_CPLUSPLUS >= 201103L
#  define WINPP_CXX_STD 11
#elif defined(_MSC_VER) && _MSC_VER >= 1900
// 兜底：老 MSVC 没有 _MSVC_LANG。不在支持矩阵内，仅避免误报。
#  define WINPP_CXX_STD 14
#else
#  error "WinAPI++ 需要 C++11 或更高标准（MSVC 请使用 /std:c++17）"
#endif

// 特性开关：上层只读这些宏，不直接写标准版本判断。
#define WINPP_HAS_STRING_VIEW      (WINPP_CXX_STD >= 17)
#define WINPP_HAS_IF_CONSTEXPR     (WINPP_CXX_STD >= 17)
#define WINPP_HAS_STRUCTURED_BIND  (WINPP_CXX_STD >= 17)
#define WINPP_HAS_SPAN             (WINPP_CXX_STD >= 20)
#define WINPP_HAS_EXPECTED         (WINPP_CXX_STD >= 23)
#define WINPP_HAS_NODISCARD        (WINPP_CXX_STD >= 17)
