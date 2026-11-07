# WinAPI++ 构建脚本
#
# 三形态：header-only / static / shared
# 双工具链：MSVC（仅 C++17）/ MinGW-w64（C++11/17/23）
cmake_minimum_required(VERSION 3.24)
project(WinAPI++ VERSION 0.1.0 LANGUAGES CXX)

# ---------------------------------------------------------------
# 选项
# ---------------------------------------------------------------
set(WINPP_LIBRARY_TYPE "static" CACHE STRING "分发形态：header-only | static | shared")
set_property(CACHE WINPP_LIBRARY_TYPE PROPERTY STRINGS header-only static shared)

option(WINPP_BUILD_EXAMPLES "构建示例程序" ON)
option(WINPP_BUILD_TESTS    "构建单元测试" ON)
option(WINPP_THROW_ON_ERROR "错误改为抛异常" OFF)

# ---------------------------------------------------------------
# 公共编译选项
# ---------------------------------------------------------------
add_library(winpp_compile_options INTERFACE)
target_compile_definitions(winpp_compile_options INTERFACE
    WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE _WIN32_WINNT=0x0A00)
target_link_libraries(winpp_compile_options INTERFACE user32 gdi32 shell32 advapi32 ole32)

if(MSVC)
    # MSVC 只支持 C++17 档（见 Plan0 §3.2）
    target_compile_features(winpp_compile_options INTERFACE cxx_std_17)
    # /utf-8 是硬要求：源码含中文注释，默认系统代码页会乱码
    target_compile_options(winpp_compile_options INTERFACE /utf-8 /W4)
    if(MSVC_VERSION GREATER_EQUAL 1910)
        target_compile_options(winpp_compile_options INTERFACE /permissive-)
    endif()
else()
    # MinGW / clang：最低 C++11，用户可 -DCMAKE_CXX_STANDARD=17/23 升级
    target_compile_features(winpp_compile_options INTERFACE cxx_std_11)
    target_compile_options(winpp_compile_options INTERFACE -Wall -Wextra)
    if(MINGW)
        target_compile_options(winpp_compile_options INTERFACE -Wpedantic)
    endif()
endif()

if(WINPP_THROW_ON_ERROR)
    target_compile_definitions(winpp_compile_options INTERFACE WINPP_THROW_ON_ERROR)
endif()

# ---------------------------------------------------------------
# 库目标
# ---------------------------------------------------------------
if(WINPP_LIBRARY_TYPE STREQUAL "header-only")
    add_library(winpp INTERFACE)
    target_include_directories(winpp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_definitions(winpp INTERFACE WINPP_HEADER_ONLY)
    target_link_libraries(winpp INTERFACE winpp_compile_options)
else()
    set(WINPP_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/error.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/string.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/fs.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/process.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/registry.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/env.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/time.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/clipboard.cpp)

    if(WINPP_LIBRARY_TYPE STREQUAL "shared")
        add_library(winpp SHARED ${WINPP_SOURCES})
        target_compile_definitions(winpp PRIVATE WINPP_BUILD_SHARED)
        target_compile_definitions(winpp INTERFACE WINPP_USE_SHARED)
    else()
        add_library(winpp STATIC ${WINPP_SOURCES})
    endif()

    target_include_directories(winpp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(winpp PUBLIC winpp_compile_options)
endif()

add_library(WinAPI++::winpp ALIAS winpp)

# ---------------------------------------------------------------
# 测试
# ---------------------------------------------------------------
if(WINPP_BUILD_TESTS)
    enable_testing()
    add_executable(winpp_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_result.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_result_advanced.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_string.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_handle.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_fs.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_process.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_ui.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_sys.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_com.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_gdi.cpp)
    target_include_directories(winpp_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
    target_link_libraries(winpp_tests PRIVATE winpp)
    add_test(NAME winpp_tests COMMAND winpp_tests)
endif()

# ---------------------------------------------------------------
# 示例
# ---------------------------------------------------------------
if(WINPP_BUILD_EXAMPLES)
    add_executable(winpp_smoke ${CMAKE_CURRENT_SOURCE_DIR}/examples/smoke.cpp)
    target_link_libraries(winpp_smoke PRIVATE winpp)

    add_executable(winpp_run_command ${CMAKE_CURRENT_SOURCE_DIR}/examples/run_command.cpp)
    target_link_libraries(winpp_run_command PRIVATE winpp)

    add_executable(winpp_read_file ${CMAKE_CURRENT_SOURCE_DIR}/examples/read_file.cpp)
    target_link_libraries(winpp_read_file PRIVATE winpp)

    add_executable(winpp_hello_window ${CMAKE_CURRENT_SOURCE_DIR}/examples/hello_window.cpp)
    target_link_libraries(winpp_hello_window PRIVATE winpp)

    add_executable(winpp_paint_window ${CMAKE_CURRENT_SOURCE_DIR}/examples/paint_window.cpp)
    target_link_libraries(winpp_paint_window PRIVATE winpp)
endif()
