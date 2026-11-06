#include <winpp_test.h>
#include <winpp/winpp.hpp>

#include <string>
#include <utility>

WINPP_TEST_CASE("Result<int> 成功路径") {
    winpp::Result<int> r(42);
    WINPP_CHECK(r.has_value());
    WINPP_CHECK(static_cast<bool>(r));
    WINPP_CHECK_EQ(r.value(), 42);
}

WINPP_TEST_CASE("Result<int> 失败路径") {
    winpp::Result<int> r(winpp::error(5, "测试上下文"));
    WINPP_CHECK(!r.has_value());
    WINPP_CHECK(!static_cast<bool>(r));
    WINPP_CHECK_EQ(r.error().code, 5u);
}

WINPP_TEST_CASE("Result<std::string> 移动构造与析构") {
    winpp::Result<std::string> a(std::string("你好"));
    winpp::Result<std::string> b(std::move(a));
    WINPP_CHECK(b.has_value());
    WINPP_CHECK_EQ(b.value(), std::string("你好"));
}

WINPP_TEST_CASE("Result<std::string> 赋值切换状态") {
    winpp::Result<std::string> r(std::string("甲"));
    r = winpp::Result<std::string>(winpp::error(7));
    WINPP_CHECK(!r.has_value());
    WINPP_CHECK_EQ(r.error().code, 7u);

    r = winpp::Result<std::string>(std::string("乙"));
    WINPP_CHECK(r.has_value());
    WINPP_CHECK_EQ(r.value(), std::string("乙"));
}

WINPP_TEST_CASE("Result<void> 成功与失败") {
    winpp::Result<void> good;
    WINPP_CHECK(good.has_value());
    WINPP_CHECK(static_cast<bool>(good));

    winpp::Result<void> bad(winpp::error(2));
    WINPP_CHECK(!bad);
    WINPP_CHECK_EQ(bad.error().code, 2u);
}

WINPP_TEST_CASE("ok() / fail() 工厂") {
    auto a = winpp::ok(7);
    WINPP_CHECK(a.has_value());
    WINPP_CHECK_EQ(a.value(), 7);

    auto b = winpp::fail(winpp::error(1));
    WINPP_CHECK(!b);
}

WINPP_TEST_CASE("value_or 回退值") {
    winpp::Result<int> good(10);
    WINPP_CHECK_EQ(good.value_or(99), 10);

    winpp::Result<int> bad(winpp::error(3));
    WINPP_CHECK_EQ(bad.value_or(99), 99);
}
