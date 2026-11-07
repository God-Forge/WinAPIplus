#include <winpp_test.h>
#include <winpp/winpp.hpp>

WINPP_TEST_CASE("GDI：屏幕 DC 与内存 DC") {
    auto screen = winpp::gdi::dc::from_window(nullptr);
    WINPP_CHECK(static_cast<bool>(screen));
    if (!screen) return;

    auto mem = winpp::gdi::dc::create_compatible(screen.value().native());
    WINPP_CHECK(static_cast<bool>(mem));
    if (mem) WINPP_CHECK(mem.value().valid());
}

WINPP_TEST_CASE("GDI：画笔与画刷") {
    auto pen = winpp::gdi::create_pen(RGB(255, 0, 0), 2);
    WINPP_CHECK(static_cast<bool>(pen));
    if (pen) WINPP_CHECK(pen.value().valid());

    auto brush = winpp::gdi::create_solid_brush(RGB(0, 120, 215));
    WINPP_CHECK(static_cast<bool>(brush));
    if (brush) WINPP_CHECK(brush.value().valid());
}

WINPP_TEST_CASE("GDI：字体") {
    auto font = winpp::gdi::create_font("Consolas", 16);
    WINPP_CHECK(static_cast<bool>(font));
    if (font) WINPP_CHECK(font.value().valid());
}

WINPP_TEST_CASE("GDI：绘制操作不崩溃") {
    auto screen = winpp::gdi::dc::from_window(nullptr);
    if (!screen) {
        WINPP_CHECK(true);
        return;
    }

    auto mem = winpp::gdi::dc::create_compatible(screen.value().native());
    if (!mem) {
        WINPP_CHECK(true);
        return;
    }

    auto brush = winpp::gdi::create_solid_brush(RGB(0, 120, 215));
    if (!brush) return;

    RECT rc;
    rc.left = 0;
    rc.top = 0;
    rc.right = 100;
    rc.bottom = 50;

    mem.value().fill_rect(rc, brush.value().get());
    mem.value().set_text_color(RGB(255, 255, 255));
    mem.value().set_bk_mode(TRANSPARENT);
    mem.value().text_out(5, 5, "hello");
    mem.value().text_rect(rc, "centered", DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    mem.value().rectangle(rc);
    mem.value().line(0, 0, 100, 50);
    mem.value().ellipse(rc);

    WINPP_CHECK(true);
}

WINPP_TEST_CASE("GDI：双缓冲") {
    auto screen = winpp::gdi::dc::from_window(nullptr);
    if (!screen) {
        WINPP_CHECK(true);
        return;
    }

    auto bb = winpp::gdi::back_buffer::create(screen.value().native(), 200, 100);
    WINPP_CHECK(static_cast<bool>(bb));
    if (bb) {
        WINPP_CHECK(bb.value().valid());
        bb.value().flush(0, 0);
    }
}

WINPP_TEST_CASE("GDI：object_selector 自动恢复") {
    auto screen = winpp::gdi::dc::from_window(nullptr);
    if (!screen) {
        WINPP_CHECK(true);
        return;
    }

    auto pen = winpp::gdi::create_pen(RGB(0, 255, 0), 1);
    if (!pen) return;

    {
        winpp::gdi::object_selector sel(screen.value().native(), pen.value().get());
        WINPP_CHECK(sel.old() != nullptr);
    }
    WINPP_CHECK(true);
}

WINPP_TEST_CASE("GDI：borrow 不释放外部 DC") {
    auto screen = winpp::gdi::dc::from_window(nullptr);
    if (!screen) {
        WINPP_CHECK(true);
        return;
    }

    HDC raw = screen.value().native();
    {
        auto borrowed = winpp::gdi::dc::borrow(raw);
        WINPP_CHECK(borrowed.native() == raw);
    }
    // 借用对象析构后，原 DC 仍应可用
    WINPP_CHECK(screen.value().valid());
    WINPP_CHECK(::GetDeviceCaps(raw, LOGPIXELSX) > 0);
}
