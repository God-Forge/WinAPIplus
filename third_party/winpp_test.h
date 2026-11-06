// WinAPI++ 极简测试框架（M0 临时替代 doctest）
//
// 设计：零依赖、单头文件、C++11 可用。
// 宏名刻意贴近 doctest 习惯（TEST_CASE / CHECK），将来联网后可平滑替换。
#pragma once

#include <cstddef>
#include <cstdio>
#include <vector>

namespace wtest {

struct TestCase {
    const char* name;
    void (*fn)();
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> r;
    return r;
}

inline int& failure_count() {
    static int f = 0;
    return f;
}

inline int& check_count() {
    static int c = 0;
    return c;
}

struct Registrar {
    Registrar(const char* name, void (*fn)()) {
        TestCase tc;
        tc.name = name;
        tc.fn = fn;
        registry().push_back(tc);
    }
};

inline void fail(const char* file, int line, const char* expr) {
    std::printf("    [失败] %s:%d\n           断言: %s\n", file, line, expr);
    ++failure_count();
}

inline int run_all() {
    const std::size_t total = registry().size();
    std::printf("运行 %d 个测试用例\n\n", static_cast<int>(total));

    for (std::size_t i = 0; i < total; ++i) {
        const int before = failure_count();
        registry()[i].fn();
        const bool ok = (failure_count() == before);
        std::printf("%s %s\n", ok ? "[通过]" : "[失败]", registry()[i].name);
    }

    std::printf("\n断言总数: %d，失败: %d\n", check_count(), failure_count());
    return failure_count() == 0 ? 0 : 1;
}

}  // namespace wtest

#define WINPP_TEST_CONCAT_(a, b) a##b
#define WINPP_TEST_CONCAT(a, b) WINPP_TEST_CONCAT_(a, b)

#define WINPP_TEST_CASE(name)                                                \
    static void WINPP_TEST_CONCAT(winpp_test_fn_, __LINE__)();               \
    static ::wtest::Registrar WINPP_TEST_CONCAT(winpp_test_reg_, __LINE__)(  \
        name, &WINPP_TEST_CONCAT(winpp_test_fn_, __LINE__));                 \
    static void WINPP_TEST_CONCAT(winpp_test_fn_, __LINE__)()

#define WINPP_CHECK(expr)                                                    \
    do {                                                                     \
        ++::wtest::check_count();                                            \
        if (!(expr)) ::wtest::fail(__FILE__, __LINE__, #expr);               \
    } while (0)

#define WINPP_CHECK_EQ(a, b) WINPP_CHECK((a) == (b))
#define WINPP_CHECK_NE(a, b) WINPP_CHECK((a) != (b))
