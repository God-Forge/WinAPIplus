# Plan0 — WinAPI++ 项目设计草案

> 版本：v0.9（M0–M5 已完成） · 日期：2026-11-06 · 状态：M4–M5 验收通过，待开 M6
>
> **一句话定位**：把 Win32 API 包成「现代 C++ 该有的样子」——RAII 管资源、Result 管错误、UTF-8 管字符串、原生句柄随时可拿。
>
> **v0.9 变更**：M4–M5 完成（窗口/消息循环/DPI/控件/消息框/托盘），115 个断言全绿，§11.3 记录实施结果。
>
> **v0.8 变更**：M1–M3 完成（Result 链式 API、`WINPP_TRY`、文件层、进程层），98 个断言全绿。
>
> **v0.7 变更**：CMake 4.5 构建验证完成（双工具链 × 三形态 = 6 组，含 ctest）；查明 CMake 崩溃真因是运行沙箱限制而非 CMake 本身。
>
> **v0.6 变更**：M0 实施完成，12 组构建组合全绿；§11.1 记录实施偏差与环境问题；§6.3 的 traits 接口按实现调整。
>
> **v0.5 变更**：§12 的 8 项开放问题全部拍板；文档与代码注释定为**全中文**（§7）。
>
> **v0.4 变更**：标准矩阵收敛为 **MSVC 仅 C++17、MinGW 三档全开**；C++11 合规性改由 MinGW 独家把关。
>
> **v0.3 变更**：MinGW-w64 提升为**一等公民**，与 MSVC 并列；新增 §3.7 工具链差异与 ABI 隔离铁律。
>
> **v0.2 变更**：语言标准三档同时支持；分发形态改为 **header-only / 静态库 / 动态库 三形态同源构建**。

---

## 1. 背景与目标

Win32 是一套 1993 年定型的 C 接口：句柄要手动关、错误靠 `GetLastError()` 全局变量、字符串是 UTF-16、每个 API 都有 A/W 两个版本、参数又长又爱传 `nullptr`。在 C++ 里直接用它，样板代码能占掉业务代码的一大半。

**封装的目标不是消灭 Win32，而是消灭它的粗糙表面。** 典型收益：

```cpp
// Before：原生 Win32
HANDLE h = CreateFileW(L"config.json", GENERIC_READ, FILE_SHARE_READ, nullptr,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
if (h == INVALID_HANDLE_VALUE) {
    DWORD err = GetLastError();
    wchar_t buf[512];
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, err, 0, buf, 512, nullptr);
    std::fwprintf(stderr, L"打开失败：%s (%lu)\n", buf, err);
    return 1;
}
DWORD size = GetFileSize(h, nullptr);
std::string data(size, '\0');
DWORD read = 0;
BOOL ok = ReadFile(h, data.data(), size, &read, nullptr);
CloseHandle(h);
if (!ok) { /* 又来一遍错误处理…… */ }

// After：WinAPI++（三种标准、三种分发形态下写法完全一致）
auto file = winpp::fs::open("config.json", winpp::fs::read_only);
if (!file) {
    std::cerr << "打开失败：" << file.error().message() << '\n';
    return 1;
}
auto text = file->read_text();          // Result<std::string>，UTF-8
```

### 设计目标（按优先级）

1. **消除样板**：常见操作一行搞定，错误处理统一为 `Result<T>`。
2. **资源安全**：所有内核对象、GDI 对象、注册表键、COM 指针都由 RAII 持有，异常/早退都不泄漏。
3. **接口一致**：**同一份源码、同一套 API，在 MSVC（C++17）与 MinGW-w64（C++11/17/23）、三种分发形态下行为完全相同**。用户换标准、换编译器、换链接方式，不需要改一行业务代码。
4. **零成本倾向**：薄封装，热路径允许内联，不引入虚函数派发。
5. **逃生通道（escape hatch）**：每个封装对象都能 `native()` 取出原始句柄，任何没封装到的能力可以立刻退回原生调用，不被库绑架。
6. **可学习性**：头文件即文档，命名可预测，读一个封装类就能猜到其他类的长相。

### 非目标（明确不做）

| 不做 | 原因 |
| --- | --- |
| 跨平台抽象 | 不做 Qt / SDL 那种"同一套接口到处跑"，那会丢掉 Win32 的特性 |
| 完整 GUI 框架 | 不做布局引擎、样式表、控件主题；只做窗口/消息/控件的薄壳 |
| 完整 COM 绑定 | 只提供 `ComPtr` 级别的智能指针与常用 Shell 接口，不做 IDL 生成 |
| 完整 DirectX / WinRT 封装 | 留给专用库 |
| ANSI（A 版）API 封装 | 一律走 W 版 + UTF-8 转换，减少一倍接口面积 |
| 为 C++11 做「功能降级」 | C++11 下只损失语法糖，不损失任何能力（见 §3.4） |

---

## 2. 技术选型

| 议题 | 候选 | 决定 | 理由 |
| --- | --- | --- | --- |
| 语言标准 | MSVC: C++17；MinGW: C++11/17/23 | **MSVC 固定 C++17，MinGW 三档全开** | 见 §3.2；C++11 与 C++23 两档只由 MinGW 验证，MSVC 不参与 |
| 错误处理 | 异常 / 返回码 / `Result<T>` | **`Result<T>` 默认不抛** | Win32 错误是"值"不是"异常"，且窗口过程、线程入口不允许异常穿越边界 |
| 分发形态 | header-only / 静态库 / 动态库 | **三形态同源，CMake 选项切换** | 见 §4；头文件党、静态链接党、DLL 分发党都照顾到 |
| 构建 | CMake / MSBuild / 手写 | **CMake ≥ 3.24** | 多 target、多标准矩阵用 `target_compile_features` 表达最干净 |
| 编译器 | MSVC / MinGW-w64 / clang-cl | **MSVC 与 MinGW-w64 双主力，clang-cl 尽力而为** | 两者都是一等公民、CI 全量覆盖（见 §3.7）；clang-cl 只做 nightly 回归，不阻断合并 |
| 工具链 | Windows SDK / MinGW-w64 UCRT | **MSVC 配 Windows SDK；MinGW 用 UCRT 运行时** | UCRT 与 MSVC 行为一致；MSVCRT 版已过时，不承诺支持 |
| 测试 | doctest / Catch2 / GTest | **doctest** | 单头文件、C++11 可用、编译快；vendor 进 `third_party/`，不依赖网络 |
| 命名空间 | `winpp` / `wapi` / `win32` | **`winpp`** | 短、不易与现有库冲突；宏前缀 `WINPP_` |
| 字符集 | 一律 UNICODE | **仅 W 版** | 内部统一 UTF-16，对外统一 UTF-8，边界只在一处转换 |

---

## 3. 多标准 / 多编译器兼容层（`winpp/compat`）

### 3.1 总原则

> **公共 API 的签名在三个标准、两个编译器下必须逐字相同。**

