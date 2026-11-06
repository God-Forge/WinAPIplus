// WinAPI++ 通用 RAII 句柄
//
// 设计要点（见 Plan0 §6.3）：
//   - 一个模板 + 若干 traits，避免手写 N 个 RAII 类。
//   - 移动后源对象自动置为 invalid；不可拷贝。
//   - get() 是逃生通道：随时能拿回原始句柄直接调 Win32。
#pragma once

#include <winpp/compat/config.hpp>

namespace winpp {

template <class Traits>
class basic_handle {
public:
    typedef typename Traits::value_type value_type;

    basic_handle() noexcept : h_(Traits::invalid()) {}

    explicit basic_handle(value_type h) noexcept : h_(h) {}

    basic_handle(basic_handle&& other) noexcept : h_(other.release()) {}

    basic_handle& operator=(basic_handle&& other) noexcept {
        if (this != &other) reset(other.release());
        return *this;
    }

    basic_handle(const basic_handle&) = delete;
    basic_handle& operator=(const basic_handle&) = delete;

    ~basic_handle() { reset(); }

    bool valid() const noexcept { return h_ != Traits::invalid(); }

    // 逃生通道
    value_type get() const noexcept { return h_; }

    // 放弃所有权
    value_type release() noexcept {
        const value_type h = h_;
        h_ = Traits::invalid();
        return h;
    }

    void reset(value_type h = Traits::invalid()) noexcept {
        if (valid()) Traits::close(h_);
        h_ = h;
    }

    explicit operator bool() const noexcept { return valid(); }

private:
    value_type h_;
};

}  // namespace winpp
