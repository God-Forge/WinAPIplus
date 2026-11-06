// WinAPI++ 分发形态导出宏
//
// 三种形态（见 Plan0 §4）：
//   header-only : 定义 WINPP_HEADER_ONLY          -> WINPP_API 空, WINPP_INLINE = inline
//   静态库      : 不定义任何宏                     -> WINPP_API 空, WINPP_INLINE 空
//   动态库      : 库侧 WINPP_BUILD_SHARED          -> __declspec(dllexport)
//                 用户侧 WINPP_USE_SHARED          -> __declspec(dllimport)
//
// 实现文件统一写在 detail/*.inl 里，带 WINPP_INLINE：
//   header-only 时被公共头尾部 include，inline 保证多 TU 不重定义；
//   库模式时被 src/*.cpp include，产生唯一外部符号。
#pragma once

#if defined(WINPP_HEADER_ONLY)
#  define WINPP_API
#  define WINPP_INLINE inline
#elif defined(WINPP_BUILD_SHARED)
#  define WINPP_API __declspec(dllexport)
#  define WINPP_INLINE
#elif defined(WINPP_USE_SHARED)
#  define WINPP_API __declspec(dllimport)
#  define WINPP_INLINE
#else
// 静态库
#  define WINPP_API
#  define WINPP_INLINE
#endif