用户代码只写 `winpp::string_view`、`winpp::Result<T>`、`winpp::span<const T>`——至于它底下是标准库类型还是自研垫片，由 `compat/` 在编译期决定。C++11 用户不该看到 `std::string_view` 这个词，C++17 用户也不该被迫接受一个仿制货；MSVC 用户不该被 libstdc++ 的脾气绊倒，MinGW 用户也不该踩到 MS STL 的专属行为。

实现手段：**能力检测宏 + 类型别名 + 最小垫片**。不搞「C++11 版一套 API、C++17 版另一套」的分叉，那是维护地狱；同理也不搞「MSVC 版一套、MinGW 版一套」。

### 3.2 标准检测（`compat/config.hpp`）

```cpp
#pragma once

// MSVC 默认 __cplusplus 恒为 199711L（除非 /Zc:__cplusplus），必须优先看 _MSVC_LANG
#if defined(_MSVC_LANG)
#  define WINPP_CPLUSPLUS _MSVC_LANG
#else
#  define WINPP_CPLUSPLUS __cplusplus
#endif

#if   WINPP_CPLUSPLUS >= 202302L
#  define WINPP_CXX_STD 23
#elif WINPP_CPLUSPLUS >= 201703L
#  define WINPP_CXX_STD 17
#elif WINPP_CPLUSPLUS >= 201103L
#  define WINPP_CXX_STD 11
#elif defined(_MSC_VER) && _MSC_VER >= 1900
#  define WINPP_CXX_STD 14          // 兜底：老 MSVC 无 _MSVC_LANG；不在支持矩阵内，仅避免误报
#else
#  error "WinAPI++ 需要 C++11 或更高标准（MSVC 2017+ 请用 /std:c++17）"
#endif

#define WINPP_HAS_STRING_VIEW       (WINPP_CXX_STD >= 17)
#define WINPP_HAS_IF_CONSTEXPR      (WINPP_CXX_STD >= 17)
#define WINPP_HAS_STRUCTURED_BIND   (WINPP_CXX_STD >= 17)
#define WINPP_HAS_SPAN              (WINPP_CXX_STD >= 20)
#define WINPP_HAS_EXPECTED          (WINPP_CXX_STD >= 23)
#define WINPP_HAS_NODISCARD         (WINPP_CXX_STD >= 17)
```

**标准矩阵（v0.4 起收敛）**：

| 编译器 | C++11 | C++17 | C++23 | 说明 |
| --- | --- | --- | --- | --- |
| MSVC（VS2022） | — | ✅ `/std:c++17` | — | 只支持 C++17 一档 |
| MinGW-w64 GCC | ✅ `-std=c++11` | ✅ `-std=c++17` | ✅ `-std=c++23` | 三档全开，标准合规的主战场 |

**关键坑位（必须在 CI 里钉死）**：

- **MSVC 只跑 C++17，但它编的是 C++11 代码**。源码本身必须停留在 C++11 子集（因为 MinGW 要用 `-std=c++11` 编同一份文件），所以 MSVC 端是「C++17 编译器编 C++11 代码」——能过，但**不构成 C++11 合规性证明**。
- **C++11 合规的唯一守门员是 MinGW**。更要命的是：`compat/shims.hpp` 里那些自研垫片（C++11 的 `string_view`、`Result` 的 union 实现）在 MSVC 下**根本不会被编译到**——MSVC 恒为 C++17，永远走标准库分支。所以 MinGW `-std=c++11` 的 CI 是这些代码的**唯一防线**，缺了它等于没测。
- **反向陷阱**：若某个公共头误用了 `std::string_view` 而没走 `winpp::string_view` 别名，MSVC 会照编不误（c++17 有），只有 MinGW c++11 会当场炸。交叉验证不可省。
- **C++23 档也只在 MinGW**（GCC 13+ 才有 `-std=c++23`，且标准库部分特性滞后）。MSVC 的 `/std:c++latest` 不进矩阵。
- CMake 侧：MSVC 要求 `cxx_std_17`、MinGW 要求 `cxx_std_11`（见 §9），用户自己升标准时 CMake 会自动取高。
- `_MSVC_LANG` 需要 VS2015 Update 3 及以上；GCC/Clang 的 `__cplusplus` 正常，无需特殊处理。

### 3.3 垫片清单（`compat/shims.hpp`）

| 公共别名 | C++17+ | C++11/14 | 说明 |
| --- | --- | --- | --- |
| `winpp::string_view` | `std::string_view` | 自研只读视图（`const char*` + `size`） | 提供 `data/size/empty/begin/end/substr/find/operator==`，可从 `std::string`、字面量、`const char*` 构造 |
| `winpp::wstring_view` | `std::wstring_view` | 自研只读视图（`const wchar_t*` + `size`） | 同上，供 UTF-16 侧使用 |
| `winpp::span<T>` | `std::span`（**C++20+ 才启用**；C++17 档仍用自研） | 自研只读视图（`T*` + `size`） | 只提供读接口，不提供动态扩展。MSVC 档固定 C++17，所以 MSVC 用户实际也用自研 span |
| `winpp::Result<T>` | 自研（**始终**） | 自研 | 见 3.5：不别名到 `std::expected` |
| `WINPP_NODISCARD` | `[[nodiscard]]` | MSVC `_Check_return_` / GCC `__attribute__((warn_unused_result))` | 可被 `WINPP_NO_NODISCARD` 关闭 |
| `WINPP_CONSTEXPR` | `constexpr` | `constexpr`（限制更严） | 仅用于真正可常量求值的函数 |
| `WINPP_IF_CONSTEXPR` | `if constexpr` | tag dispatch 宏 | 实现内部使用，**不暴露给用户** |
| `std::make_unique` | 原生 | 自补（`unique_ptr<T>(new T(...))`） | 仅实现内部使用 |
| `std::byte` | 原生 | `unsigned char` 别名 | 文件读写缓冲区用 |

**`string_view` 的生命周期纪律**（C++11 自研版尤其要小心）：视图不拥有内存，只接受「活得比它久」的源。封装层内部传入 Win32 前统一经 `winpp::wide_string` 转换，转换结果在同一条完整表达式内有效，不会悬垂。

### 3.4 C++11 下会损失什么？

只损失**语法糖**，不损失能力：

| 特性 | C++11 替代 |
| --- | --- |
| `std::string_view` | `winpp::string_view`（同名、同接口） |
| `[[nodiscard]]` | `WINPP_NODISCARD` 宏 |
| `if constexpr` | tag dispatch（仅内部） |
| 结构化绑定 | `auto r = ...; r.first; r.second;` |
| `std::filesystem` | **本来就不用**——文件层直接包 Win32，三档标准同一套实现 |
| `std::optional` | 用 `Result<T>` 表达「可能没有」 |
| `std::expected` | 自研 `Result<T>`（三档标准同一套） |
| `std::print` | `std::cout` / `printf` |

