#include <winpp_test.h>
#include <winpp/winpp.hpp>

#include <string>

WINPP_TEST_CASE("UTF-8 与 UTF-16 往返（中文）") {
    const std::string u8 = "你好，世界";
    const std::wstring w = winpp::to_wide(winpp::string_view(u8.data(), u8.size()));
    WINPP_CHECK(!w.empty());
    const std::string back = winpp::to_utf8(winpp::wstring_view(w.data(), w.size()));
    WINPP_CHECK_EQ(back, u8);
}

WINPP_TEST_CASE("string_view 垫片基础行为") {
    winpp::string_view sv("abc");
    WINPP_CHECK_EQ(sv.size(), 3u);
    WINPP_CHECK(sv == winpp::string_view("abc"));
    WINPP_CHECK(sv != winpp::string_view("abd"));
    WINPP_CHECK(sv.substr(1, 2) == winpp::string_view("bc"));
}

WINPP_TEST_CASE("string_view 从 std::string 构造") {
    const std::string s = "hello";
    winpp::string_view sv(s);
    WINPP_CHECK_EQ(sv.size(), 5u);
    WINPP_CHECK(sv == winpp::string_view("hello"));
}

WINPP_TEST_CASE("wide_string 适配器") {
    winpp::wide_string ws("测试");
    WINPP_CHECK(ws.c_str() != nullptr);
    WINPP_CHECK_EQ(ws.str().size(), 2u);
}
