// 唯一实现源：窗口（全部 inline，三形态共用）
#include <winpp/ui/window.hpp>

namespace winpp {
namespace ui {

namespace detail {

inline const wchar_t* window_class_name() {
    return L"WinppWindowClass";
}

}  // namespace detail

inline window::window(window&& other) noexcept
    : hwnd_(other.hwnd_), handlers_(std::move(other.handlers_)) {
    other.hwnd_ = nullptr;
    if (hwnd_ != nullptr) {
        ::SetWindowLongPtrW(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    }
}

inline window& window::operator=(window&& other) noexcept {
    if (this != &other) {
        if (hwnd_ != nullptr && ::IsWindow(hwnd_)) ::DestroyWindow(hwnd_);
        hwnd_ = other.hwnd_;
        handlers_ = std::move(other.handlers_);
        other.hwnd_ = nullptr;
        if (hwnd_ != nullptr) {
            ::SetWindowLongPtrW(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        }
    }
    return *this;
}

inline window::~window() {
    if (hwnd_ != nullptr && ::IsWindow(hwnd_)) {
        ::DestroyWindow(hwnd_);
    }
    hwnd_ = nullptr;
}

inline bool window::valid() const noexcept {
    return hwnd_ != nullptr && ::IsWindow(hwnd_) != 0;
}

inline void window::show(int cmd) noexcept {
    if (hwnd_ != nullptr) ::ShowWindow(hwnd_, cmd);
}

inline void window::hide() noexcept {
    if (hwnd_ != nullptr) ::ShowWindow(hwnd_, SW_HIDE);
}

inline void window::close() noexcept {
    if (hwnd_ != nullptr) ::PostMessageW(hwnd_, WM_CLOSE, 0, 0);
}

inline void window::set_title(string_view title) {
    if (hwnd_ == nullptr) return;
    const winpp::wide_string w(title);
    ::SetWindowTextW(hwnd_, w.c_str());
}

inline void window::on(UINT msg, handler h) {
    handlers_[msg] = std::move(h);
}

inline void window::off(UINT msg) {
    handlers_.erase(msg);
}

inline int window::run() {
    MSG msg;
    while (::GetMessageW(&msg, nullptr, 0, 0) > 0) {
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}

inline LRESULT CALLBACK window::wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        window* self = static_cast<window*>(cs->lpCreateParams);
        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    }

    window* self = reinterpret_cast<window*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self != nullptr) {
        return self->dispatch(msg, wp, lp);
    }
    return ::DefWindowProcW(hwnd, msg, wp, lp);
}

inline LRESULT window::dispatch(UINT msg, WPARAM wp, LPARAM lp) {
    std::unordered_map<UINT, handler>::iterator it = handlers_.find(msg);
    if (it != handlers_.end() && it->second) {
        return it->second(*this, msg, wp, lp);
    }
    if (msg == WM_DESTROY) {
        ::PostQuitMessage(0);
    }
    return ::DefWindowProcW(hwnd_, msg, wp, lp);
}

inline Result<window> window::create(const window_options& opts) {
    static bool registered = false;
    const HINSTANCE hInst = ::GetModuleHandleW(nullptr);

    if (!registered) {
        WNDCLASSEXW wc;
        ::ZeroMemory(&wc, sizeof(wc));
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = &window::wnd_proc;
        wc.hInstance = hInst;
        wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.lpszClassName = detail::window_class_name();
        if (::RegisterClassExW(&wc) == 0) {
            return winpp::last_error("RegisterClassExW");
        }
        registered = true;
    }

    int x = CW_USEDEFAULT;
    int y = CW_USEDEFAULT;
    int w = opts.width;
    int h = opts.height;

    if (opts.center) {
        RECT rc;
        rc.left = 0;
        rc.top = 0;
        rc.right = w;
        rc.bottom = h;
        ::AdjustWindowRectEx(&rc, static_cast<DWORD>(opts.style), FALSE, 0);
        w = rc.right - rc.left;
        h = rc.bottom - rc.top;
        x = (::GetSystemMetrics(SM_CXSCREEN) - w) / 2;
        y = (::GetSystemMetrics(SM_CYSCREEN) - h) / 2;
    }

    window self;
    const winpp::wide_string wtitle(opts.title);

    const HWND hwnd = ::CreateWindowExW(
        0,
        detail::window_class_name(),
        wtitle.c_str(),
        static_cast<DWORD>(opts.style),
        x, y, w, h,
        nullptr, nullptr, hInst, &self);

    if (hwnd == nullptr) {
        return winpp::last_error("CreateWindowExW");
    }
    self.hwnd_ = hwnd;
    return self;
}

inline int message_loop() {
    MSG msg;
    while (::GetMessageW(&msg, nullptr, 0, 0) > 0) {
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}

}  // namespace ui
}  // namespace winpp
