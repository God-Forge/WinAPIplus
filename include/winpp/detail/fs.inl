// 唯一实现源：文件对象
// header-only 模式由 file.hpp 尾部 include；库模式由 src/fs.cpp include。
#include <winpp/fs/file.hpp>
#include <winpp/core/check.hpp>
#include <winpp/core/string.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <windows.h>

#include <cstring>
#include <string>
#include <vector>

namespace winpp {
namespace fs {

WINPP_INLINE Result<std::vector<unsigned char> > file::read_all() const {
    if (!h_.valid()) return winpp::error(6u, "file::read_all");  // ERROR_INVALID_HANDLE

    LARGE_INTEGER size;
    size.QuadPart = 0;
    if (!::GetFileSizeEx(h_.get(), &size)) return winpp::last_error("GetFileSizeEx");
    if (size.QuadPart <= 0) return std::vector<unsigned char>();

    std::vector<unsigned char> buf(static_cast<std::size_t>(size.QuadPart));
    std::size_t total = 0;
    while (total < buf.size()) {
        DWORD n = 0;
        const DWORD want = static_cast<DWORD>(buf.size() - total);
        if (!::ReadFile(h_.get(), &buf[total], want, &n, nullptr)) {
            return winpp::last_error("ReadFile");
        }
        if (n == 0) break;
        total += n;
    }
    buf.resize(total);
    return buf;
}

WINPP_INLINE Result<std::string> file::read_text() const {
    WINPP_TRY(raw, read_all());

    const unsigned char* p = raw.empty() ? nullptr : &raw[0];
    const std::size_t n = raw.size();

    // UTF-8 BOM
    if (n >= 3 && p[0] == 0xEF && p[1] == 0xBB && p[2] == 0xBF) {
        return std::string(reinterpret_cast<const char*>(p + 3), n - 3);
    }

    // UTF-16 LE BOM
    if (n >= 2 && p[0] == 0xFF && p[1] == 0xFE) {
        std::wstring w;
        w.reserve((n - 2) / 2);
        for (std::size_t i = 2; i + 1 < n; i += 2) {
            w.push_back(static_cast<wchar_t>(p[i] | (static_cast<unsigned>(p[i + 1]) << 8)));
        }
        return winpp::to_utf8(winpp::wstring_view(w.data(), w.size()));
    }

    // UTF-16 BE BOM
    if (n >= 2 && p[0] == 0xFE && p[1] == 0xFF) {
        std::wstring w;
        w.reserve((n - 2) / 2);
        for (std::size_t i = 2; i + 1 < n; i += 2) {
            w.push_back(static_cast<wchar_t>((static_cast<unsigned>(p[i]) << 8) | p[i + 1]));
        }
        return winpp::to_utf8(winpp::wstring_view(w.data(), w.size()));
    }

    return std::string(reinterpret_cast<const char*>(p), n);
}

WINPP_INLINE Result<void> file::write(string_view data) {
    if (!h_.valid()) return winpp::error(6u, "file::write");

    std::size_t total = 0;
    while (total < data.size()) {
        DWORD n = 0;
        const DWORD want = static_cast<DWORD>(data.size() - total);
        if (!::WriteFile(h_.get(), data.data() + total, want, &n, nullptr)) {
            return winpp::last_error("WriteFile");
        }
        total += n;
    }
    return Result<void>();
}

WINPP_INLINE Result<unsigned long long> file::size() const {
    if (!h_.valid()) return winpp::error(6u, "file::size");

    LARGE_INTEGER s;
    s.QuadPart = 0;
    if (!::GetFileSizeEx(h_.get(), &s)) return winpp::last_error("GetFileSizeEx");
    return static_cast<unsigned long long>(s.QuadPart);
}

WINPP_INLINE Result<file> open(string_view path, access acc) {
    DWORD desired = 0;
    DWORD disposition = OPEN_EXISTING;
    switch (acc) {
        case access::read:
            desired = GENERIC_READ;
            break;
        case access::write:
            desired = GENERIC_WRITE;
            disposition = CREATE_ALWAYS;
            break;
        case access::read_write:
            desired = GENERIC_READ | GENERIC_WRITE;
            disposition = OPEN_ALWAYS;
            break;
    }

    const winpp::wide_string wpath(path);
    const HANDLE h = ::CreateFileW(wpath.c_str(), desired, FILE_SHARE_READ, nullptr,
                                   disposition, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return winpp::last_error("CreateFileW");
    return file(h);
}

}  // namespace fs
}  // namespace winpp
