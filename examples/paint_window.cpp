// M7 示例：GDI 双缓冲绘制
#include <winpp/winpp.hpp>

#include <cstdio>

int main() {
    winpp::ui::enable_dpi_awareness();

    winpp::ui::window_options opts;
    opts.title = "WinAPI++ GDI 示例";
    opts.width = 640;
    opts.height = 480;

    auto w = winpp::ui::window::create(opts);
    if (!w) {
        std::printf("创建窗口失败：%s\n", w.error().message().c_str());
        return 1;
    }

    w.value().on(WM_CLOSE, [](winpp::ui::window& self, UINT, WPARAM, LPARAM) -> LRESULT {
        ::DestroyWindow(self.native());
        return 0;
    });

    w.value().on(WM_PAINT, [](winpp::ui::window& self, UINT, WPARAM, LPARAM) -> LRESULT {
        PAINTSTRUCT ps;
        const HDC hdc = ::BeginPaint(self.native(), &ps);

        RECT rc;
        ::GetClientRect(self.native(), &rc);
        const int width = rc.right - rc.left;
        const int height = rc.bottom - rc.top;

        auto bb = winpp::gdi::back_buffer::create(hdc, width, height);
        if (bb) {
            auto mem = winpp::gdi::dc::borrow(bb.value().native());

            auto bg = winpp::gdi::create_solid_brush(RGB(32, 32, 32));
            auto accent = winpp::gdi::create_solid_brush(RGB(0, 120, 215));
            auto font = winpp::gdi::create_font("Segoe UI", 28);

            RECT full;
            full.left = 0;
            full.top = 0;
            full.right = width;
            full.bottom = height;
            if (bg) mem.fill_rect(full, bg.value().get());

            RECT box;
            box.left = 40;
            box.top = 40;
            box.right = 240;
            box.bottom = 140;
            if (accent) mem.fill_rect(box, accent.value().get());

            if (font) {
                winpp::gdi::object_selector sel(bb.value().native(), font.value().get());
                mem.set_text_color(RGB(240, 240, 240));
                mem.set_bk_mode(TRANSPARENT);
                mem.text_out(50, 170, "WinAPI++ GDI");
            }

            mem.set_text_color(RGB(200, 200, 200));
            mem.text_out(50, 220, "双缓冲绘制，不闪烁");

            mem.rectangle(full);
            mem.line(0, 0, width, height);
            mem.ellipse(box);

            bb.value().flush(0, 0);
        }

        ::EndPaint(self.native(), &ps);
        return 0;
    });

    w.value().show();
    return w.value().run();
}
