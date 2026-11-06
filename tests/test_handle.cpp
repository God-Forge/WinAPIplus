#include <winpp_test.h>
#include <winpp/winpp.hpp>

#include <string>
#include <utility>

WINPP_TEST_CASE("句柄默认无效") {
    winpp::file_handle h;
    WINPP_CHECK(!h.valid());
    WINPP_CHECK(!h);
    WINPP_CHECK(h.get() == INVALID_HANDLE_VALUE);
}

WINPP_TEST_CASE("句柄移动后源失效") {
    winpp::file_handle a;
    winpp::file_handle b(std::move(a));
    WINPP_CHECK(!b.valid());
    WINPP_CHECK(!a.valid());
}

WINPP_TEST_CASE("error::message() 能格式化 Win32 错误码") {
    const winpp::error e(2, "test");
    const std::string msg = e.message();
    WINPP_CHECK(!msg.empty());
}

WINPP_TEST_CASE("last_error() 捕获错误码") {
    ::SetLastError(1234);
    const winpp::error e = winpp::last_error("test");
    WINPP_CHECK_EQ(e.code, 1234u);
}

WINPP_TEST_CASE("error 布尔语义") {
    winpp::error none;
    WINPP_CHECK(!static_cast<bool>(none));

    winpp::error bad(5);
    WINPP_CHECK(static_cast<bool>(bad));
    WINPP_CHECK(winpp::is_access_denied(bad));
}
