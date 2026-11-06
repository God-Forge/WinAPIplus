// 唯一实现源：控件（全部 inline）
#include <winpp/ui/controls.hpp>

namespace winpp {
namespace ui {

inline control::control(control&& other) noexcept : hwnd_(other.hwnd_) {
    other.hwnd_ = nullptr;
}

inline control& control::operator=(control&& other) noexcept {
    if (this != &other) {
        destroy();
        hwnd_ = other.hwnd_;
        other.hwnd_ = nullptr;
    }
    return *this;
}

inline control::~control() {
    destroy();
}

inline void control::destroy() noexcept {
    if (hwnd_ != nullptr && ::IsWindow(hwnd_)) {
        ::DestroyWindow(hwnd_);
    }
    hwnd_ = nullptr;
}

inline bool control::valid() const noexcept {
    return hwnd_ != nullptr && ::IsWindow(hwnd_) != 0;
}

inline void control::set_text(string_view text) {
    if (hwnd_ == nullptr) return;
    const winpp::wide_string w(text);
    ::SetWindowTextW(hwnd_, w.c_str());
}

inline std::string control::text() const {
    if (hwnd_ == nullptr) return std::string();
    const int len = ::GetWindowTextLengthW(hwnd_);
    if (len <= 0) return std::string();

    std::wstring buf(static_cast<std::size_t>(len) + 1, L'\0');
    const int got = ::GetWindowTextW(hwnd_, &buf[0], len + 1);
    if (got <= 0) return std::string();
    buf.resize(static_cast<std::size_t>(got));
    return winpp::to_utf8(winpp::wstring_view(buf.data(), buf.size()));
}

inline void control::enable(bool on) {
    if (hwnd_ != nullptr) ::EnableWindow(hwnd_, on ? TRUE : FALSE);
}

inline void control::show(int cmd) {
    if (hwnd_ != nullptr) ::ShowWindow(hwnd_, cmd);
}

inline void control::hide() noexcept {
    if (hwnd_ != nullptr) ::ShowWindow(hwnd_, SW_HIDE);
}

inline Result<button> button::create(HWND parent, string_view text, int id,
                                     int x, int y, int w, int h) {
    const winpp::wide_string wtext(text);
    const HWND hwnd = ::CreateWindowExW(
        0, L"BUTTON", wtext.c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x, y, w, h, parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        ::GetModuleHandleW(nullptr), nullptr);
    if (hwnd == nullptr) return winpp::last_error("CreateWindowExW(button)");

    button b;
    b.hwnd_ = hwnd;
    return b;
}

inline Result<edit> edit::create(HWND parent, string_view text, int id,
                                 int x, int y, int w, int h) {
    const winpp::wide_string wtext(text);
    const HWND hwnd = ::CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", wtext.c_str(),
        WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
        x, y, w, h, parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        ::GetModuleHandleW(nullptr), nullptr);
    if (hwnd == nullptr) return winpp::last_error("CreateWindowExW(edit)");

    edit e;
    e.hwnd_ = hwnd;
    return e;
}

inline Result<listbox> listbox::create(HWND parent, int id, int x, int y, int w, int h) {
    const HWND hwnd = ::CreateWindowExW(
        WS_EX_CLIENTEDGE, L"LISTBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
        x, y, w, h, parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        ::GetModuleHandleW(nullptr), nullptr);
    if (hwnd == nullptr) return winpp::last_error("CreateWindowExW(listbox)");

    listbox lb;
    lb.hwnd_ = hwnd;
    return lb;
}

inline void listbox::add_item(string_view text) {
    if (hwnd_ == nullptr) return;
    const winpp::wide_string w(text);
    ::SendMessageW(hwnd_, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(w.c_str()));
}

inline int listbox::count() const {
    if (hwnd_ == nullptr) return 0;
    return static_cast<int>(::SendMessageW(hwnd_, LB_GETCOUNT, 0, 0));
}

inline std::string listbox::item(int index) const {
    if (hwnd_ == nullptr) return std::string();
    const LRESULT len = ::SendMessageW(hwnd_, LB_GETTEXTLEN,
                                       static_cast<WPARAM>(index), 0);
    if (len <= 0) return std::string();

    std::wstring buf(static_cast<std::size_t>(len) + 1, L'\0');
    ::SendMessageW(hwnd_, LB_GETTEXT, static_cast<WPARAM>(index),
                   reinterpret_cast<LPARAM>(&buf[0]));
    buf.resize(static_cast<std::size_t>(len));
    return winpp::to_utf8(winpp::wstring_view(buf.data(), buf.size()));
}

inline int listbox::selection() const {
    if (hwnd_ == nullptr) return -1;
    return static_cast<int>(::SendMessageW(hwnd_, LB_GETCURSEL, 0, 0));
}

inline void listbox::set_selection(int index) {
    if (hwnd_ != nullptr) {
        ::SendMessageW(hwnd_, LB_SETCURSEL, static_cast<WPARAM>(index), 0);
    }
}

}  // namespace ui
}  // namespace winpp
