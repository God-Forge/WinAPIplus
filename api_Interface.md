# WinAPI++ API 参考

> 版本：v0.1 · 对应设计文档：Plan0 v0.8 · 最后更新：2026-11-06
>
> 本文档只描述**已实现并通过测试**的接口。规划中的接口见 Plan0 §11 里程碑。

---

## 0. 快速索引

| 头文件 | 内容 | 里程碑 |
| --- | --- | --- |
| `<winpp/winpp.hpp>` | 总入口，包含以下全部 | — |
| `<winpp/compat/config.hpp>` | 标准检测宏 | M0 |
| `<winpp/compat/export.hpp>` | `WINPP_API` / `WINPP_INLINE` | M0 |
| `<winpp/compat/shims.hpp>` | `string_view` / `span` / `WINPP_NODISCARD` | M0 |
| `<winpp/core/error.hpp>` | `winpp::error`、`format_message`、`last_error` | M0 |
| `<winpp/core/result.hpp>` | `Result<T>`、`ok()`、`fail()` | M0/M1 |
| `<winpp/core/check.hpp>` | `WINPP_TRY` / `WINPP_TRY_VOID` | M1 |
| `<winpp/core/handle.hpp>` | `basic_handle<Traits>` | M0 |
| `<winpp/core/traits.hpp>` | 句柄 traits 与别名 | M0 |
| `<winpp/core/string.hpp>` | `to_utf8` / `to_wide` / `wide_string` | M0 |
| `<winpp/fs/file.hpp>` | `fs::file`、`fs::open` | M2 |
| `<winpp/fs/path.hpp>` | 路径工具 | M2 |
| `<winpp/sys/process.hpp>` | `sys::run`、`sys::process` | M3 |

---

## 1. 使用约定

### 1.1 头文件

```cpp
#include <winpp/winpp.hpp>   // 一般只用这一个
```

需要控制编译开销时，也可以只包含具体子头，例如 `<winpp/fs/file.hpp>`。

### 1.2 命名空间

| 命名空间 | 用途 |
| --- | --- |
| `winpp` | 核心类型（`error`、`Result`、句柄、字符串） |
| `winpp::fs` | 文件与路径 |
| `winpp::sys` | 进程、管道 |

### 1.3 错误处理约定

**所有可能失败的操作都返回 `Result<T>`，不抛异常。**

```cpp
auto r = winpp::fs::open("a.txt", winpp::fs::access::read);
if (!r) {
    // r.error() 是 winpp::error
    std::printf("%s\n", r.error().message().c_str());
    return;
}
auto& f = r.value();
```

- `Result<T>` 转换到 `bool` 判断成败。
- `error::message()` 是**延迟格式化**：只有真正调用时才调 `FormatMessageW`。
- 失败时调用 `value()` 会 `std::terminate()`，调试期能立刻定位。

### 1.4 字符串与编码

| 规则 | 说明 |
| --- | --- |
| 入参 | 一律 `winpp::string_view`（UTF-8） |
| 返回 | 一律 `std::string`（UTF-8） |
| 与 Win32 交界 | 内部统一转 UTF-16，调用方不需要看见 `wchar_t` |
| 源文件编码 | UTF-8 无 BOM；MSVC 需 `/utf-8`（CMake 已自动加） |

### 1.5 标准与编译器支持

| 编译器 | 标准 | 说明 |
| --- | --- | --- |
| MSVC（VS2022+） | 仅 C++17 | `/std:c++17` |
| MinGW-w64 GCC | C++11 / C++17 / C++23 | 要求 GCC ≥ 11、UCRT、posix 线程模型 |

公共 API 在三个标准下**签名逐字相同**；C++11 下由 `winpp::` 垫片提供 `string_view` 等。

---

## 2. 宏与配置

### 2.1 导出与内联

| 宏 | 含义 | 定义条件 |
| --- | --- | --- |
| `WINPP_API` | 符号导出/导入 | header-only 为空；静态库为空；DLL 下为 `dllexport`/`dllimport` |
| `WINPP_INLINE` | 实现片段内联 | header-only 为 `inline`；库模式为空 |

用户一般不需要直接使用这两个宏，除非自己写实现片段。

