// 唯一实现源：路径工具
#include <winpp/fs/path.hpp>
#include <winpp/core/check.hpp>
#include <winpp/core/string.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>

#include <string>

namespace winpp {
namespace fs {

WINPP_INLINE std::string join(string_view a, string_view b) {
    if (a.empty()) return std::string(b.data(), b.size());
    if (b.empty()) return std::string(a.data(), a.size());

    // b 是绝对路径（盘符或 UNC）时直接返回 b
    if (b.size() >= 2 && b[1] == ':') return std::string(b.data(), b.size());
    if (b.size() >= 2 && b[0] == '\\' && b[1] == '\\') return std::string(b.data(), b.size());

    std::string r(a.data(), a.size());
    const char last = r.empty() ? '\0' : r[r.size() - 1];
    if (last != '\\' && last != '/') r.push_back('\\');
    r.append(b.data(), b.size());
    return r;
}

WINPP_INLINE Result<std::string> full_path(string_view p) {
    const winpp::wide_string wp(p);
    const DWORD need = ::GetFullPathNameW(wp.c_str(), 0, nullptr, nullptr);
    if (need == 0) return winpp::last_error("GetFullPathNameW");

    std::wstring buf(static_cast<std::size_t>(need), L'\0');
    const DWORD n = ::GetFullPathNameW(wp.c_str(), need, &buf[0], nullptr);
    if (n == 0) return winpp::last_error("GetFullPathNameW");
    buf.resize(n);
    return winpp::to_utf8(winpp::wstring_view(buf.data(), buf.size()));
}

WINPP_INLINE bool exists(string_view p) {
    const winpp::wide_string wp(p);
    return ::GetFileAttributesW(wp.c_str()) != INVALID_FILE_ATTRIBUTES;
}

WINPP_INLINE Result<void> remove(string_view p) {
    const winpp::wide_string wp(p);
    if (!::DeleteFileW(wp.c_str())) return winpp::last_error("DeleteFileW");
    return Result<void>();
}

WINPP_INLINE Result<std::string> known_folder(int csidl) {
    wchar_t buf[MAX_PATH];
    const HRESULT hr = ::SHGetFolderPathW(nullptr, csidl, nullptr, 0, buf);
    if (FAILED(hr)) return winpp::error(static_cast<unsigned long>(hr), "SHGetFolderPathW");
    return winpp::to_utf8(winpp::wstring_view(buf));
}

}  // namespace fs
}  // namespace winpp
