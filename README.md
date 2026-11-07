# WinAPI++

把 Win32 API 包成「现代 C++ 该有的样子」——RAII 管资源、Result 管错误、UTF-8 管字符串、原生句柄随时可拿。

设计文档见 [Plan0.md](Plan0.md)。

---

## 当前状态

**M0–M6 里程碑已完成**：兼容层、核心类型、文件层、进程层、界面层、系统工具全部就位。

| 验证项 | 结果 |
| --- | --- |
| MinGW-w64 g++ 11.5 × C++11/17/23 × header-only/static/shared | 9 / 9 通过 |
| CMake 4.5 构建：MinGW 三形态（含 ctest） | 3 / 3 通过 |
| CMake 4.5 构建：MSVC 三形态（含 ctest） | 3 / 3 通过 |
| 编译警告 | 0 |
| 单元测试 | **40 个用例 / 140 个断言全绿** |

已实现的能力：

| 层 | 内容 |
| --- | --- |
| 核心 | `Result<T>`（含 `map` / `and_then`）、`WINPP_TRY`、`error`、`basic_handle`、UTF-8 ↔ UTF-16 |
| 文件 | `fs::open` / `read_text` / `read_all` / `write` / `size`、`fs::join` / `full_path` / `exists` / `remove` / `known_folder` |
| 进程 | `sys::run`（捕获 stdout/stderr）、`sys::process::spawn` / `wait` / `kill` |
| 界面 | `ui::window` / `message_loop` / DPI 感知、`button` / `edit` / `listbox`、`message_box`、`tray_icon` |
| 系统 | `sys::key`（注册表）、`sys::get_env` / `set_env` / `expand_env`、`sys::local_time` / `now_unix_ms`、`ui::clipboard_text`、`com::com_ptr` |

---

## 支持矩阵

| 编译器 | 标准 | 说明 |
| --- | --- | --- |
| MSVC（VS2022+） | 仅 C++17 | 固定 `/std:c++17` |
| MinGW-w64 GCC | C++11 / C++17 / C++23 | 要求 GCC ≥ 11、UCRT、posix 线程模型 |

分发形态：`header-only` / `static` / `shared` 三选一，同一份源码。

---

## 安装（静态库分发）

### 方式一：安装包（推荐给 IDE 用户）

1. 解压 `WinAPI++-1.0.0-win64.zip`
2. 双击 `install.bat`（或右键 `install.ps1` → 用 PowerShell 运行）
3. 安装程序会：
   - 复制头文件与静态库到 `%LOCALAPPDATA%\WinAPI++`
   - 设置用户环境变量 `WINPP_HOME`
   - **检测 Visual Studio / Dev-C++ / RedPanda-C++**
   - 在 `%LOCALAPPDATA%\WinAPI++\examples` 生成可直接打开的示例项目

可选参数：

```powershell
.\install.ps1 -InstallDir "D:\WinAPI++"   # 自定义安装目录
.\install.ps1 -NoEnv                      # 不设置环境变量
.\install.ps1 -Force                      # 覆盖已有目录不询问
```

### 方式二：源码构建静态库

```bash
# MinGW
bash scripts/build_static_mingw.sh        # -> dist/lib/mingw-ucrt-x64/libwinpp.a

# MSVC
cmd /c scripts\build_static_msvc.bat      # -> dist/lib/msvc-x64/winpp.lib

# 打包成 zip
bash scripts/package.sh                   # -> dist/WinAPI++-1.0.0-win64.zip
```

### 在自己的项目里使用静态库

| 配置项 | 值 |
| --- | --- |
| 包含目录 | `<安装目录>\include` |
| 库目录 | `<安装目录>\lib\msvc-x64`（MSVC）或 `<安装目录>\lib\mingw-ucrt-x64`（MinGW） |
| 链接库 | `winpp.lib` / `libwinpp.a` + `user32 gdi32 shell32 advapi32 ole32` |
| 宏定义 | `WIN32_LEAN_AND_MEAN` `NOMINMAX` `UNICODE` `_UNICODE` `_WIN32_WINNT=0x0A00` |

> MSVC 用户注意：库是用 `/MD` 构建的，项目也要用 `/MD`（默认即是），否则会报 `LNK2038` CRT 不匹配。
> 源文件若含中文，记得加 `/utf-8`。

### CMake 用户

```cmake
find_package(WinAPI++ CONFIG REQUIRED)
target_link_libraries(app PRIVATE WinAPI++::winpp)
```

---

## 快速开始