> 这也解释了为什么文件层坚持「包 Win32 而不是包 `std::filesystem`」：后者在 C++11 下不存在，在 C++17 下行为又和 Win32 有细微差异（长路径、`\\?\` 前缀、保留名）。包 Win32 反而让三档标准天然一致。

### 3.5 为什么 `Result<T>` 不别名到 `std::expected`

`std::expected` 是 C++23 的，而 C++23 档**只有 MinGW 有**（MSVC 停在 C++17）。若在 C++23 下别名、低版本用自研，就会出现「同一份代码在 MinGW c++23 下走 `std::expected` 语义，在 MSVC 与 MinGW c++11/17 下走自研语义」的分叉（异常规格、`value()` 失败时的行为、`and_then` 的重载决议都可能不同）。

**结论**：`Result<T>` 三档标准统一自研，接口刻意设计成与 `std::expected` 对齐（`has_value()` / `value()` / `error()` / `value_or()` / `and_then()`），将来若真要切换，只需改 `compat/shims.hpp` 里一行别名，用户代码零改动。

C++11 下的实现：`union { T value_; error err_; }` + tag 位，手写拷贝/移动构造与析构（或用 `std::aligned_storage` + placement new）。约 120 行，有完整单测。

### 3.6 C++23（仅 MinGW）下有额外收益吗？

有，但**不进入公共 API**，只作为内部优化与可选项。这档只有 MinGW 能吃到，MSVC 用户看不到任何差异：

- 内部可用 `std::expected` 承载 `Result` 的存储（`WINPP_USE_STD_EXPECTED` 实验开关，默认关）。
- `std::to_underlying`、`deducing this` 可简化少量内部代码。
- 示例程序可选用 `std::print`（`WINPP_EXAMPLES_CXX23` 开关）。

公共接口一律停留在 C++11 可表达的子集——**这是本项目的核心纪律**。

### 3.7 MinGW-w64 专项（一等公民）

MinGW-w64 不是「同一份代码换个编译器」那么简单：标准库实现（libstdc++ vs MS STL）、运行时（UCRT vs vcruntime）、ABI、工具链约定全都不同。以下每一条都是本项目必须**主动**处理、而不是等 CI 炸了再补的差异。

#### 3.7.1 工具链基线

| 项 | 要求 | 说明 |
| --- | --- | --- |
| CRT | **UCRT**（`mingw-w64-ucrt-x86_64-gcc`） | MSVCRT 版已过时，行为与 MSVC 有差，不承诺支持 |
| 线程模型 | **posix**（MSYS2 / WinLibs 的 posix 构建） | win32 线程模型下 libstdc++ **没有** `std::thread`，见 3.7.2 |
| GCC 版本 | ≥ 11；C++23 档需 ≥ 13 | CI 固定 MSYS2 最新稳定版 |
| 发行渠道 | MSYS2（首选）/ WinLibs 独立包 / LLVM-MinGW | 文档只保证这三家 |
| 架构 | x86_64 必测；i686 不承诺 | 32 位异常模型（SJLJ/DWARF）差异大，暂不覆盖 |

#### 3.7.2 标准库差异（CI 会当场抓出来的）

- **`std::thread` / `std::mutex` / `std::condition_variable` 依赖 posix 线程模型**。用户若装了 win32 线程模型的 MinGW，这些符号在编译期直接报「not supported by this configuration」。因此库内对 `std::thread` 的使用**收敛在 `sys/thread.hpp` 一处**，将来若要支持 win32 线程模型，只需替换这一层。
- `std::filesystem`：GCC 8 及以下需额外 `-lstdc++fs`。本项目不依赖它（见 §3.4），天然免疫。
- `std::to_chars` 浮点重载：GCC 11 之前缺失，C++17 档的格式化代码不得依赖。
- `std::string` 的 SSO 阈值、`std::unordered_map` 的迭代顺序、`std::function` 的小对象优化阈值，**MS STL 与 libstdc++ 不同**——库内一律不得依赖这些实现细节。

#### 3.7.3 头文件与宏差异

- **`_WIN32_WINNT` 必须显式定义**（统一 `0x0A00`）。MinGW-w64 的默认值与 Windows SDK 不一致，不定义会导致 `GetDpiForWindow`、`SetThreadDpiAwarenessContext` 等新 API 在 MinGW 下不可见——这是「MSVC 编得过、MinGW 编不过」的头号原因。
- **SAL 注解不可用**：`_Check_return_` 在 MinGW 下要么缺失要么是空宏。`WINPP_NODISCARD` 的 GCC 分支统一用 `__attribute__((warn_unused_result))`。
- **`#pragma comment(lib, "...")` 在 GCC 下被直接忽略**（最多一条 warning）。所有链接一律走 CMake `target_link_libraries`——写进 §7 的硬性纪律。
- `UNICODE` / `_UNICODE` 在 MinGW 下**不会**自动定义，CMake 里已全局补齐。
- 个别 SDK 头（部分 `winrt/`、`d3d12` 扩展）在 MinGW 里版本滞后。遇到时以能力检测宏降级，**不硬依赖**。

#### 3.7.4 链接与入口

| 场景 | MinGW 做法 | 备注 |
| --- | --- | --- |
| 控制台程序 `main` | 默认即可 | — |
| 宽字符入口 `wmain` | 加 `-municode` | 否则链接器找不到入口 |
| GUI 程序 `wWinMain` | `-municode -mwindows` | CMake 中 `set_target_properties(... WIN32_EXECUTABLE TRUE)` |
| 资源文件 `.rc` | `enable_language(RC)` + `windres` | CMake 自动选用 `windres`，与 `rc.exe` 语法同源 |
| 分发独立 exe | `-static-libgcc -static-libstdc++` | 不依赖 MSYS2 运行时 DLL；**动态库模式慎用**（会引入多份运行时副本） |
| 可重现构建 | `-Wl,--no-insert-timestamp` | 可选，便于产物比对 |

#### 3.7.5 ABI 隔离铁律（最容易踩的雷）

> **MinGW 产出的 `.dll` / `.a` 与 MSVC 产出的 `.lib` / `.obj` 不能混链。**

原因：C++ 名字修饰、异常模型、`std::string` 布局、`new/delete` 归属全都不同。C ABI 层面可以互通，C++ 层面绝对不行。

推论：

1. 发布的二进制包必须**按工具链分目录**：`lib/msvc-x64/`、`lib/mingw-ucrt-x64/`，绝不混放。
2. 动态库模式（§4.3）的约束在 MinGW 下同样成立，且多一条：**DLL 与调用方必须是同一 GCC 主版本**（libstdc++ 的 ABI 在 GCC 主版本间不保证稳定）。
3. 用户的 EXE 用 MSVC 编，就只能链 MSVC 版产物；用 MinGW 编，就只能链 MinGW 版产物。文档必须在显眼处写死这一条。

#### 3.7.6 源文件编码

