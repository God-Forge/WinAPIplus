// WinAPI++ 句柄 traits 与常用句柄别名
//
// 这里集中 include <windows.h>，其余核心头尽量不碰它。
#pragma once

#include <winpp/compat/config.hpp>
#include <winpp/core/handle.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <windows.h>

namespace winpp {

// 文件 / 管道等内核对象：无效值是 INVALID_HANDLE_VALUE
struct file_traits {
    typedef HANDLE value_type;
    static value_type invalid() noexcept { return INVALID_HANDLE_VALUE; }
    static void close(value_type h) noexcept {
        if (h != INVALID_HANDLE_VALUE && h != nullptr) ::CloseHandle(h);
    }
};

// 事件 / 互斥量 / 信号量等同步对象：无效值是 nullptr
struct event_traits {
    typedef HANDLE value_type;
    static value_type invalid() noexcept { return nullptr; }
    static void close(value_type h) noexcept { if (h != nullptr) ::CloseHandle(h); }
};

struct mutex_traits {
    typedef HANDLE value_type;
    static value_type invalid() noexcept { return nullptr; }
    static void close(value_type h) noexcept { if (h != nullptr) ::CloseHandle(h); }
};

struct semaphore_traits {
    typedef HANDLE value_type;
    static value_type invalid() noexcept { return nullptr; }
    static void close(value_type h) noexcept { if (h != nullptr) ::CloseHandle(h); }
};

// 模块（DLL）句柄：用 FreeLibrary 释放
struct module_traits {
    typedef HMODULE value_type;
    static value_type invalid() noexcept { return nullptr; }
    static void close(value_type h) noexcept { if (h != nullptr) ::FreeLibrary(h); }
};

// 注册表键：用 RegCloseKey 释放
struct registry_traits {
    typedef HKEY value_type;
    static value_type invalid() noexcept { return nullptr; }
    static void close(value_type h) noexcept { if (h != nullptr) ::RegCloseKey(h); }
};

typedef basic_handle<file_traits>      file_handle;
typedef basic_handle<event_traits>     event_handle;
typedef basic_handle<mutex_traits>     mutex_handle;
typedef basic_handle<semaphore_traits> semaphore_handle;
typedef basic_handle<module_traits>    module_handle;
typedef basic_handle<registry_traits>  registry_key;

}  // namespace winpp
