// M2 示例：读取文件并打印
#include <winpp/winpp.hpp>

#include <cstdio>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("用法: read_file <路径>\n");
        return 1;
    }

    auto f = winpp::fs::open(argv[1], winpp::fs::access::read);
    if (!f) {
        std::printf("打开失败：%s\n", f.error().message().c_str());
        return 1;
    }

    auto text = f.value().read_text();
    if (!text) {
        std::printf("读取失败：%s\n", text.error().message().c_str());
        return 1;
    }

    std::printf("%s", text.value().c_str());
    return 0;
}
