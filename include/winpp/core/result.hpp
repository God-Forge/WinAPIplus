// WinAPI++ 结果类型 Result<T>
//
// 设计要点（见 Plan0 §6.2 / §3.5）：
//   - 三档标准统一自研，不别名到 std::expected，避免语义分叉。
//   - 不抛异常；失败时 value() 直接 terminate（配合调试断言）。
//   - C++11 用匿名 union + 手动生命周期管理实现，无额外分配。
#pragma once

#include <winpp/compat/config.hpp>
#include <winpp/compat/shims.hpp>
#include <winpp/core/error.hpp>

#include <exception>
#include <new>
#include <type_traits>
#include <utility>

namespace winpp {

template <class T>
class Result {
public:
    typedef T value_type;

    Result(const T& v) : has_(true) { new (&storage_.value) T(v); }
    Result(T&& v)      : has_(true) { new (&storage_.value) T(std::move(v)); }
    Result(winpp::error e) : has_(false) { new (&storage_.err) winpp::error(std::move(e)); }

    Result(const Result& o) : has_(o.has_) {
        if (has_) new (&storage_.value) T(o.storage_.value);
        else      new (&storage_.err) winpp::error(o.storage_.err);
    }

    Result(Result&& o) noexcept : has_(o.has_) {
        if (has_) new (&storage_.value) T(std::move(o.storage_.value));
        else      new (&storage_.err) winpp::error(std::move(o.storage_.err));
    }

    Result& operator=(const Result& o) {
        if (this != &o) {
            if (has_ == o.has_) {
                if (has_) storage_.value = o.storage_.value;
                else      storage_.err = o.storage_.err;
            } else {
                destroy();
                has_ = o.has_;
                if (has_) new (&storage_.value) T(o.storage_.value);
                else      new (&storage_.err) winpp::error(o.storage_.err);
            }
        }
        return *this;
    }

    Result& operator=(Result&& o) noexcept {
        if (this != &o) {
            if (has_ == o.has_) {
                if (has_) storage_.value = std::move(o.storage_.value);
                else      storage_.err = std::move(o.storage_.err);
            } else {
                destroy();
                has_ = o.has_;
                if (has_) new (&storage_.value) T(std::move(o.storage_.value));
                else      new (&storage_.err) winpp::error(std::move(o.storage_.err));
            }
        }
        return *this;
    }

    ~Result() { destroy(); }

    bool has_value() const noexcept { return has_; }
    explicit operator bool() const noexcept { return has_; }

    T&       value() &       { check(); return storage_.value; }
    const T& value() const & { check(); return storage_.value; }
    T&&      value() &&      { check(); return std::move(storage_.value); }

    // 仅在失败时有效
    const winpp::error& error() const noexcept { return storage_.err; }

    template <class U>
    T value_or(U&& fallback) const {
        return has_ ? storage_.value : static_cast<T>(std::forward<U>(fallback));
    }

    // 链式变换：成功时把值交给 f，失败时原样传递错误。
    template <class F>
    auto map(F&& f) const
        -> Result<typename std::decay<decltype(f(std::declval<const T&>()))>::type> {
        typedef typename std::decay<decltype(f(std::declval<const T&>()))>::type U;
        if (has_) return Result<U>(f(storage_.value));
        return Result<U>(storage_.err);
    }

    // 链式衔接：f 自己返回 Result<U>，失败短路。
    template <class F>
    auto and_then(F&& f) const -> decltype(f(std::declval<const T&>())) {
        typedef decltype(f(std::declval<const T&>())) R;
        if (has_) return f(storage_.value);
        return R(storage_.err);
    }

private:
    void check() const {
        if (!has_) std::terminate();
    }

    void destroy() noexcept {
        if (has_) storage_.value.~T();
        else      storage_.err.~error();
    }

    union Storage {
        T            value;
        winpp::error err;
        Storage() {}
        ~Storage() {}
    };

    Storage storage_;
    bool    has_;
};

// ---------------------------------------------------------------
// Result<void>
// ---------------------------------------------------------------
template <>
class Result<void> {
public:
    typedef void value_type;

    Result() : has_(true) {}
    Result(winpp::error e) : err_(std::move(e)), has_(false) {}

    bool has_value() const noexcept { return has_; }
    explicit operator bool() const noexcept { return has_; }

    void value() const { if (!has_) std::terminate(); }

    const winpp::error& error() const noexcept { return err_; }

    // void 版本只保留 and_then：衔接下一个可失败动作。
    template <class F>
    auto and_then(F&& f) const -> decltype(f()) {
        typedef decltype(f()) R;
        if (has_) return f();
        return R(err_);
    }

private:
    winpp::error err_;
    bool         has_;
};

// ---------------------------------------------------------------
// 工厂
// ---------------------------------------------------------------
template <class T>
Result<typename std::decay<T>::type> ok(T&& v) {
    return Result<typename std::decay<T>::type>(std::forward<T>(v));
}

inline Result<void> fail(winpp::error e) {
    return Result<void>(std::move(e));
}

}  // namespace winpp
