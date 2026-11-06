// WinAPI++ COM 智能指针
//
// 轻量 ComPtr：只做引用计数管理，不追求与 WRL 等价。
#pragma once

#include <winpp/compat/config.hpp>
#include <winpp/core/error.hpp>
#include <winpp/core/result.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <windows.h>
#include <unknwn.h>

namespace winpp {
namespace com {

// 初始化 COM（STA）。重复调用是安全的。
inline Result<void> initialize() {
    const HRESULT hr = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        return winpp::error(static_cast<unsigned long>(hr), "CoInitializeEx");
    }
    return Result<void>();
}

inline void uninitialize() {
    ::CoUninitialize();
}

template <class T>
class com_ptr {
public:
    com_ptr() noexcept : p_(nullptr) {}
    explicit com_ptr(T* p) noexcept : p_(p) {}

    com_ptr(com_ptr&& other) noexcept : p_(other.p_) { other.p_ = nullptr; }

    com_ptr& operator=(com_ptr&& other) noexcept {
        if (this != &other) {
            reset();
            p_ = other.p_;
            other.p_ = nullptr;
        }
        return *this;
    }

    com_ptr(const com_ptr&) = delete;
    com_ptr& operator=(const com_ptr&) = delete;

    ~com_ptr() { reset(); }

    T*  get() const noexcept { return p_; }
    T** put() noexcept { reset(); return &p_; }
    T*  operator->() const noexcept { return p_; }
    explicit operator bool() const noexcept { return p_ != nullptr; }

    void reset(T* p = nullptr) noexcept {
        if (p_ != nullptr) p_->Release();
        p_ = p;
    }

    T* detach() noexcept {
        T* p = p_;
        p_ = nullptr;
        return p;
    }

private:
    T* p_;
};

// 按 CLSID + IID 创建对象
template <class T>
WINPP_NODISCARD Result<com_ptr<T>> create(REFCLSID clsid, REFIID iid) {
    com_ptr<T> p;
    const HRESULT hr = ::CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER,
                                          iid, reinterpret_cast<void**>(p.put()));
    if (FAILED(hr)) {
        return winpp::error(static_cast<unsigned long>(hr), "CoCreateInstance");
    }
    return p;
}

}  // namespace com
}  // namespace winpp
