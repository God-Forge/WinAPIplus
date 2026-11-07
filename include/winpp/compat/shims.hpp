// WinAPI++ 标准垫片（shims）
//
// 目标：公共 API 只写 winpp::string_view / winpp::span，
// 三个标准下签名逐字相同。C++17+ 直接别名到标准库，
// C++11/14 用自研只读视图顶上。
#pragma once

#include <winpp/compat/config.hpp>

#include <cstddef>
#include <string>
#include <vector>

#if WINPP_HAS_STRING_VIEW
#  include <string_view>
#endif

namespace winpp {

// ---------------------------------------------------------------
// WINPP_NODISCARD
// ---------------------------------------------------------------
#if WINPP_HAS_NODISCARD
#  define WINPP_NODISCARD [[nodiscard]]
#elif defined(__GNUC__) || defined(__clang__)
#  define WINPP_NODISCARD __attribute__((warn_unused_result))
#else
#  define WINPP_NODISCARD
#endif

// ---------------------------------------------------------------
// string_view / wstring_view
// ---------------------------------------------------------------
#if WINPP_HAS_STRING_VIEW

using std::string_view;
using std::wstring_view;

#else  // C++11/14：自研只读视图

template <class CharT>
class basic_string_view {
public:
    typedef CharT                value_type;
    typedef std::size_t          size_type;
    typedef const CharT*         const_iterator;

    static const size_type npos = static_cast<size_type>(-1);

    basic_string_view() noexcept : data_(0), size_(0) {}
    basic_string_view(const CharT* s) noexcept
        : data_(s), size_(s ? std::char_traits<CharT>::length(s) : 0) {}
    basic_string_view(const CharT* s, size_type n) noexcept : data_(s), size_(n) {}

    template <class Traits, class Alloc>
    basic_string_view(const std::basic_string<CharT, Traits, Alloc>& s) noexcept
        : data_(s.data()), size_(s.size()) {}

    const CharT* data() const noexcept { return data_; }
    size_type size() const noexcept { return size_; }
    size_type length() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }

    const_iterator begin() const noexcept { return data_; }
    const_iterator end() const noexcept { return data_ + size_; }
    const_iterator cbegin() const noexcept { return data_; }
    const_iterator cend() const noexcept { return data_ + size_; }

    const CharT& operator[](size_type i) const { return data_[i]; }
    const CharT& front() const { return data_[0]; }
    const CharT& back() const { return data_[size_ - 1]; }

    void remove_prefix(size_type n) noexcept { data_ += n; size_ -= n; }
    void remove_suffix(size_type n) noexcept { size_ -= n; }

    basic_string_view substr(size_type pos = 0, size_type n = npos) const noexcept {
        if (pos > size_) pos = size_;
        size_type rest = size_ - pos;
        if (n < rest) rest = n;
        return basic_string_view(data_ + pos, rest);
    }

    int compare(basic_string_view other) const noexcept {
        const size_type n = size_ < other.size_ ? size_ : other.size_;
        const int c = n ? std::char_traits<CharT>::compare(data_, other.data_, n) : 0;
        if (c != 0) return c;
        if (size_ < other.size_) return -1;
        if (size_ > other.size_) return 1;
        return 0;
    }

    size_type find(CharT ch, size_type pos = 0) const noexcept {
        for (size_type i = pos; i < size_; ++i) {
            if (data_[i] == ch) return i;
        }
        return npos;
    }

    std::basic_string<CharT> to_string() const {
        if (!data_) return std::basic_string<CharT>();
        return std::basic_string<CharT>(data_, size_);
    }

    friend bool operator==(basic_string_view a, basic_string_view b) noexcept {
        if (a.size_ != b.size_) return false;
        if (a.size_ == 0) return true;
        return std::char_traits<CharT>::compare(a.data_, b.data_, a.size_) == 0;
    }
    friend bool operator!=(basic_string_view a, basic_string_view b) noexcept { return !(a == b); }
    friend bool operator<(basic_string_view a, basic_string_view b) noexcept { return a.compare(b) < 0; }
    friend bool operator>(basic_string_view a, basic_string_view b) noexcept { return b < a; }
    friend bool operator<=(basic_string_view a, basic_string_view b) noexcept { return !(b < a); }
    friend bool operator>=(basic_string_view a, basic_string_view b) noexcept { return !(a < b); }

private:
    const CharT* data_;
    size_type    size_;
};

template <class CharT>
const typename basic_string_view<CharT>::size_type basic_string_view<CharT>::npos;

typedef basic_string_view<char>    string_view;
typedef basic_string_view<wchar_t> wstring_view;

#endif  // WINPP_HAS_STRING_VIEW

// ---------------------------------------------------------------
// span
// ---------------------------------------------------------------
#if WINPP_HAS_SPAN

using std::span;

#else  // C++11/14/17：自研只读视图

template <class T>
class span {
public:
    typedef T           element_type;
    typedef std::size_t size_type;

    span() noexcept : data_(0), size_(0) {}
    span(T* data, size_type n) noexcept : data_(data), size_(n) {}

    template <std::size_t N>
    span(T (&arr)[N]) noexcept : data_(arr), size_(N) {}

    T* data() const noexcept { return data_; }
    size_type size() const noexcept { return size_; }
    size_type size_bytes() const noexcept { return size_ * sizeof(T); }
    bool empty() const noexcept { return size_ == 0; }

    T* begin() const noexcept { return data_; }
    T* end() const noexcept { return data_ + size_; }

    T& operator[](size_type i) const { return data_[i]; }

private:
    T*        data_;
    size_type size_;
};

#endif  // WINPP_HAS_SPAN

}  // namespace winpp