### 2.2 标准检测（`config.hpp`）

| 宏 | 含义 |
| --- | --- |
| `WINPP_CPLUSPLUS` | 实际标准版本号（MSVC 优先读 `_MSVC_LANG`） |
| `WINPP_CXX_STD` | 归一化后的 `11` / `17` / `23` |
| `WINPP_HAS_STRING_VIEW` | 是否有 `std::string_view` |
| `WINPP_HAS_SPAN` | 是否有 `std::span`（C++20+） |
| `WINPP_HAS_EXPECTED` | 是否有 `std::expected`（C++23） |
| `WINPP_HAS_NODISCARD` | 是否支持 `[[nodiscard]]` |
| `WINPP_HAS_IF_CONSTEXPR` | 是否支持 `if constexpr` |
| `WINPP_HAS_STRUCTURED_BIND` | 是否支持结构化绑定 |

### 2.3 属性宏

```cpp
#define WINPP_NODISCARD   // C++17+ 为 [[nodiscard]]；GCC 为 __attribute__((warn_unused_result))
```

### 2.4 错误传播宏（`check.hpp`）

#### `WINPP_TRY(var, expr)`

执行 `expr`（必须返回 `Result<T>`）：
- 失败 → 把 `error` 直接返回给调用者；
- 成功 → 把值移动构造到 `var`。

```cpp
winpp::Result<int> quarter(int v) {
    WINPP_TRY(h, halve(v));      // h 是 int
    WINPP_TRY(q, halve(h));
    return winpp::ok(q);
}
```

#### `WINPP_TRY_VOID(expr)`

用于 `Result<void>`，只做失败短路，不绑定变量：

```cpp
winpp::Result<int> checked_double(int v) {
    WINPP_TRY_VOID(require_positive(v));
    return winpp::ok(v * 2);
}
```

---

## 3. 核心类型

### 3.1 `winpp::error`

```cpp
struct error {
    unsigned long   code = 0;    // Win32 错误码（0 表示无 Win32 来源）
    std::error_code sys;         // 可选的 std::error_code
    string_view     context;     // 出错位置，如 "CreateFileW"

    error();
    error(unsigned long c, string_view ctx = string_view());
    explicit error(std::error_code ec, string_view ctx = string_view());

    WINPP_NODISCARD std::string message() const;
    explicit operator bool() const noexcept;
};
```

| 成员 | 说明 |
| --- | --- |
| `code` | Win32 错误码（`GetLastError()` 的返回值） |
| `sys` | 标准库错误码，适用于 `std::filesystem` 等场景 |
| `context` | 出错位置标签，便于日志定位 |
| `message()` | 格式化为 UTF-8 文本；延迟调用 `FormatMessageW` |
| `operator bool` | `code != 0` 或 `sys` 非零时为真 |

```cpp
winpp::error e(2, "CreateFileW");
std::printf("%s\n", e.message().c_str());   // 系统找不到指定的文件。
```

#### 自由函数

```cpp
WINPP_NODISCARD WINPP_API std::string format_message(unsigned long code);
WINPP_NODISCARD WINPP_API error last_error(string_view context = string_view()) noexcept;
```

- `format_message(code)`：把 Win32 错误码格式化成 UTF-8。
- `last_error(ctx)`：捕获当前线程的 `GetLastError()`。

#### 错误码判定

```cpp
bool is_not_found(const error& e) noexcept;      // code == 2   (ERROR_FILE_NOT_FOUND)
bool is_access_denied(const error& e) noexcept;  // code == 5   (ERROR_ACCESS_DENIED)
bool is_already_exists(const error& e) noexcept; // code == 183 (ERROR_ALREADY_EXISTS)
```

### 3.2 `winpp::Result<T>`

```cpp
template <class T>
class Result {
public:
    Result(const T& v);
    Result(T&& v);
    Result(winpp::error e);

    Result(const Result&);
    Result(Result&&) noexcept;
    Result& operator=(const Result&);
    Result& operator=(Result&&) noexcept;

    bool has_value() const noexcept;
    explicit operator bool() const noexcept;

    T&       value() &;
    const T& value() const &;
    T&&      value() &&;

    const winpp::error& error() const noexcept;
    template <class U> T value_or(U&& fallback) const;

    template <class F> auto map(F&& f) const -> Result<U>;
    template <class F> auto and_then(F&& f) const -> R;
};
```

