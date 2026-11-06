#include <winpp_test.h>
#include <winpp/winpp.hpp>

#include <string>

namespace {

winpp::ui::window_options make_opts() {
    winpp::ui::window_options o;
    o.title = "WinAPI++ 测试窗口";
    o.width = 320;
    o.height = 240;
    o.style = WS_OVERLAPPEDWINDOW;
    o.center = false;
    return o;
}

void pump_messages() {
    MSG msg;
    while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
    }
}

}  // namespace

WINPP_TEST_CASE("窗口：创建与销毁") {
    auto w = winpp::ui::window::create(make_opts());
    WINPP_CHECK(static_cast<bool>(w));
    if (!w) return;
    WINPP_CHECK(w.value().valid());
    WINPP_CHECK(w.value().native() != nullptr);
}

WINPP_TEST_CASE("窗口：设置标题") {
    auto w = winpp::ui::window::create(make_opts());
    if (!w) {
        WINPP_CHECK(false);
        return;
    }
    w.value().set_title("新标题");

    wchar_t buf[64];
    ::ZeroMemory(buf, sizeof(buf));
    ::GetWindowTextW(w.value().native(), buf, 64);
    WINPP_CHECK(std::wstring(buf) == L"新标题");
}

WINPP_TEST_CASE("窗口：WM_CLOSE 处理器被调用") {
    auto w = winpp::ui::window::create(make_opts());
    if (!w) {
        WINPP_CHECK(false);
        return;
    }

    int called = 0;
    w.value().on(WM_CLOSE, [&called](winpp::ui::window&, UINT, WPARAM, LPARAM) -> LRESULT {
        ++called;
        return 0;
    });

    ::PostMessageW(w.value().native(), WM_CLOSE, 0, 0);
    pump_messages();

    WINPP_CHECK_EQ(called, 1);
}

WINPP_TEST_CASE("窗口：off 后处理器不再触发") {
    auto w = winpp::ui::window::create(make_opts());
    if (!w) {
        WINPP_CHECK(false);
        return;
    }

    int called = 0;
    w.value().on(WM_CLOSE, [&called](winpp::ui::window&, UINT, WPARAM, LPARAM) -> LRESULT {
        ++called;
        return 0;
    });
    w.value().off(WM_CLOSE);

    ::PostMessageW(w.value().native(), WM_CLOSE, 0, 0);
    pump_messages();

    WINPP_CHECK_EQ(called, 0);
}

WINPP_TEST_CASE("控件：按钮创建与文本") {
    auto w = winpp::ui::window::create(make_opts());
    if (!w) {
        WINPP_CHECK(false);
        return;
    }

    auto b = winpp::ui::button::create(w.value().native(), "点我", 1001, 10, 10, 120, 32);
    WINPP_CHECK(static_cast<bool>(b));
    if (!b) return;

    WINPP_CHECK(b.value().valid());
    WINPP_CHECK_EQ(b.value().text(), std::string("点我"));

    b.value().set_text("改过了");
    WINPP_CHECK_EQ(b.value().text(), std::string("改过了"));
}

WINPP_TEST_CASE("控件：编辑框文本") {
    auto w = winpp::ui::window::create(make_opts());
    if (!w) {
        WINPP_CHECK(false);
        return;
    }

    auto e = winpp::ui::edit::create(w.value().native(), "初始内容", 1002, 10, 60, 200, 28);
    WINPP_CHECK(static_cast<bool>(e));
    if (!e) return;

    WINPP_CHECK_EQ(e.value().text(), std::string("初始内容"));
}

WINPP_TEST_CASE("控件：列表框增项与读取") {
    auto w = winpp::ui::window::create(make_opts());
    if (!w) {
        WINPP_CHECK(false);
        return;
    }

    auto lb = winpp::ui::listbox::create(w.value().native(), 1003, 10, 10, 160, 120);
    WINPP_CHECK(static_cast<bool>(lb));
    if (!lb) return;

    lb.value().add_item("第一项");
    lb.value().add_item("第二项");

    WINPP_CHECK_EQ(lb.value().count(), 2);
    WINPP_CHECK_EQ(lb.value().item(0), std::string("第一项"));
    WINPP_CHECK_EQ(lb.value().item(1), std::string("第二项"));
}

WINPP_TEST_CASE("DPI：窗口 DPI 可读取") {
    auto w = winpp::ui::window::create(make_opts());
    if (!w) {
        WINPP_CHECK(false);
        return;
    }
    const unsigned int dpi = winpp::ui::dpi_for_window(w.value().native());
    WINPP_CHECK(dpi >= 96u);
}