```cpp
#include <winpp/winpp.hpp>

int main() {
    // 结果与错误
    auto r = winpp::ok(42);
    if (r) {
        std::printf("%d\n", r.value());
    }

    // 链式调用
    auto text = winpp::ok(21).map([](int v) { return std::to_string(v * 2); });

    // 文件
    auto f = winpp::fs::open("config.txt", winpp::fs::access::read);
    if (f) {
        auto content = f.value().read_text();
        if (content) std::printf("%s", content.value().c_str());
    }

    // 进程
    winpp::sys::process_options opts;
    opts.command_line = "cmd /c echo hello";
    opts.capture_stdout = true;
    auto out = winpp::sys::run(opts);
    if (out) std::printf("%s", out.value().out.c_str());
}
```

三种形态的用法完全一致，区别只在构建时怎么链接：

```cmake
# header-only
target_link_libraries(app PRIVATE WinAPI++::winpp)

# 静态库
# 构建时 -DWINPP_LIBRARY_TYPE=static

# 动态库
# 构建时 -DWINPP_LIBRARY_TYPE=shared（会自动带上 WINPP_USE_SHARED）
```

---

## 构建

### CMake（推荐，需 CMake ≥ 3.24）

```bash
cmake -S . -B build -G Ninja \
      -DCMAKE_CXX_COMPILER=g++ \
      -DWINPP_LIBRARY_TYPE=static
cmake --build build
ctest --test-dir build --output-on-failure
```

选项：

| 选项 | 默认 | 说明 |
| --- | --- | --- |
| `WINPP_LIBRARY_TYPE` | `static` | `header-only` / `static` / `shared` |
| `WINPP_BUILD_TESTS` | `ON` | 构建单元测试 |
| `WINPP_BUILD_EXAMPLES` | `ON` | 构建示例 |
| `WINPP_THROW_ON_ERROR` | `OFF` | 错误改为抛异常 |

### 本机验证脚本（CMake 不可用时的替代）

```bash
# MinGW：三标准 × 三形态，共 9 组
bash scripts/build_matrix.sh

# MSVC：C++17 × 三形态，共 3 组
cmd /c scripts\build_msvc.bat
```

两个脚本都会实际运行测试程序，而不只是编译。

---

## 已知限制

- `sys::run` 捕获的 stdout/stderr 按**原始字节**返回，不做代码页转换。cmd 内置命令（如 `echo`）输出的是 OEM 代码页，中文环境下直接按 UTF-8 打印会乱码；需要时请自行用 `MultiByteToWideChar(CP_OEMCP, ...)` 转换。
- `sys::process::spawn` 暂不支持捕获输出（`capture_*` 仅对 `run` 生效）。

---

## 已知环境问题（本机，非项目缺陷）

1. **CMake 在受限沙箱下会在 `configure` 阶段崩溃**（Access Violation，最小空项目也复现）。放宽到完整权限后一切正常——这是运行环境限制，不是 CMake 或本项目的问题，在普通终端里直接跑即可。
2. **VS2026 的 `vcvars64.bat` 报「文件名、目录名或卷标语法不正确」**，导致 `cl` 不在 PATH。`scripts/build_msvc.bat` 已改为手动设置 `PATH/INCLUDE/LIB` 绕过。
3. **doctest 未能下载**（网络凭据问题），`third_party/winpp_test.h` 是自研极简测试框架，宏名对齐 doctest，联网后可直接替换。

---

## 目录结构

```
include/winpp/
├── winpp.hpp            总入口
├── compat/              标准兼容层（config / shims / export）
├── core/                error / result / check / handle / string / traits
├── fs/                  file / path
├── sys/                 process
└── detail/              *.inl 唯一实现源
src/                     库模式编译单元
tests/                   单元测试（26 用例 / 98 断言）
scripts/                 本机构建验证脚本
third_party/             测试框架
```

---

## 设计要点

- **兼容层**：公共 API 只写 `winpp::string_view` / `winpp::span` / `winpp::Result<T>`，C++17+ 别名到标准库，C++11 用自研垫片。签名在三个标准下逐字相同。
- **一份实现、两种用法**：实现写在 `detail/*.inl`，header-only 时以 `inline` 贴进头文件，库模式时由 `src/*.cpp` 包含产生唯一符号。
- **逃生通道**：每个封装对象都能 `get()` 取回原始句柄。
- **不抛异常**：所有可失败操作返回 `Result<T>`，`error::message()` 延迟格式化。

---

## 许可

待定。
