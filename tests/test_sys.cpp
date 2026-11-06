#include <winpp_test.h>
#include <winpp/winpp.hpp>

#include <string>

// ---------------------------------------------------------------
// 注册表
// ---------------------------------------------------------------
WINPP_TEST_CASE("注册表：读取已知键") {
    auto k = winpp::sys::key::open(HKEY_CURRENT_USER, "Software", KEY_READ);
    WINPP_CHECK(static_cast<bool>(k));
}

WINPP_TEST_CASE("注册表：写入、读取、删除（受限环境自动跳过）") {
    const char* subkey = "Software\\WinAPPTests\\UnitTest";

    auto k = winpp::sys::key::create(HKEY_CURRENT_USER, subkey, KEY_ALL_ACCESS);
    if (!k) {
        // 部分受限进程只能读注册表，写会被拒绝（ERROR_ACCESS_DENIED）
        WINPP_CHECK(true);
        return;
    }

    WINPP_CHECK(static_cast<bool>(k.value().set_string("Name", "值")));

    auto get = k.value().get_string("Name");
    WINPP_CHECK(static_cast<bool>(get));
    if (get) WINPP_CHECK_EQ(get.value(), std::string("值"));

    WINPP_CHECK(static_cast<bool>(k.value().set_dword("Count", 42)));
    auto gd = k.value().get_dword("Count");
    WINPP_CHECK(static_cast<bool>(gd));
    if (gd) WINPP_CHECK_EQ(gd.value(), 42u);

    k = winpp::sys::key();

    auto parent = winpp::sys::key::open(HKEY_CURRENT_USER, "Software\\WinAPPTests", KEY_ALL_ACCESS);
    if (parent) {
        WINPP_CHECK(static_cast<bool>(parent.value().remove_tree("UnitTest")));
    }
}

WINPP_TEST_CASE("注册表：打开不存在的键返回错误") {
    auto k = winpp::sys::key::open(HKEY_CURRENT_USER, "Software\\WinAPPTests\\NoSuchKey_12345");
    WINPP_CHECK(!k);
}

// ---------------------------------------------------------------
// 环境变量
// ---------------------------------------------------------------
WINPP_TEST_CASE("环境变量：设置、读取、删除") {
    const char* name = "WINPP_TEST_VAR";

    WINPP_CHECK(static_cast<bool>(winpp::sys::set_env(name, "hello")));

    auto v = winpp::sys::get_env(name);
    WINPP_CHECK(static_cast<bool>(v));
    if (v) WINPP_CHECK_EQ(v.value(), std::string("hello"));

    WINPP_CHECK(static_cast<bool>(winpp::sys::unset_env(name)));

    auto gone = winpp::sys::get_env(name);
    WINPP_CHECK(!gone);
}

WINPP_TEST_CASE("环境变量：expand_env 展开引用") {
    WINPP_CHECK(static_cast<bool>(winpp::sys::set_env("WINPP_EXPAND_BASE", "D:\\demo")));
    const std::string r = winpp::sys::expand_env("%WINPP_EXPAND_BASE%\\file.txt");
    WINPP_CHECK_EQ(r, std::string("D:\\demo\\file.txt"));
    WINPP_CHECK(static_cast<bool>(winpp::sys::unset_env("WINPP_EXPAND_BASE")));
}

// ---------------------------------------------------------------
// 时间
// ---------------------------------------------------------------
WINPP_TEST_CASE("时间：本地时间合理") {
    const winpp::sys::system_time t = winpp::sys::local_time();
    WINPP_CHECK(t.year >= 2000);
    WINPP_CHECK(t.month >= 1 && t.month <= 12);
    WINPP_CHECK(t.day >= 1 && t.day <= 31);

    const std::string s = winpp::sys::format_time(t);
    WINPP_CHECK_EQ(s.size(), 19u);
}

WINPP_TEST_CASE("时间：Unix 毫秒时间戳") {
    const unsigned long long ms = winpp::sys::now_unix_ms();
    WINPP_CHECK(ms > 1700000000000ull);  // 2023 年之后
}

// ---------------------------------------------------------------
// 剪贴板（无桌面会话时自动跳过）
// ---------------------------------------------------------------
WINPP_TEST_CASE("剪贴板：写入后读回") {
    const std::string text = "WinAPI++ 剪贴板测试";

    auto set = winpp::ui::set_clipboard_text(text);
    if (!set) {
        WINPP_CHECK(true);  // 无桌面会话，跳过
        return;
    }

    auto get = winpp::ui::clipboard_text();
    WINPP_CHECK(static_cast<bool>(get));
    if (get) WINPP_CHECK_EQ(get.value(), text);
}