| 方法 | 说明 |
| --- | --- |
| `has_value()` / `operator bool` | 是否成功 |
| `value()` | 取值的三种重载；失败时 `terminate` |
| `error()` | 取错误；仅在失败时有效 |
| `value_or(fallback)` | 失败时返回回退值 |
| `map(f)` | 成功时把值交给 `f` 变换，失败原样传递 |
| `and_then(f)` | 成功时调用返回 `Result<U>` 的 `f`，失败短路 |

```cpp
auto r = winpp::ok(21).map([](int v) { return std::to_string(v * 2); });
// r.value() == "42"

auto q = winpp::ok(10).and_then([](int v) -> winpp::Result<int> {
    return winpp::ok(v + 1);
});
// q.value() == 11
```

#### `Result<void>`

```cpp
template <> class Result<void> {
public:
    Result();
    Result(winpp::error e);

    bool has_value() const noexcept;
    explicit operator bool() const noexcept;
    void value() const;
    const winpp::error& error() const noexcept;

    template <class F> auto and_then(F&& f) const -> R;
};
```

#### 工厂

```cpp
template <class T> Result<typename std::decay<T>::type> ok(T&& v);
inline Result<void> fail(winpp::error e);
```

```cpp
auto a = winpp::ok(7);                  // Result<int>
auto b = winpp::fail(winpp::error(1));  // Result<void>
```

### 3.3 `winpp::basic_handle<Traits>`

```cpp
template <class Traits>
class basic_handle {
public:
    typedef typename Traits::value_type value_type;

    basic_handle() noexcept;
    explicit basic_handle(value_type h) noexcept;

    basic_handle(basic_handle&&) noexcept;
    basic_handle& operator=(basic_handle&&) noexcept;
    basic_handle(const basic_handle&) = delete;
    basic_handle& operator=(const basic_handle&) = delete;
    ~basic_handle();

    bool       valid() const noexcept;
    value_type get() const noexcept;      // 逃生通道：取原始句柄
    value_type release() noexcept;        // 放弃所有权
    void       reset(value_type h = Traits::invalid()) noexcept;
    explicit   operator bool() const noexcept;
};
```

- 默认构造为「无效」状态。
- 移动后源对象自动置为无效。
- 析构时调用 `Traits::close()`。

### 3.4 句柄 traits（`traits.hpp`）

| traits | 句柄类型 | 无效值 | 关闭方式 |
| --- | --- | --- | --- |
| `file_traits` | `HANDLE` | `INVALID_HANDLE_VALUE` | `CloseHandle` |
| `event_traits` | `HANDLE` | `nullptr` | `CloseHandle` |
| `mutex_traits` | `HANDLE` | `nullptr` | `CloseHandle` |
| `semaphore_traits` | `HANDLE` | `nullptr` | `CloseHandle` |
| `module_traits` | `HMODULE` | `nullptr` | `FreeLibrary` |

对应别名：

```cpp
typedef basic_handle<file_traits>      file_handle;
typedef basic_handle<event_traits>     event_handle;
typedef basic_handle<mutex_traits>     mutex_handle;
typedef basic_handle<semaphore_traits> semaphore_handle;
typedef basic_handle<module_traits>    module_handle;
```

### 3.5 字符串转换（`string.hpp`）

```cpp
WINPP_NODISCARD WINPP_API std::string  to_utf8(wstring_view w);
WINPP_NODISCARD WINPP_API std::wstring to_wide(string_view u8);
```

```cpp
class wide_string {
public:
    explicit wide_string(string_view u8);
    const wchar_t* c_str() const noexcept;
    operator const wchar_t*() const noexcept;   // 可直接传给 Win32
    const std::wstring& str() const noexcept;
};
```

```cpp
const std::string u8 = winpp::to_utf8(winpp::wstring_view(L"你好"));
winpp::wide_string ws("你好");
::MessageBoxW(nullptr, ws, L"标题", MB_OK);   // 隐式转换
```

> `wide_string` 持有转换结果，`c_str()` 的生命周期跟随对象本身。

