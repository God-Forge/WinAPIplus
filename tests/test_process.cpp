#include <winpp_test.h>
#include <winpp/winpp.hpp>

#include <string>

WINPP_TEST_CASE("进程：捕获 stdout") {
    winpp::sys::process_options opts;
    opts.command_line = "cmd /c echo hello";
    opts.capture_stdout = true;

    auto r = winpp::sys::run(opts);
    WINPP_CHECK(static_cast<bool>(r));
    if (r) {
        WINPP_CHECK(r.value().out.find("hello") != std::string::npos);
        WINPP_CHECK_EQ(r.value().exit_code, 0u);
    }
}

WINPP_TEST_CASE("进程：退出码传递") {
    winpp::sys::process_options opts;
    opts.command_line = "cmd /c exit 3";

    auto r = winpp::sys::run(opts);
    WINPP_CHECK(static_cast<bool>(r));
    if (r) WINPP_CHECK_EQ(r.value().exit_code, 3u);
}

WINPP_TEST_CASE("进程：捕获 stderr") {
    winpp::sys::process_options opts;
    opts.command_line = "cmd /c echo oops 1>&2";
    opts.capture_stderr = true;

    auto r = winpp::sys::run(opts);
    WINPP_CHECK(static_cast<bool>(r));
    if (r) WINPP_CHECK(r.value().err.find("oops") != std::string::npos);
}

WINPP_TEST_CASE("进程：spawn 后 wait") {
    winpp::sys::process_options opts;
    opts.command_line = "cmd /c exit 0";

    auto p = winpp::sys::process::spawn(opts);
    WINPP_CHECK(static_cast<bool>(p));
    if (p) {
        WINPP_CHECK(p.value().valid());
        auto code = p.value().wait();
        WINPP_CHECK(static_cast<bool>(code));
        if (code) WINPP_CHECK_EQ(code.value(), 0u);
    }
}

WINPP_TEST_CASE("进程：不存在的命令返回错误") {
    winpp::sys::process_options opts;
    opts.command_line = "no_such_program_xyz_12345";

    auto r = winpp::sys::run(opts);
    WINPP_CHECK(!r);
}
