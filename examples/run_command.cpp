// M3 示例：执行命令并捕获输出
#include <winpp/winpp.hpp>

#include <cstdio>

int main(int argc, char** argv) {
    winpp::sys::process_options opts;
    opts.command_line = (argc > 1) ? winpp::string_view(argv[1]) : winpp::string_view("cmd /c echo hello from WinAPI++");
    opts.capture_stdout = true;
    opts.capture_stderr = true;

    auto r = winpp::sys::run(opts);
    if (!r) {
        std::printf("执行失败：%s\n", r.error().message().c_str());
        return 1;
    }

    std::printf("退出码: %lu\n", r.value().exit_code);
    std::printf("stdout: %s\n", r.value().out.c_str());
    if (!r.value().err.empty()) {
        std::printf("stderr: %s\n", r.value().err.c_str());
    }
    return 0;
}
