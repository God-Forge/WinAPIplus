// WinAPI++ 文件对象
//
// 设计要点（见 Plan0 §6.6 精神）：
//   - RAII 持有 HANDLE，move-only。
//   - 入参一律 UTF-8 string_view，内部转 UTF-16。
//   - 读文本自动处理 UTF-8 BOM 与 UTF-16 BOM。
#pragma once

#include <winpp/compat/config.hpp>
#include <winpp/compat/export.hpp>
#include <winpp/compat/shims.hpp>
#include <winpp/core/error.hpp>
#include <winpp/core/handle.hpp>
#include <winpp/core/result.hpp>
#include <winpp/core/traits.hpp>

#include <string>
#include <utility>
#include <vector>

namespace winpp {
namespace fs {

enum class access {
    read,
    write,
    read_write
};

class WINPP_API file {
public:
    file() noexcept : h_() {}
    explicit file(HANDLE h) noexcept : h_(h) {}

    file(file&& other) noexcept : h_(std::move(other.h_)) {}
    file& operator=(file&& other) noexcept {
        if (this != &other) h_ = std::move(other.h_);
        return *this;
    }

    file(const file&) = delete;
    file& operator=(const file&) = delete;

    ~file() {}

    bool   valid() const noexcept { return h_.valid(); }
    HANDLE native() const noexcept { return h_.get(); }
    void   close() noexcept { h_.reset(); }

    // 读整个文件为 UTF-8 文本（自动识别 BOM）。
    WINPP_NODISCARD Result<std::string> read_text() const;

    // 读整个文件为原始字节。
    WINPP_NODISCARD Result<std::vector<unsigned char> > read_all() const;

    // 写入 UTF-8 文本。
    WINPP_NODISCARD Result<void> write(string_view data);

    WINPP_NODISCARD Result<unsigned long long> size() const;

private:
    winpp::file_handle h_;
};

// 打开文件。write 模式使用 CREATE_ALWAYS（不存在则创建、存在则截断）。
WINPP_NODISCARD WINPP_API Result<file> open(string_view path, access acc);

}  // namespace fs
}  // namespace winpp

#if defined(WINPP_HEADER_ONLY)
#  include <winpp/detail/fs.inl>
#endif
