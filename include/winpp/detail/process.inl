// 唯一实现源：进程与管道
#include <winpp/sys/process.hpp>
#include <winpp/core/check.hpp>
#include <winpp/core/string.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <windows.h>

#include <string>
#include <thread>
#include <utility>

namespace winpp {
namespace sys {
namespace detail {

struct pipe_pair {
    HANDLE read;
    HANDLE write;
};

WINPP_INLINE Result<pipe_pair> make_pipe() {
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = nullptr;
    sa.bInheritHandle = TRUE;

    pipe_pair p;
    p.read = nullptr;
    p.write = nullptr;
    if (!::CreatePipe(&p.read, &p.write, &sa, 0)) return winpp::last_error("CreatePipe");
    ::SetHandleInformation(p.read, HANDLE_FLAG_INHERIT, 0);
    return p;
}

WINPP_INLINE std::string drain(HANDLE h) {
    std::string out;
    char buf[4096];
    DWORD n = 0;
    while (::ReadFile(h, buf, sizeof(buf), &n, nullptr) && n > 0) {
        out.append(buf, n);
    }
    return out;
}

WINPP_INLINE void close_quietly(HANDLE& h) noexcept {
    if (h != nullptr && h != INVALID_HANDLE_VALUE) {
        ::CloseHandle(h);
        h = nullptr;
    }
}

}  // namespace detail

WINPP_INLINE Result<process_result> run(const process_options& opts) {
    process_result result;

    detail::pipe_pair out_pipe;
    out_pipe.read = nullptr;
    out_pipe.write = nullptr;
    detail::pipe_pair err_pipe;
    err_pipe.read = nullptr;
    err_pipe.write = nullptr;

    if (opts.capture_stdout) {
        WINPP_TRY(p, detail::make_pipe());
        out_pipe = p;
    }
    if (opts.capture_stderr) {
        WINPP_TRY(p, detail::make_pipe());
        err_pipe = p;
    }

    std::wstring cmdline = winpp::to_wide(opts.command_line);
    std::wstring wdir = winpp::to_wide(opts.working_dir);

    STARTUPINFOW si;
    ::ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    if (out_pipe.write != nullptr || err_pipe.write != nullptr) {
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdInput = ::GetStdHandle(STD_INPUT_HANDLE);
        si.hStdOutput = out_pipe.write ? out_pipe.write : ::GetStdHandle(STD_OUTPUT_HANDLE);
        si.hStdError = err_pipe.write
                           ? err_pipe.write
                           : (out_pipe.write ? out_pipe.write : ::GetStdHandle(STD_ERROR_HANDLE));
    }

    PROCESS_INFORMATION pi;
    ::ZeroMemory(&pi, sizeof(pi));

    const BOOL ok = ::CreateProcessW(
        nullptr,
        &cmdline[0],
        nullptr,
        nullptr,
        TRUE,
        CREATE_NO_WINDOW,
        nullptr,
        wdir.empty() ? nullptr : &wdir[0],
        &si,
        &pi);

    // 父进程不再需要写端，关掉后子进程退出时读端才能读到 EOF。
    detail::close_quietly(out_pipe.write);
    detail::close_quietly(err_pipe.write);

    if (!ok) {
        detail::close_quietly(out_pipe.read);
        detail::close_quietly(err_pipe.read);
        return winpp::last_error("CreateProcessW");
    }

    // 两个管道各开一个线程读，避免子进程输出过多时互相阻塞。
    std::thread t_out;
    std::thread t_err;
    if (out_pipe.read != nullptr) {
        t_out = std::thread([&result, &out_pipe]() {
            result.out = detail::drain(out_pipe.read);
        });
    }
    if (err_pipe.read != nullptr) {
        t_err = std::thread([&result, &err_pipe]() {
            result.err = detail::drain(err_pipe.read);
        });
    }

    ::WaitForSingleObject(pi.hProcess, INFINITE);

    if (t_out.joinable()) t_out.join();
    if (t_err.joinable()) t_err.join();

    DWORD code = 0;
    ::GetExitCodeProcess(pi.hProcess, &code);
    result.exit_code = static_cast<unsigned long>(code);

    ::CloseHandle(pi.hThread);
    ::CloseHandle(pi.hProcess);
    detail::close_quietly(out_pipe.read);
    detail::close_quietly(err_pipe.read);

    return result;
}

WINPP_INLINE process::process() noexcept : hProcess_(nullptr), hThread_(nullptr) {}

WINPP_INLINE process::process(process&& other) noexcept
    : hProcess_(other.hProcess_),
      hThread_(other.hThread_),
      out_(std::move(other.out_)),
      err_(std::move(other.err_)) {
    other.hProcess_ = nullptr;
    other.hThread_ = nullptr;
}

WINPP_INLINE process& process::operator=(process&& other) noexcept {
    if (this != &other) {
        if (hProcess_ != nullptr) ::CloseHandle(hProcess_);
        if (hThread_ != nullptr) ::CloseHandle(hThread_);
        hProcess_ = other.hProcess_;
        hThread_ = other.hThread_;
        out_ = std::move(other.out_);
        err_ = std::move(other.err_);
        other.hProcess_ = nullptr;
        other.hThread_ = nullptr;
    }
    return *this;
}

WINPP_INLINE process::~process() {
    if (hProcess_ != nullptr) ::CloseHandle(hProcess_);
    if (hThread_ != nullptr) ::CloseHandle(hThread_);
}

WINPP_INLINE bool process::valid() const noexcept { return hProcess_ != nullptr; }

WINPP_INLINE HANDLE process::native() const noexcept { return hProcess_; }

WINPP_INLINE Result<process> process::spawn(const process_options& opts) {
    std::wstring cmdline = winpp::to_wide(opts.command_line);
    std::wstring wdir = winpp::to_wide(opts.working_dir);

    STARTUPINFOW si;
    ::ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi;
    ::ZeroMemory(&pi, sizeof(pi));

    const BOOL ok = ::CreateProcessW(nullptr, &cmdline[0], nullptr, nullptr, TRUE,
                                     CREATE_NO_WINDOW, nullptr,
                                     wdir.empty() ? nullptr : &wdir[0], &si, &pi);
    if (!ok) return winpp::last_error("CreateProcessW");

    process p;
    p.hProcess_ = pi.hProcess;
    p.hThread_ = pi.hThread;
    return p;
}

WINPP_INLINE Result<unsigned long> process::wait() noexcept {
    if (hProcess_ == nullptr) return winpp::error(6u, "process::wait");
    ::WaitForSingleObject(hProcess_, INFINITE);
    DWORD code = 0;
    if (!::GetExitCodeProcess(hProcess_, &code)) return winpp::last_error("GetExitCodeProcess");
    return static_cast<unsigned long>(code);
}

WINPP_INLINE void process::kill(unsigned long exit_code) noexcept {
    if (hProcess_ != nullptr) ::TerminateProcess(hProcess_, static_cast<UINT>(exit_code));
}

WINPP_INLINE const std::string& process::captured_stdout() const noexcept { return out_; }
WINPP_INLINE const std::string& process::captured_stderr() const noexcept { return err_; }

}  // namespace sys
}  // namespace winpp