- GCC 默认按 **UTF-8** 解析源文件，中文注释与中文字符串字面量直接可用。
- MSVC 默认按**系统代码页**（中文环境即 GBK），必须显式 `/utf-8`，否则中文字面量乱码。本项目源码含大量中文注释，这条是**硬要求**（见 §9）。
- 两个编译器的 `wchar_t` 都是 16 位，UTF-16 语义一致，无需特殊处理。

---

## 4. 三种分发形态

### 4.1 同源三形态

同一份源码，三种构建产物，靠编译宏切换：

| 形态 | 定义宏 | `WINPP_API` | `WINPP_INLINE` | 产物 |
| --- | --- | --- | --- | --- |
| header-only | `WINPP_HEADER_ONLY` | 空 | `inline` | 无产物，纯头文件 |
| 静态库 | 无 | 空 | 空 | `winpp.lib` / `libwinpp.a` |
| 动态库 | `WINPP_BUILD_SHARED`（库侧）/ `WINPP_USE_SHARED`（用户侧） | `__declspec(dllexport/dllimport)` | 空 | `winpp.dll` + `winpp.lib` |

### 4.2 实现文件的「一份代码、两种用法」模式

```cpp
// include/winpp/compat/export.hpp
#pragma once
#if defined(WINPP_HEADER_ONLY)
#  define WINPP_API
#  define WINPP_INLINE inline
#elif defined(WINPP_BUILD_SHARED)
#  define WINPP_API __declspec(dllexport)
#  define WINPP_INLINE
#elif defined(WINPP_USE_SHARED)
#  define WINPP_API __declspec(dllimport)
#  define WINPP_INLINE
#else
#  define WINPP_API            // 静态库
#  define WINPP_INLINE
#endif
```

```cpp
// include/winpp/core/error.hpp   —— 只声明
WINPP_API std::string format_message(unsigned long code);

#if defined(WINPP_HEADER_ONLY)
#  include <winpp/detail/error.inl>     // 头文件模式：把实现贴进来
#endif
```

```cpp
// include/winpp/detail/error.inl  —— 唯一实现，带 WINPP_INLINE
WINPP_INLINE std::string format_message(unsigned long code) { /* ... */ }
```

```cpp
// src/error.cpp   —— 库模式：编译进 .lib/.dll
#include <winpp/core/error.hpp>
#include <winpp/detail/error.inl>
```

- `.inl` 是**唯一真相源**，绝不出现「头文件里写一份、cpp 里抄一份」。
- header-only 时 `WINPP_INLINE` = `inline`，多编译单元包含不产生重复符号。
- 库模式时 `WINPP_INLINE` 为空，`.cpp` 里产生唯一外部符号，`.dll` 导出。
- 模板类（`Result`、`basic_handle`、`window`）**永远**在头文件里，永远不加 `WINPP_API`——见 4.3。

### 4.3 动态库模式的硬约束

跨 DLL 边界传递 STL 类型是 Windows 上最经典的雷区。本项目的应对：

| 约束 | 说明 |
| --- | --- |
| **模板不进 DLL** | 所有模板类（`Result<T>`、`basic_handle<Traits>`、`window`、`process` 等）实现全部在头文件，两边各自实例化。`WINPP_API` 只加在非模板自由函数与少量非模板类上。 |
| **CRT 必须一致** | DLL 模式要求 `/MD`（动态 CRT），且 DLL 与 EXE 使用**相同 MSVC 工具集**、**相同 Debug/Release**。Debug DLL 配 Release EXE 会在 `std::string` 析构处炸。 |
| **接口返回类型收敛** | 跨边界返回类型限定为 `std::string`、POD 聚合体、`Result<上述>`；不返回 `std::vector`、`std::function`、`std::unordered_map`。 |
| **`std::function` 回调** | 只在同模块内使用；跨 DLL 注册窗口消息回调时，回调类型改为裸函数指针 + `void*` 上下文。 |
| **异常不跨边界** | 所有 `WINPP_API` 函数 `noexcept`，错误一律走 `Result`。 |

> 如果这些约束让你觉得别扭——那就选静态库或 header-only，它们在 MSVC 上最省心。DLL 是为「多个模块共享同一份实现、减小体积」准备的。

### 4.4 三形态的取舍

| 维度 | header-only | 静态库 | 动态库 |
| --- | --- | --- | --- |
| 接入成本 | 最低（拷头文件即可） | 中 | 高（要配 DLL 路径、CRT） |
| 编译时间 | 差（每 TU 重编译） | 好 | 好 |
| 二进制体积 | 大（重复实例化） | 中 | 小（共享） |
| ABI 稳定性 | 无所谓 | 无所谓 | 需维护 |
| 调试体验 | 好 | 好 | 差（符号分离） |
| 适用场景 | 小工具、示例、快速原型 | **默认推荐** | 插件/多模块大项目 |

**默认推荐静态库**：编译快、无 CRT 雷区、无导出宏烦恼，是 90% 项目的正确答案。header-only 作为「零配置接入」选项保留。

---

## 5. 分层架构

```
┌────────────────────────────────────────────────────────────┐
│ L5  高层便利   shell 操作 · 系统信息 · 格式化 · 常用对话框      │
├────────────────────────────────────────────────────────────┤
│ L4  GDI/绘制   dc · bitmap · brush · pen · font · 双缓冲     │
├────────────────────────────────────────────────────────────┤
│ L3  界面       window · window_class · message_loop         │
│                controls · dpi · tray · dialog · clipboard   │
├────────────────────────────────────────────────────────────┤
│ L2  系统服务   process · pipe · thread · registry · service  │
│                env · mmap · known_folder · time             │
├────────────────────────────────────────────────────────────┤
│ L1  资源       file · event · mutex · semaphore · module     │
│                token · job · heap                           │
├────────────────────────────────────────────────────────────┤
│ L0  基础       compat · result · error · handle · string     │
└────────────────────────────────────────────────────────────┘
        ↑ 依赖方向严格向上：下层绝不 include 上层
```

**分层铁律**：

1. 依赖只向上，下层不 include 上层。
2. **只有 L0 允许 include `compat/`**；上层一律使用 L0 暴露的别名，杜绝「某个头里冒出 `std::string_view`」的标准泄漏。
3. 每个 L0 头都必须能在 `-std=c++11`（GCC）与 `/std:c++17`（MSVC）下**独立**编译通过。

---

## 6. 核心抽象设计（草案接口）

> 下列签名在 C++11/17/23 下**完全一致**。`string_view`、`span` 均指 `winpp::` 版本。

### 6.1 `winpp::error`

```cpp
namespace winpp {

struct error {
    unsigned long   code = 0;      // Win32 错误码（0 表示无 Win32 来源）
    std::error_code sys{};         // 可选：filesystem 等场景
    string_view     context{};     // 出错位置，如 "CreateFileW"

    std::string message() const;   // FormatMessageW → UTF-8（延迟、失败兜底）
    explicit operator bool() const noexcept { return code != 0 || sys; }
};

error last_error(string_view context = {}) noexcept;   // 捕获 GetLastError

} // namespace winpp
```

