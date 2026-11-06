// M4 示例：最小窗口 + 控件
#include <winpp/winpp.hpp>

#include <cstdio>

int main() {
    winpp::ui::enable_dpi_awareness();

    winpp::ui::window_options opts;
    opts.title = "WinAPI++ 示例窗口";
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

    auto btn = winpp::ui::button::create(w.value().native(), "按钮", 1001, 20, 20, 120, 32);
    auto ed = winpp::ui::edit::create(w.value().native(), "输入点什么", 1002, 20, 70, 200, 28);

    (void)btn;
    (void)ed;

    w.value().show();
    return w.value().run();
}
