#include <winpp_test.h>
#include <winpp/winpp.hpp>

#include <string>

WINPP_TEST_CASE("文件：写入再读回（UTF-8 中文）") {
    const std::string path = "test_fs_roundtrip.txt";

    {
        auto f = winpp::fs::open(path, winpp::fs::access::write);
        WINPP_CHECK(static_cast<bool>(f));
        if (f) {
            auto w = f.value().write("你好，WinAPI++");
            WINPP_CHECK(static_cast<bool>(w));
        }
    }

    {
        auto f = winpp::fs::open(path, winpp::fs::access::read);
        WINPP_CHECK(static_cast<bool>(f));
        if (f) {
            auto text = f.value().read_text();
            WINPP_CHECK(static_cast<bool>(text));
            if (text) WINPP_CHECK_EQ(text.value(), std::string("你好，WinAPI++"));

            auto sz = f.value().size();
            WINPP_CHECK(static_cast<bool>(sz));
            if (sz) WINPP_CHECK(sz.value() > 0);
        }
    }

    WINPP_CHECK(winpp::fs::exists(path));
    WINPP_CHECK(static_cast<bool>(winpp::fs::remove(path)));
    WINPP_CHECK(!winpp::fs::exists(path));
}

WINPP_TEST_CASE("文件：打开不存在的文件返回错误") {
    auto f = winpp::fs::open("definitely_not_here_12345.txt", winpp::fs::access::read);
    WINPP_CHECK(!f);
    if (!f) WINPP_CHECK(winpp::is_not_found(f.error()));
}

WINPP_TEST_CASE("路径：join 拼接规则") {
    WINPP_CHECK_EQ(winpp::fs::join("C:\\a", "b"), std::string("C:\\a\\b"));
    WINPP_CHECK_EQ(winpp::fs::join("C:\\a\\", "b"), std::string("C:\\a\\b"));
    WINPP_CHECK_EQ(winpp::fs::join("C:\\a", "D:\\b"), std::string("D:\\b"));
    WINPP_CHECK_EQ(winpp::fs::join("", "b"), std::string("b"));
    WINPP_CHECK_EQ(winpp::fs::join("C:\\a", ""), std::string("C:\\a"));
}

WINPP_TEST_CASE("路径：full_path 展开为绝对路径") {
    auto p = winpp::fs::full_path(".");
    WINPP_CHECK(static_cast<bool>(p));
    if (p) WINPP_CHECK(!p.value().empty());
}

WINPP_TEST_CASE("路径：known_folder 返回非空") {
    // CSIDL_APPDATA = 0x001a
    auto p = winpp::fs::known_folder(0x001a);
    WINPP_CHECK(static_cast<bool>(p));
    if (p) WINPP_CHECK(!p.value().empty());
}