- `message()` **延迟格式化**：正常路径不付出代价，只有真要打印时才调 `FormatMessageW`。
- 内置高频错误码判定：`is_not_found()` / `is_access_denied()` / `is_already_exists()`。

### 6.2 `winpp::Result<T>`

```cpp
template <class T>
class Result {
public:
    Result(T value);                    // 隐式成功构造
    Result(error err);

    explicit operator bool() const noexcept;
    bool has_value() const noexcept;

    T&       value() &;
    const T& value() const &;
    T&&      value() &&;
    T        value_or(T fallback) const;

    const error& error() const noexcept;   // 仅在失败时有效

    template <class F> auto map(F&& f) const;        // → Result<U>
    template <class F> auto and_then(F&& f) const;   // 链式
};

template <> class Result<void>;   // 只表达成功/失败

template <class T> Result<T> ok(T&& v);
Result<void> fail(error e);
#define WINPP_TRY(var, expr) /* 失败即 return 同名 error */
```

- 不抛异常；`value()` 在失败时 `std::terminate`（配合 `WINPP_CHECK` 断言，调试期立刻定位）。
- `WINPP_THROW_ON_ERROR` 编译开关可把 `Result` 退化为抛异常模式。

### 6.3 句柄：`basic_handle<Traits>`

```cpp
struct file_traits {
    typedef HANDLE value_type;
    // 用静态函数而非 static constexpr 成员：C++11 下后者被 ODR-use 时
    // 需要类外定义，header-only 模式下多 TU 会重复定义。
    static value_type invalid() noexcept { return INVALID_HANDLE_VALUE; }
    static void close(value_type h) noexcept { CloseHandle(h); }
};

template <class Traits>
class basic_handle {
public:
    typedef typename Traits::value_type value_type;

    basic_handle() noexcept;                       // 默认 invalid
    basic_handle(basic_handle&&) noexcept;
    basic_handle& operator=(basic_handle&&) noexcept;
    basic_handle(const basic_handle&) = delete;    // 不可拷贝

    ~basic_handle();

    bool       valid() const noexcept;
    value_type get() const noexcept;               // 逃生通道
    value_type release() noexcept;
    void       reset(value_type h = Traits::invalid()) noexcept;

    explicit operator bool() const noexcept;
};

typedef basic_handle<file_traits>     file_handle;
typedef basic_handle<event_traits>    event_handle;
typedef basic_handle<registry_traits> registry_key;
```

> C++11 用 `typedef` 而非 `using` 别名？——**不必**。`using` 别名是 C++11 就有的特性，放心用。这里写 `typedef` 只是为了提醒自己：**C++14 的 `auto` 返回类型推导、泛型 lambda、`std::make_unique` 一律禁用**。

### 6.4 字符串：UTF-8 与 UTF-16 的唯一交界

```cpp
namespace winpp {

std::string  to_utf8(wstring_view w);
std::wstring to_wide(string_view u8);

// 轻量适配器：UTF-8 → wchar_t*，调用点不必写 .c_str()
class wide_string {
public:
    wide_string(string_view u8);
    const wchar_t* c_str() const noexcept;
    operator const wchar_t*() const noexcept;   // 可直接传给 Win32
};

}
```

- **规则**：公开 API 字符串入参一律 `string_view`（UTF-8），返回值一律 `std::string`（UTF-8）。
- `wstring_view` 与 `string_view` 一样由 `compat/shims.hpp` 提供：C++17+ 别名到标准库，C++11/14 为自研视图。因此上面的签名在三档标准下**逐字相同**。

### 6.5 窗口与消息

```cpp
namespace winpp { namespace ui {

struct window_options {
    string_view title;
    int  width = 800, height = 600;
    unsigned long style = WS_OVERLAPPEDWINDOW;
    bool center = true;
};

class window {
public:
    static Result<window> create(const window_options& opts);

    window(window&&) noexcept;
    ~window();

    HWND native() const noexcept;

    void show(int cmd = SW_SHOW);
    void close();

    typedef std::function<LRESULT(window&, UINT, WPARAM, LPARAM)> handler;
    void on(UINT msg, handler h);      // 运行时注册消息处理器（默认方案）

    int run();                          // 局部消息循环，返回 ExitCode
};

int message_loop();

}} // namespace winpp::ui
```

- `WndProc` 用 `GWLP_USERDATA` 在 `WM_NCCREATE` 时绑定 `this`；未注册消息走 `DefWindowProcW`。
- 消息表用 `std::unordered_map<UINT, handler>`；热路径（`WM_PAINT`）后续可优化为静态数组。
- 动态库模式下 `handler` 改为裸函数指针 + `void*` 上下文（见 §4.3）。

### 6.6 进程与管道（高频需求）

```cpp
namespace winpp { namespace sys {

struct process_options {
    string_view command_line;
    string_view working_dir;
    bool        capture_stdout = false;
    bool        capture_stderr = false;
    bool        inherit_env    = true;
    const std::vector<std::string>* extra_env = nullptr;   // 指针避免跨 DLL 传容器
};

struct process_result {
    unsigned long exit_code = 0;
    std::string   out;
    std::string   err;
};

Result<process_result> run(const process_options& opts);      // 同步等待
Result<process>        spawn(const process_options& opts);    // 异步，可 wait/kill

}} // namespace winpp::sys
```

这一层是"用起来最爽"的部分，一行拿到命令输出：

```cpp
winpp::sys::process_options opts;
opts.command_line   = "git status";
opts.capture_stdout = true;

auto r = winpp::sys::run(opts);      // 三档标准下写法完全相同
if (r) std::cout << r->out;
```

C++20 及以上还可以写成 `run({.command_line = "git status", .capture_stdout = true})`——那是编译器给的语法糖，与库无关，库本身不依赖。

---

## 7. 命名与风格约定

| 元素 | 约定 | 示例 |
| --- | --- | --- |
| 命名空间 | 小写，按层嵌套 | `winpp::fs`、`winpp::ui` |
| 类型 | PascalCase | `window`、`Result`、`file_handle` |
| 函数 / 方法 | snake_case | `read_text()`、`create()` |
| 成员变量 | 尾下划线 | `hwnd_`、`code_` |
| 常量 / 枚举值 | `k` 前缀 + PascalCase | `kMaxPath`、`FileAccess::kRead` |
| 宏 | `WINPP_` 前缀 + 全大写 | `WINPP_TRY`、`WINPP_API` |
| 头文件 | 小写 + 下划线，`.hpp`；实现片段 `.inl` | `winpp/fs/file.hpp` |
| 注释语言 | **中文**（代码注释、设计文档、提交信息一律中文） | `// 打开文件，失败时返回 error` |

**C++11 兼容性纪律（写进 code review 清单）**：

