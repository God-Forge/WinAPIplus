// WinAPI++ 进程
//
// 设计要点（见 Plan0 §6.6）：
//   - run() 同步执行并可选捕获 stdout / stderr。
//   - spawn() 异步启动，可 wait / kill。
//   - 命令行一律 UTF-8，内部转 UTF-16。
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

namespace winpp {
namespace sys {

struct process_options {
    string_view command_line;     // 完整命令行（含程序名）
    string_view working_dir;      // 空表示继承当前目录
    bool        capture_stdout = false;
    bool        capture_stderr = false;
};

struct process_result {
    unsigned long exit_code = 0;
    std::string   out;            // 仅 capture_stdout
    std::string   err;            // 仅 capture_stderr
};

// 同步执行，等待结束。
WINPP_NODISCARD WINPP_API Result<process_result> run(const process_options& opts);

class WINPP_API process {
public:
    process() noexcept;
    process(process&& other) noexcept;
    process& operator=(process&& other) noexcept;
    process(const process&) = delete;
    process& operator=(const process&) = delete;
    ~process();

    // 异步启动（不等待）。capture_stdout/stderr 仅对 run() 生效。
    WINPP_NODISCARD static Result<process> spawn(const process_options& opts);

    bool   valid() const noexcept;
    HANDLE native() const noexcept;

    // 等待结束并返回退出码。
    WINPP_NODISCARD Result<unsigned long> wait() noexcept;

    void kill(unsigned long exit_code = 1) noexcept;

    // spawn 时捕获到的输出（wait 之后可用）
    const std::string& captured_stdout() const noexcept;
    const std::string& captured_stderr() const noexcept;

private:
    HANDLE      hProcess_;
    HANDLE      hThread_;
    std::string out_;
    std::string err_;
};

}  // namespace sys
}  // namespace winpp

#if defined(WINPP_HEADER_ONLY)
#  include <winpp/detail/process.inl>
#endif
