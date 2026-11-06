// WinAPI++ 冒烟示例：验证库能被正常包含与链接
#include <winpp/winpp.hpp>

#include <cstdio>
#include <string>

int main() {
    auto r = winpp::ok(42);
    std::printf("Result: %d\n", r.value());

    const std::string s = winpp::to_utf8(winpp::wstring_view(L"你好，WinAPI++"));
    std::printf("UTF-8: %s\n", s.c_str());

    const winpp::error e(2);
    std::printf("错误消息: %s\n", e.message().c_str());
    return 0;
}