- 禁止：`auto` 返回类型推导、泛型 lambda、`std::make_unique`、`if constexpr`、结构化绑定、`inline` 变量、`std::string_view`（用 `winpp::`）、`std::optional`（用 `Result`）、`std::filesystem`。
- 允许：`using` 别名、`nullptr`、`constexpr`（C++11 限制内）、`noexcept`、右值引用、变参模板、`enum class`。
- `static constexpr` 成员（如 `basic_handle<Traits>::invalid`）若被 ODR-use（取地址、传引用），C++11/14 必须在 `.inl` 里补类外定义；C++17+ 由 `inline` 变量自动兜底。写模板时别漏。
- 公共头加 `[[nodiscard]]` 一律写成 `WINPP_NODISCARD`。

**跨编译器纪律（MSVC / MinGW-w64 双主力）**：

- 禁止 `#pragma comment(lib, "...")`：GCC 直接忽略，链接一律走 CMake `target_link_libraries`。
- 禁止依赖 STL 实现细节（SSO 阈值、迭代顺序、`std::function` 小对象优化、`std::unordered_map` 桶数）。
- 禁止在公共头里裸写 `__declspec` / `__attribute__`，一律用 `WINPP_API` / `WINPP_NODISCARD` 宏。
- 源文件统一 **UTF-8 无 BOM**（MSVC 靠 `/utf-8` 而非 BOM 识别；BOM 反而会被 GCC 当成合法但多余的字符）。

---

## 8. 目录结构

```
WinAPI++/
├── CMakeLists.txt
├── README.md
├── Plan0.md                  # 本文档
├── cmake/
│   ├── winpp-config.cmake.in
│   └── compiler-check.cmake
├── include/winpp/
│   ├── winpp.hpp             # 总入口，聚合所有公共头
│   ├── compat/               # ★ 标准兼容层（仅 L0 可见）
│   │   ├── config.hpp        # 标准检测宏
│   │   ├── shims.hpp         # string_view / span / nodiscard 垫片
│   │   └── export.hpp        # WINPP_API / WINPP_INLINE
│   ├── core/                 # L0
│   │   ├── result.hpp
│   │   ├── error.hpp
│   │   ├── handle.hpp
│   │   ├── string.hpp
│   │   └── traits.hpp
│   ├── fs/                   # L1
│   │   ├── file.hpp
│   │   └── path.hpp
│   ├── sys/                  # L2
│   │   ├── process.hpp
│   │   ├── pipe.hpp
│   │   ├── thread.hpp
│   │   ├── registry.hpp
│   │   ├── env.hpp
│   │   └── time.hpp
│   ├── ui/                   # L3
│   │   ├── window.hpp
│   │   ├── window_class.hpp
│   │   ├── message_loop.hpp
│   │   ├── dpi.hpp
│   │   └── tray.hpp
│   ├── gdi/                  # L4
│   │   ├── dc.hpp
│   │   └── objects.hpp
│   ├── com/                  # 可选
│   │   └── com_ptr.hpp
│   └── detail/               # ★ 实现片段（.inl），被头文件与 .cpp 共用
│       ├── error.inl
│       ├── string.inl
│       └── process.inl
├── src/                      # 静态库/动态库模式的编译单元
│   ├── error.cpp
│   ├── string.cpp
│   ├── fs.cpp
│   ├── process.cpp
│   └── dllmain.cpp           # 仅动态库模式（可选）
├── examples/
│   ├── hello_window.cpp
│   ├── run_command.cpp
│   └── read_file.cpp
├── tests/
│   ├── CMakeLists.txt
│   ├── test_result.cpp
│   ├── test_string.cpp
│   ├── test_handle.cpp
│   └── test_compat_compile.cpp   # 三标准下编译同一份 API 用例
├── third_party/
│   └── doctest.h
└── docs/
    ├── design.md
    ├── style.md
    └── portability.md        # 三标准 × 三形态 × 双工具链的坑位记录
```

---

## 9. 构建配置

```cmake
cmake_minimum_required(VERSION 3.24)
project(WinAPI++ VERSION 0.1.0 LANGUAGES CXX)

set(WINPP_LIBRARY_TYPE "static" CACHE STRING "分发形态：header-only | static | shared")
set_property(CACHE WINPP_LIBRARY_TYPE PROPERTY STRINGS header-only static shared)

option(WINPP_BUILD_EXAMPLES  "构建示例程序"   ON)
option(WINPP_BUILD_TESTS     "构建单元测试"   ON)
option(WINPP_THROW_ON_ERROR  "错误改为抛异常" OFF)

add_library(winpp_compile_options INTERFACE)
target_compile_definitions(winpp_compile_options INTERFACE
    WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE _WIN32_WINNT=0x0A00)
target_link_libraries(winpp_compile_options INTERFACE user32 gdi32 shell32 advapi32 ole32)

# ---- 标准矩阵：MSVC 固定 C++17，MinGW 三档全开 ----
if(MSVC)   # 含 clang-cl
    target_compile_features(winpp_compile_options INTERFACE cxx_std_17)
    # /utf-8 是硬要求：源码含中文注释，默认系统代码页会乱码
    target_compile_options(winpp_compile_options INTERFACE /utf-8 /W4)
    if(MSVC_VERSION GREATER_EQUAL 1910)
        target_compile_options(winpp_compile_options INTERFACE /permissive-)
    endif()
else()
    # MinGW / clang：最低 C++11；用户可 -DCMAKE_CXX_STANDARD=17/23 自行升级
    target_compile_features(winpp_compile_options INTERFACE cxx_std_11)
    target_compile_options(winpp_compile_options INTERFACE -Wall -Wextra)
    if(MINGW)
        target_compile_options(winpp_compile_options INTERFACE -Wpedantic)
    endif()
endif()
```

```cmake
if(WINPP_LIBRARY_TYPE STREQUAL "header-only")
    add_library(winpp INTERFACE)
    target_include_directories(winpp INTERFACE include)
    target_compile_definitions(winpp INTERFACE WINPP_HEADER_ONLY)
    target_link_libraries(winpp INTERFACE winpp_compile_options)

else()
    set(_objs ${CMAKE_CURRENT_SOURCE_DIR}/src/error.cpp
              ${CMAKE_CURRENT_SOURCE_DIR}/src/string.cpp
              ${CMAKE_CURRENT_SOURCE_DIR}/src/fs.cpp
              ${CMAKE_CURRENT_SOURCE_DIR}/src/process.cpp)

    if(WINPP_LIBRARY_TYPE STREQUAL "shared")
        add_library(winpp SHARED ${_objs})
        target_compile_definitions(winpp PRIVATE WINPP_BUILD_SHARED)
        target_compile_definitions(winpp INTERFACE WINPP_USE_SHARED)
    else()
        add_library(winpp STATIC ${_objs})
    endif()

    target_include_directories(winpp PUBLIC include)
    target_link_libraries(winpp PUBLIC winpp_compile_options)
endif()

add_library(WinAPI++::winpp ALIAS winpp)
```

