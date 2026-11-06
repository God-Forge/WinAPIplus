// 唯一实现源：注册表
#include <winpp/sys/registry.hpp>
#include <winpp/core/check.hpp>
#include <winpp/core/string.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <windows.h>

#include <string>
#include <utility>

namespace winpp {
namespace sys {

WINPP_INLINE key::key(key&& other) noexcept : h_(std::move(other.h_)) {}

WINPP_INLINE key& key::operator=(key&& other) noexcept {
    if (this != &other) h_ = std::move(other.h_);
    return *this;
}

WINPP_INLINE key::~key() {}

WINPP_INLINE Result<key> key::open(HKEY root, string_view subkey, unsigned long access) {
    const winpp::wide_string wsub(subkey);
    HKEY h = nullptr;
    const LONG rc = ::RegOpenKeyExW(root, wsub.c_str(), 0, static_cast<DWORD>(access), &h);
    if (rc != ERROR_SUCCESS) {
        return winpp::error(static_cast<unsigned long>(rc), "RegOpenKeyExW");
    }
    return key(h);
}

WINPP_INLINE Result<key> key::create(HKEY root, string_view subkey, unsigned long access) {
    const winpp::wide_string wsub(subkey);
    HKEY h = nullptr;
    DWORD disposition = 0;
    const LONG rc = ::RegCreateKeyExW(root, wsub.c_str(), 0, nullptr,
                                      REG_OPTION_NON_VOLATILE,
                                      static_cast<DWORD>(access), nullptr,
                                      &h, &disposition);
    if (rc != ERROR_SUCCESS) {
        return winpp::error(static_cast<unsigned long>(rc), "RegCreateKeyExW");
    }
    return key(h);
}

WINPP_INLINE Result<std::string> key::get_string(string_view name) const {
    if (!h_.valid()) return winpp::error(6u, "key::get_string");

    const winpp::wide_string wname(name);
    DWORD type = 0;
    DWORD size = 0;
    LONG rc = ::RegQueryValueExW(h_.get(), wname.c_str(), nullptr, &type, nullptr, &size);
    if (rc != ERROR_SUCCESS) {
        return winpp::error(static_cast<unsigned long>(rc), "RegQueryValueExW");
    }
    if (type != REG_SZ && type != REG_EXPAND_SZ) {
        return winpp::error(0u, "值类型不是字符串");
    }

    std::wstring buf(static_cast<std::size_t>(size) / sizeof(wchar_t) + 1, L'\0');
    DWORD real_size = static_cast<DWORD>(buf.size() * sizeof(wchar_t));
    rc = ::RegQueryValueExW(h_.get(), wname.c_str(), nullptr, &type,
                            reinterpret_cast<LPBYTE>(&buf[0]), &real_size);
    if (rc != ERROR_SUCCESS) {
        return winpp::error(static_cast<unsigned long>(rc), "RegQueryValueExW");
    }
    buf.resize(static_cast<std::size_t>(real_size) / sizeof(wchar_t));
    while (!buf.empty() && buf.back() == L'\0') buf.pop_back();
    return winpp::to_utf8(winpp::wstring_view(buf.data(), buf.size()));
}

WINPP_INLINE Result<unsigned long> key::get_dword(string_view name) const {
    if (!h_.valid()) return winpp::error(6u, "key::get_dword");

    const winpp::wide_string wname(name);
    DWORD type = 0;
    DWORD value = 0;
    DWORD size = sizeof(value);
    const LONG rc = ::RegQueryValueExW(h_.get(), wname.c_str(), nullptr, &type,
                                       reinterpret_cast<LPBYTE>(&value), &size);
    if (rc != ERROR_SUCCESS) {
        return winpp::error(static_cast<unsigned long>(rc), "RegQueryValueExW");
    }
    if (type != REG_DWORD) return winpp::error(0u, "值类型不是 DWORD");
    return static_cast<unsigned long>(value);
}

WINPP_INLINE Result<void> key::set_string(string_view name, string_view value) {
    if (!h_.valid()) return winpp::error(6u, "key::set_string");

    const winpp::wide_string wname(name);
    const winpp::wide_string wvalue(value);
    const DWORD size = static_cast<DWORD>((wvalue.str().size() + 1) * sizeof(wchar_t));
    const LONG rc = ::RegSetValueExW(h_.get(), wname.c_str(), 0, REG_SZ,
                                     reinterpret_cast<const BYTE*>(wvalue.c_str()), size);
    if (rc != ERROR_SUCCESS) {
        return winpp::error(static_cast<unsigned long>(rc), "RegSetValueExW");
    }
    return Result<void>();
}

WINPP_INLINE Result<void> key::set_dword(string_view name, unsigned long value) {
    if (!h_.valid()) return winpp::error(6u, "key::set_dword");

    const winpp::wide_string wname(name);
    const DWORD v = static_cast<DWORD>(value);
    const LONG rc = ::RegSetValueExW(h_.get(), wname.c_str(), 0, REG_DWORD,
                                     reinterpret_cast<const BYTE*>(&v), sizeof(v));
    if (rc != ERROR_SUCCESS) {
        return winpp::error(static_cast<unsigned long>(rc), "RegSetValueExW");
    }
    return Result<void>();
}

WINPP_INLINE Result<void> key::remove_value(string_view name) {
    if (!h_.valid()) return winpp::error(6u, "key::remove_value");
    const winpp::wide_string wname(name);
    const LONG rc = ::RegDeleteValueW(h_.get(), wname.c_str());
    if (rc != ERROR_SUCCESS) {
        return winpp::error(static_cast<unsigned long>(rc), "RegDeleteValueW");
    }
    return Result<void>();
}

WINPP_INLINE Result<void> key::remove_subkey(string_view subkey) {
    if (!h_.valid()) return winpp::error(6u, "key::remove_subkey");
    const winpp::wide_string wsub(subkey);
    const LONG rc = ::RegDeleteKeyW(h_.get(), wsub.c_str());
    if (rc != ERROR_SUCCESS) {
        return winpp::error(static_cast<unsigned long>(rc), "RegDeleteKeyW");
    }
    return Result<void>();
}

WINPP_INLINE Result<void> key::remove_tree(string_view subkey) {
    if (!h_.valid()) return winpp::error(6u, "key::remove_tree");
    const winpp::wide_string wsub(subkey);
    const LONG rc = ::RegDeleteTreeW(h_.get(), wsub.c_str());
    if (rc != ERROR_SUCCESS) {
        return winpp::error(static_cast<unsigned long>(rc), "RegDeleteTreeW");
    }
    return Result<void>();
}

}  // namespace sys
}  // namespace winpp