---

## 4. 文件系统（`winpp::fs`）

### 4.1 `access`

```cpp
enum class access {
    read,        // GENERIC_READ,     OPEN_EXISTING（必须存在）
    write,       // GENERIC_WRITE,    CREATE_ALWAYS（不存在则创建、存在则截断）
    read_write   // GENERIC_READ | GENERIC_WRITE, OPEN_ALWAYS
};
```

### 4.2 `file`

```cpp
class WINPP_API file {
public:
    file() noexcept;
    explicit file(HANDLE h) noexcept;

    file(file&&) noexcept;
    file& operator=(file&&) noexcept;
    file(const file&) = delete;
    file& operator=(const file&) = delete;

    bool   valid() const noexcept;
    HANDLE native() const noexcept;
    void   close() noexcept;

    WINPP_NODISCARD Result<std::string>                read_text() const;
    WINPP_NODISCARD Result<std::vector<unsigned char>> read_all() const;
    WINPP_NODISCARD Result<void>                       write(string_view data);
    WINPP_NODISCARD Result<unsigned long long>         size() const;
};
```

| 方法 | 说明 |
| --- | --- |
| `read_text()` | 读全部内容为 UTF-8；自动识别 UTF-8 / UTF-16 LE / UTF-16 BE BOM |
| `read_all()` | 读全部内容为原始字节 |
| `write(data)` | 从当前文件位置写入 UTF-8 字节 |
| `size()` | 文件大小（字节） |

### 4.3 `open`

```cpp
WINPP_NODISCARD WINPP_API Result<file> open(string_view path, access acc);
```

```cpp
auto f = winpp::fs::open("config.json", winpp::fs::access::read);
if (!f) {
    std::printf("打开失败：%s\n", f.error().message().c_str());
    return;
}
auto text = f.value().read_text();
if (text) std::printf("%s", text.value().c_str());
```

> 注意：`file` 对象在作用域内会持有句柄，**删除文件前必须先让它析构或 `close()`**，否则 `DeleteFileW` 会失败（共享模式未包含 `FILE_SHARE_DELETE`）。

### 4.4 路径工具

```cpp
WINPP_NODISCARD WINPP_API std::string join(string_view a, string_view b);
WINPP_NODISCARD WINPP_API Result<std::string> full_path(string_view p);
WINPP_NODISCARD WINPP_API bool exists(string_view p);
WINPP_NODISCARD WINPP_API Result<void> remove(string_view p);
WINPP_NODISCARD WINPP_API Result<std::string> known_folder(int csidl);
```

| 函数 | 说明 |
| --- | --- |
| `join(a, b)` | 拼接路径并补反斜杠；`b` 为绝对路径时直接返回 `b` |
| `full_path(p)` | 展开为绝对路径（`GetFullPathNameW`） |
| `exists(p)` | 文件或目录是否存在 |
| `remove(p)` | 删除文件（`DeleteFileW`） |
| `known_folder(csidl)` | 取系统已知文件夹，如 `0x001a`（`CSIDL_APPDATA`） |

```cpp
winpp::fs::join("C:\\a", "b");       // "C:\\a\\b"
winpp::fs::join("C:\\a", "D:\\b");   // "D:\\b"

auto appdata = winpp::fs::known_folder(0x001a);  // CSIDL_APPDATA
```

---

## 5. 进程（`winpp::sys`）

### 5.1 `process_options`

```cpp
struct process_options {
    string_view command_line;     // 完整命令行（含程序名）
    string_view working_dir;      // 空表示继承当前目录
    bool        capture_stdout = false;
    bool        capture_stderr = false;
};
```

### 5.2 `process_result`

```cpp
struct process_result {
    unsigned long exit_code = 0;
    std::string   out;   // 仅 capture_stdout 时有效
    std::string   err;   // 仅 capture_stderr 时有效
};
```

### 5.3 `run`

```cpp
WINPP_NODISCARD WINPP_API Result<process_result> run(const process_options& opts);
```

同步执行命令并等待结束；内部用两个线程分别读 stdout/stderr，避免管道互相阻塞。