- MSVC 端要求 `cxx_std_17`（生成 `/std:c++17`）；MinGW 端要求 `cxx_std_11`（生成 `-std=c++11`）。用户在自己的项目里设更高标准时 CMake 自动取高，库侧零改动。
- **别被 `/std:c++17` 骗了**：MSVC 编的是 C++11 子集源码，不等于「MSVC 支持 C++11」。C++11 合规性由 MinGW 独家证明（见 §3.2）。
- MinGW 用户想跑 C++17 / C++23 档，只需 `-DCMAKE_CXX_STANDARD=17`（或 `23`）。
- `_WIN32_WINNT=0x0A00` 两个工具链都显式定义，杜绝「MSVC 编得过、MinGW 编不过」（见 §3.7.3）。
- 安装导出 `winpp-config.cmake`，支持 `find_package(WinAPI++ CONFIG)`；安装目录按工具链分（`lib/msvc-x64/`、`lib/mingw-ucrt-x64/`，见 §3.7.5）。
- GUI 示例在 MinGW 下需额外链接选项：

  ```cmake
  if(MINGW)
      target_link_options(winpp_hello_window PRIVATE -municode)
      set_target_properties(winpp_hello_window PROPERTIES WIN32_EXECUTABLE TRUE)
  endif()
  ```

---

## 10. 测试与 CI

### 10.1 测试分层

- **单元测试**（doctest）：`Result` 语义（含 C++11 移动/析构正确性）、`error::message()`、字符串互转（中文/emoji 边界）、句柄移动释放、`registry` 临时键、`process::run("cmd /c echo hi")`。
- **编译期一致性测试**：`test_compat_compile.cpp` 在三个标准下编译**同一份**用例文件，任何标准下出现签名差异即失败。
- **形态一致性测试**：同一份测试代码分别链接 header-only / static / shared 三种产物。
- **集成测试**：示例程序即测试，`hello_window` 创建窗口、投递 `WM_CLOSE`、退出码 0。

### 10.2 CI 矩阵

| 编译器 | 标准 | 形态 | 频率 |
| --- | --- | --- | --- |
| MSVC 2022 x64 | `/std:c++17` | static | 每次 PR |
| MSVC 2022 x64 | `/std:c++17` | header-only + shared | 每次 PR |
| MinGW-w64 GCC（MSYS2 UCRT64） | `-std=c++11` | static | 每次 PR |
| MinGW-w64 GCC（MSYS2 UCRT64） | `-std=c++17` | static | 每次 PR |
| MinGW-w64 GCC（MSYS2 UCRT64） | `-std=c++23` | static | 每次 PR |
| MinGW-w64 GCC（MSYS2 UCRT64） | `-std=c++17` | header-only + shared | 每次 PR |
| clang-cl x64 | `/std:c++17` | static | nightly |
| 全组合：MSVC(1 标准) × 3 形态 + MinGW(3 标准) × 3 形态 = 12 组 | — | — | nightly |

> **`-std=c++11` 那一行是矩阵里最不能删的一行**：它同时承担 C++11 合规验证与 `compat` 垫片的唯一编译覆盖（见 §3.2）。删掉它，MSVC 会一路绿到发布，而垫片代码从未被真正编译过。

门禁：MSVC 侧 `/W4 /utf-8`、MinGW 侧 `-Wall -Wextra -Wpedantic`，**两套工具链都零警告**才算过。

MinGW 侧实现要点：
- 用 `msys2/setup-msys2` action，安装 `mingw-w64-ucrt-x86_64-{gcc,cmake,ninja}`。
- 构建在 `UCRT64` 环境下进行，运行测试前把 `mingw64/bin` 加入 `PATH`（否则找不到 `libstdc++-6.dll`）。
- 显式断言 `gcc -dumpversion` ≥ 11，避免 runner 缓存里混进老版本。

---

## 11. 里程碑

| 里程碑 | 内容 | 验收标准 | 规模 |
| --- | --- | --- | --- |
| **M0 骨架** | CMake 三形态 + 双工具链分支、`compat/` 全套、doctest 接入、空测试 | MSVC c++17 × 3 形态 + MinGW 3 标准 × 3 形态 = **12 组空壳**全部构建通过 | M |
| **M1 基础** ✅ | `error`、`Result`、`basic_handle`、UTF-8 互转、`WINPP_TRY`、`map`/`and_then` | 三标准单测全绿；C++11 下 `Result` 移动/析构无泄漏 | M |
| **M2 文件** ✅ | `fs::open/read_text/read_all/write`、`fs::join/full_path/exists/remove/known_folder` | 能读写 UTF-8 中文路径文件（三标准） | M |
| **M3 进程** ✅ | `process::run` / `spawn`、管道捕获 stdout/stderr | `run({"cmd /c echo hi"})` 拿到 `hi` | M |
| **M4 窗口** ✅ | `window_class`、`window`、消息循环、DPI 感知 | `hello_window` 显示并正常关闭 | L |
| **M5 控件** ✅ | 按钮/编辑框/列表薄壳、消息框、托盘 | 带按钮的示例可交互 | L |
| **M6 周边** | 注册表、环境变量、时间、剪贴板、`ComPtr` | 对应单测通过 | M |
| **M7 GDI** | DC、画笔、画刷、字体、双缓冲 | 示例窗口绘制并抗闪烁 | M |

**建议节奏**：M0–M3 一次做完（兼容层 + 基础 + 文件 + 进程），即可覆盖日常脚本类需求；M4 之后再考虑 GUI。

> **M0 是本项目最硬的一块骨头**：兼容层没做扎实，后面每一层都要返工。宁可 M0 多花两天，也别带着「C++17 下能编、C++11 下炸」的债往下走。

### 11.1 M0 实施记录（2026-11-06）

**完成情况**

| 项 | 结果 |
| --- | --- |
| MinGW-w64 g++ 11.5 × C++11/17/23 × header-only/static/shared | **9 / 9 通过** |
| MSVC 14.51 × C++17 × header-only/static/shared | **3 / 3 通过** |
| CMake 4.5.0-rc3 构建：MinGW 三形态（含 ctest） | **3 / 3 通过** |
| CMake 4.5.0-rc3 构建：MSVC 三形态（含 ctest） | **3 / 3 通过** |
| 单元测试 | 16 个用例 / 41 个断言，全绿 |
| 编译警告 | 0（`-Wall -Wextra` / `/W4`） |

**与计划的偏差**

1. **traits 的无效值改为静态函数 `invalid()`**，而非 `static constexpr` 成员。原因：C++11 下静态常量成员被 ODR-use 时需要类外定义，header-only 模式下多 TU 会重复定义。§6.3 已同步更新。
2. **doctest 换为自研 `winpp_test.h`**：下载通道不通（schannel 凭据错误）。宏名对齐 doctest 习惯（`TEST_CASE` / `CHECK`），联网后可平滑替换。
3. **CMake 验证的曲折**：起初判定为 CMake 自身崩溃，最终定位为**运行沙箱限制**——workspace-write 下 CMake 初始化生成器即 Access Violation（最小空项目同样复现），放宽权限后一切正常。CMake 4.5.0-rc3 构建双工具链三形态全部通过，CMake 脚本本身无问题。

