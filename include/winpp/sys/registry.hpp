// WinAPI++ 注册表
//
// 薄封装 RegOpenKeyExW / RegQueryValueExW / RegSetValueExW 等。
#pragma once

#include <winpp/compat/config.hpp>
#include <winpp/compat/export.hpp>
#include <winpp/compat/shims.hpp>
#include <winpp/core/error.hpp>
#include <winpp/core/result.hpp>
#include <winpp/core/traits.hpp>

#include <string>

namespace winpp {
namespace sys {

class WINPP_API key {
public:
    key() noexcept : h_() {}
    explicit key(HKEY h) noexcept : h_(h) {}

    key(key&& other) noexcept;
    key& operator=(key&& other) noexcept;
    key(const key&) = delete;
    key& operator=(const key&) = delete;
    ~key();

    bool valid() const noexcept { return h_.valid(); }
    HKEY native() const noexcept { return h_.get(); }

    // 打开已存在的键（RegOpenKeyExW）
    static Result<key> open(HKEY root, string_view subkey,
                            unsigned long access = KEY_READ);

    // 创建或打开键（RegCreateKeyExW）
    static Result<key> create(HKEY root, string_view subkey,
                              unsigned long access = KEY_WRITE);

    WINPP_NODISCARD Result<std::string>  get_string(string_view name) const;
    WINPP_NODISCARD Result<unsigned long> get_dword(string_view name) const;

    WINPP_NODISCARD Result<void> set_string(string_view name, string_view value);
    WINPP_NODISCARD Result<void> set_dword(string_view name, unsigned long value);

    WINPP_NODISCARD Result<void> remove_value(string_view name);
    WINPP_NODISCARD Result<void> remove_subkey(string_view subkey);
    WINPP_NODISCARD Result<void> remove_tree(string_view subkey);

private:
    winpp::registry_key h_;
};

}  // namespace sys
}  // namespace winpp

#if defined(WINPP_HEADER_ONLY)
#  include <winpp/detail/registry.inl>
#endif
