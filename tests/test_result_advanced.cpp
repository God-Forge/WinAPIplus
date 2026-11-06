#include <winpp_test.h>
#include <winpp/winpp.hpp>

#include <string>

// ---------------------------------------------------------------
// map / and_then
// ---------------------------------------------------------------
WINPP_TEST_CASE("map：成功时变换值") {
    auto r = winpp::ok(21).map([](int v) { return v * 2; });
    WINPP_CHECK(r.has_value());
    WINPP_CHECK_EQ(r.value(), 42);
}

WINPP_TEST_CASE("map：失败时原样传递错误") {
    winpp::Result<int> bad(winpp::error(7, "map 测试"));
    auto r = bad.map([](int v) { return v * 2; });
    WINPP_CHECK(!r);
    WINPP_CHECK_EQ(r.error().code, 7u);
}

WINPP_TEST_CASE("map：改变值类型") {
    auto r = winpp::ok(123).map([](int v) { return std::to_string(v); });
    WINPP_CHECK(r.has_value());
    WINPP_CHECK_EQ(r.value(), std::string("123"));
}

WINPP_TEST_CASE("and_then：成功时衔接") {
    auto r = winpp::ok(10).and_then([](int v) -> winpp::Result<std::string> {
        return winpp::ok(std::to_string(v + 1));
    });
    WINPP_CHECK(r.has_value());
    WINPP_CHECK_EQ(r.value(), std::string("11"));
}

WINPP_TEST_CASE("and_then：前一步失败则短路") {
    bool called = false;
    auto r = winpp::Result<int>(winpp::error(3))
                 .and_then([&called](int v) -> winpp::Result<int> {
                     called = true;
                     return winpp::ok(v);
                 });
    WINPP_CHECK(!called);
    WINPP_CHECK(!r);
    WINPP_CHECK_EQ(r.error().code, 3u);
}

WINPP_TEST_CASE("and_then：后一步失败也传递错误") {
    auto r = winpp::ok(1).and_then([](int) -> winpp::Result<int> {
        return winpp::Result<int>(winpp::error(11));
    });
    WINPP_CHECK(!r);
    WINPP_CHECK_EQ(r.error().code, 11u);
}

// ---------------------------------------------------------------
// WINPP_TRY
// ---------------------------------------------------------------
namespace {

winpp::Result<int> halve(int v) {
    if (v % 2 != 0) return winpp::error(100, "halve");
    return winpp::ok(v / 2);
}

winpp::Result<int> quarter(int v) {
    WINPP_TRY(h, halve(v));
    WINPP_TRY(q, halve(h));
    return winpp::ok(q);
}

winpp::Result<void> require_positive(int v) {
    if (v <= 0) return winpp::error(101, "require_positive");
    return winpp::Result<void>();
}

winpp::Result<int> checked_double(int v) {
    WINPP_TRY_VOID(require_positive(v));
    return winpp::ok(v * 2);
}

}  // namespace

WINPP_TEST_CASE("WINPP_TRY：成功链路") {
    auto r = quarter(8);
    WINPP_CHECK(r.has_value());
    WINPP_CHECK_EQ(r.value(), 2);
}

WINPP_TEST_CASE("WINPP_TRY：中途短路并保留错误码") {
    auto r = quarter(6);  // 6 -> 3，第二次 halve 失败
    WINPP_CHECK(!r);
    WINPP_CHECK_EQ(r.error().code, 100u);
}

WINPP_TEST_CASE("WINPP_TRY：Result<void> 也能传播") {
    WINPP_CHECK(checked_double(5).has_value());
    auto bad = checked_double(-1);
    WINPP_CHECK(!bad);
    WINPP_CHECK_EQ(bad.error().code, 101u);
}

// ---------------------------------------------------------------
// Result<void>::and_then
// ---------------------------------------------------------------
WINPP_TEST_CASE("Result<void> and_then：成功衔接") {
    auto r = winpp::Result<void>().and_then([]() -> winpp::Result<int> {
        return winpp::ok(5);
    });
    WINPP_CHECK(r.has_value());
    WINPP_CHECK_EQ(r.value(), 5);
}

WINPP_TEST_CASE("Result<void> and_then：失败短路") {
    auto r = winpp::fail(winpp::error(13)).and_then([]() -> winpp::Result<int> {
        return winpp::ok(5);
    });
    WINPP_CHECK(!r);
    WINPP_CHECK_EQ(r.error().code, 13u);
}
