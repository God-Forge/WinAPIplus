// 唯一实现源：剪贴板
#include <winpp/ui/clipboard.hpp>
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

namespace winpp {
namespace ui {

WINPP_INLINE Result<std::string> clipboard_text() {
    if (!::OpenClipboard(nullptr)) return winpp::last_error("OpenClipboard");

    HANDLE h = ::GetClipboardData(CF_UNICODETEXT);
    if (h == nullptr) {
        ::CloseClipboard();
        return winpp::error(0u, "剪贴板中没有文本");
    }

    const wchar_t* p = static_cast<const wchar_t*>(::GlobalLock(h));
    if (p == nullptr) {
        ::CloseClipboard();
        return winpp::last_error("GlobalLock");
    }

    const std::string result = winpp::to_utf8(winpp::wstring_view(p));
    ::GlobalUnlock(h);
    ::CloseClipboard();
    return result;
}

WINPP_INLINE Result<void> set_clipboard_text(string_view text) {
    const winpp::wide_string w(text);
    const std::size_t bytes = (w.str().size() + 1) * sizeof(wchar_t);

    if (!::OpenClipboard(nullptr)) return winpp::last_error("OpenClipboard");
    ::EmptyClipboard();

    HGLOBAL h = ::GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (h == nullptr) {
        ::CloseClipboard();
        return winpp::last_error("GlobalAlloc");
    }

    void* p = ::GlobalLock(h);
    if (p == nullptr) {
        ::GlobalFree(h);
        ::CloseClipboard();
        return winpp::last_error("GlobalLock");
    }
    std::memcpy(p, w.c_str(), bytes);
    ::GlobalUnlock(h);

    if (::SetClipboardData(CF_UNICODETEXT, h) == nullptr) {
        ::GlobalFree(h);
        ::CloseClipboard();
        return winpp::last_error("SetClipboardData");
    }

    // SetClipboardData 成功后，内存归系统所有，不要 GlobalFree。
    ::CloseClipboard();
    return Result<void>();
}

}  // namespace ui
}  // namespace winpp
