// 唯一实现源：托盘图标（全部 inline）
#include <winpp/ui/tray.hpp>

namespace winpp {
namespace ui {

inline tray_icon::tray_icon() noexcept : added_(false) {
    ::ZeroMemory(&data_, sizeof(data_));
    data_.cbSize = sizeof(data_);
}

inline tray_icon::~tray_icon() {
    remove();
}

inline Result<void> tray_icon::add(HWND owner, unsigned int id, string_view tooltip, HICON icon) {
    if (added_) remove();

    ::ZeroMemory(&data_, sizeof(data_));
    data_.cbSize = sizeof(data_);
    data_.hWnd = owner;
    data_.uID = id;
    data_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    data_.uCallbackMessage = WM_APP + 1;
    data_.hIcon = (icon != nullptr) ? icon : ::LoadIconW(nullptr, IDI_APPLICATION);

    const winpp::wide_string wtip(tooltip);
    ::lstrcpynW(data_.szTip, wtip.c_str(), 128);

    if (!::Shell_NotifyIconW(NIM_ADD, &data_)) {
        return winpp::last_error("Shell_NotifyIconW(NIM_ADD)");
    }
    added_ = true;
    return Result<void>();
}

inline void tray_icon::remove() {
    if (!added_) return;
    ::Shell_NotifyIconW(NIM_DELETE, &data_);
    added_ = false;
}

inline void tray_icon::set_tooltip(string_view tooltip) {
    if (!added_) return;
    const winpp::wide_string wtip(tooltip);
    ::lstrcpynW(data_.szTip, wtip.c_str(), 128);
    data_.uFlags = NIF_TIP;
    ::Shell_NotifyIconW(NIM_MODIFY, &data_);
}

}  // namespace ui
}  // namespace winpp