```cpp
winpp::sys::process_options opts;
opts.command_line   = "cmd /c echo hello";
opts.capture_stdout = true;

auto r = winpp::sys::run(opts);
if (r) {
    std::printf("退出码 %lu，输出：%s\n", r.value().exit_code, r.value().out.c_str());
}
```

### 5.4 `process`

```cpp
class WINPP_API process {
public:
    process() noexcept;
    process(process&&) noexcept;
    process& operator=(process&&) noexcept;
    process(const process&) = delete;
    process& operator=(const process&) = delete;
    ~process();

    WINPP_NODISCARD static Result<process> spawn(const process_options& opts);

    bool   valid() const noexcept;
    HANDLE native() const noexcept;

    WINPP_NODISCARD Result<unsigned long> wait() noexcept;
    void kill(unsigned long exit_code = 1) noexcept;

    const std::string& captured_stdout() const noexcept;
    const std::string& captured_stderr() const noexcept;
};
```

```cpp
auto p = winpp::sys::process::spawn(opts);
if (p) {
    auto code = p.value().wait();
    if (code) std::printf("退出码 %lu\n", code.value());
}
```

> `spawn` 目前**不支持**捕获输出：`capture_stdout` / `capture_stderr` 只对 `run()` 生效。

---

## 6. 兼容层（`winpp::compat`）

用户代码通常不需要直接碰这一层，但以下别名是公开的：

| 别名 | C++17+ | C++11/14 |
| --- | --- | --- |
| `winpp::string_view` | `std::string_view` | 自研只读视图 |
| `winpp::wstring_view` | `std::wstring_view` | 自研只读视图 |
| `winpp::span<T>` | `std::span`（C++20+） | 自研只读视图 |

自研 `string_view` 提供：`data` / `size` / `length` / `empty` / `begin` / `end` / `operator[]` / `front` / `back` / `substr` / `find` / `compare` / `to_string` / 比较运算符。

> `string_view` 不拥有内存，只接受「活得比它久」的源。

---

## 7. 错误码参考

| 错误码 | 含义 | 判定函数 |
| --- | --- | --- |
| 2 | `ERROR_FILE_NOT_FOUND` | `is_not_found()` |
| 5 | `ERROR_ACCESS_DENIED` | `is_access_denied()` |
| 6 | `ERROR_INVALID_HANDLE` | — |
| 183 | `ERROR_ALREADY_EXISTS` | `is_already_exists()` |

`message()` 在 `FormatMessageW` 失败时会返回 `"未知错误（Win32 错误码 N）"`。

---

## 8. 已知限制

| 限制 | 说明 | 后续计划 |
| --- | --- | --- |
| `run` 输出编码 | 按原始字节返回，不做代码页转换。cmd 内置命令输出 OEM 代码页，中文直接按 UTF-8 打印会乱码 | 可加 `codepage` 选项或 `oem_to_utf8()` |
| `spawn` 不捕获输出 | `capture_*` 仅对 `run` 生效 | 后续版本 |
| `known_folder` 用 CSIDL | `SHGetFolderPathW`，未使用 `KNOWNFOLDERID` | 后续增强 |
| 文件共享模式 | 固定 `FILE_SHARE_READ`，不支持共享删除 | 后续可配置 |
| 未实现层 | 窗口、GDI、注册表、服务、COM（见 Plan0 §11） | M4 起 |

---

## 9. 构建集成

当前推荐用 `add_subdirectory` 引入：

```cmake
add_subdirectory(third_party/WinAPI++)
target_link_libraries(app PRIVATE WinAPI++::winpp)
```

构建时选择分发形态：

```bash
cmake -S . -B build -DWINPP_LIBRARY_TYPE=static    # header-only | static | shared
```

> **安装导出（`find_package(WinAPI++ CONFIG)`）尚未实现**：按 Plan0 Q8 的决策，当前只发源码，M6 前后再补安装与包配置。
>
> 动态库模式要求 DLL 与 EXE 使用同一工具链、同一 CRT（`/MD`）、同一 Debug/Release 配置。

---

## 10. 版本记录

| 版本 | 日期 | 变更 |
| --- | --- | --- |
| v0.1 | 2026-11-06 | 初版，覆盖 M0–M3 已实现接口 |