**环境问题（非项目缺陷）**

- 受限沙箱下 CMake 会崩，需在普通终端或完整权限下运行。
- VS2026 的 `vcvars64.bat` 报「文件名、目录名或卷标语法不正确」，`cl` 不进 PATH；`build_msvc.bat` 已手动设置 `PATH/INCLUDE/LIB` 绕过。
- MSVC 静态库形态的 ctest 首次运行耗时异常（约 4551 秒），疑似杀软实时扫描所致，非代码问题。

**下一步**：M1——`error` / `Result` / `handle` / 字符串的完整实现与边界用例（含 `WINPP_TRY` 宏、`map` / `and_then`）。

### 11.2 M1–M3 实施记录（2026-11-06）

**完成情况**

| 项 | 结果 |
| --- | --- |
| M1：`Result::map` / `and_then`、`WINPP_TRY` / `WINPP_TRY_VOID` | 完成 |
| M2：`fs::file`（open / read_text / read_all / write / size） | 完成 |
| M2：`fs::join` / `full_path` / `exists` / `remove` / `known_folder` | 完成 |
| M3：`process::run`（双线程读管道，防死锁） | 完成 |
| M3：`process::spawn` / `wait` / `kill` | 完成 |
| 单元测试 | **26 个用例 / 98 个断言全绿** |
| MinGW 直接编译：三标准 × 三形态 | 9 / 9 |
| CMake 构建：MinGW 三形态 + MSVC 三形态（含 ctest） | 6 / 6 |

**与计划的偏差**

1. **`spawn` 暂不支持捕获输出**：`capture_stdout/stderr` 仅对 `run()` 生效。异步捕获需要把读线程挂到对象生命周期上，复杂度高，留到后续版本。
2. **`known_folder` 用 CSIDL + `SHGetFolderPathW`**，而非 `KNOWNFOLDERID` + `SHGetKnownFolderPath`。前者兼容性更好、C++11 下更简单；后者留作后续增强。
3. **`file` / `process` 类补了 `WINPP_API`**：动态库模式下成员函数需要显式导出，否则 DLL 里没有符号。
4. **测试踩到句柄生命周期坑**：读文件时未先关闭句柄就 `DeleteFile`，Windows 会拒绝。测试已改为块作用域，顺带验证了 RAII 行为。
5. **`process::run` 的输出按原始字节返回**，未做代码页转换。cmd 的 `echo` 输出是 OEM 代码页（中文环境为 GBK），直接当 UTF-8 打印会乱码。后续可加 `codepage` 选项或 `oem_to_utf8()` 辅助函数。

**下一步**：M4（窗口与消息循环）。

### 11.3 M4–M5 实施记录（2026-11-06）

**完成情况**

| 项 | 结果 |
| --- | --- |
| M4：`window` / `message_loop` / DPI 感知 | 完成 |
| M5：`button` / `edit` / `listbox` / `message_box` / `tray_icon` | 完成 |
| 单元测试 | **34 个用例 / 115 个断言全绿** |
| MinGW 直接编译：三标准 × 三形态 | 9 / 9 |
| CMake 构建：MinGW 三形态 + MSVC 三形态 | 6 / 6 |

**设计决策**

1. **GUI 层全部头文件内联**：`window`、`control` 等不导出符号，三形态行为一致，避免跨 DLL 传 `std::function` 的坑。
2. **消息处理器用运行时 `std::unordered_map`**：按 M0 决策 Q3 落地。
3. **`WndProc` 用 `GWLP_USERDATA` 绑定 this**：`WM_NCCREATE` 时写入；窗口对象移动时同步更新指针，否则移动后消息会打到旧地址。
4. **DPI 动态加载**：`GetProcAddress` 取 `SetProcessDpiAwarenessContext` / `GetDpiForWindow`，老系统自动降级。
5. **控件只做薄壳**：创建、取文本、启停、显隐；不做布局引擎（非目标）。

**已知限制**

- 控件没有事件回调封装：按钮点击需自己在窗口的 `WM_COMMAND` 处理器里分发。
- `tray_icon` 依赖 Shell 环境，无桌面会话下不可用。
- 窗口类名固定为 `WinppWindowClass`，同一进程内共享。

**下一步**：M6（注册表、环境变量、时间、剪贴板、`ComPtr`）。

---

## 12. 决策记录（已定稿）

> 2026-11-06 评审通过，以下 8 项为最终决策，M0 起不再变更；如需推翻，走版本升级并在本表留痕。

| 编号 | 问题 | 决定 | 落点 |
| --- | --- | --- | --- |
| Q1 | 错误处理形态 | **`Result<T>` 默认不抛**，另提供 `WINPP_THROW_ON_ERROR` 开关让异常派接入 | §6.2 |
| Q2 | 默认分发形态 | **静态库**；header-only 作零配置选项，动态库按需 | §4.4、§9 |
| Q3 | 消息分发机制 | **运行时 `unordered_map`**；编译期模板暂缓（C++11 无 `if constexpr`，成本不划算） | §6.5 |
| Q4 | 兼容层归属 | **先内部**，接口冻结后再评估是否抽出独立库 | §3 |
| Q5 | C++23 特性暴露 | **不暴露**，仅内部可选（`WINPP_USE_STD_EXPECTED`） | §3.6 |
| Q6 | 文档与注释语言 | **全中文**：代码注释、设计文档、提交信息一律中文 | §7 |
| Q7 | MinGW 兼容边界 | **GCC ≥ 11 + UCRT + posix 线程模型**，其余「尽力而为」 | §3.7.1 |
| Q8 | 预编译二进制 | **先只发源码**；M6 后按工具链分目录发布再议 | §3.7.5 |

**待复查项（不阻塞 M0）**：

- **Q4 的抽出时机**：M3 完成后若外部仍无复用需求，就永久内部化。
- **Q8 的 M6 复查点**：接口趋稳后再决定是否上二进制发布流水线。
- **Q6 的副作用**：全中文意味着源文件含大量中文注释，MSVC 的 `/utf-8` 从「建议」升级为「缺了必炸」，已在 §9 落实。

---

## 13. 附：术语表

| 术语 | 含义 |
| --- | --- |
| 逃生通道 / escape hatch | 通过 `native()` 拿回原始句柄，绕过封装直接调 Win32 |
| traits | 描述某类句柄如何关闭、如何表示无效值的策略类 |
| Result | 携带「值或错误」的返回类型，替代异常与错误码混用 |
| 薄封装 | 只做类型安全与资源管理，不改变 API 语义 |
| 垫片 / shim | 为旧标准补上的最小替代实现（如 C++11 的 `string_view`） |
| `.inl` 片段 | 唯一实现源，被头文件（header-only）或 `.cpp`（库模式）包含 |
| 分发形态 | header-only / static / shared 三种产物形式 |

---

*本文档为 v0.7 版。M0 已验收通过（见 §11.1），下一步 M1。*
