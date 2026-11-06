#include <winpp_test.h>
#include <winpp/winpp.hpp>

#include <utility>

WINPP_TEST_CASE("COM：初始化与空指针") {
    auto r = winpp::com::initialize();
    WINPP_CHECK(static_cast<bool>(r));
    if (!r) return;

    winpp::com::com_ptr<IUnknown> p;
    WINPP_CHECK(!p);
    WINPP_CHECK(p.get() == nullptr);

    IUnknown** slot = p.put();
    WINPP_CHECK(slot != nullptr);
    WINPP_CHECK(*slot == nullptr);

    winpp::com::uninitialize();
}

WINPP_TEST_CASE("COM：移动语义") {
    winpp::com::com_ptr<IUnknown> a;
    winpp::com::com_ptr<IUnknown> b(std::move(a));
    WINPP_CHECK(!b);
    WINPP_CHECK(a.get() == nullptr);
}
