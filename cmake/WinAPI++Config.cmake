# WinAPI++ 包配置
#
# 用法：
#   find_package(WinAPI++ CONFIG REQUIRED)
#   target_link_libraries(app PRIVATE WinAPI++::winpp)

get_filename_component(WINPP_PREFIX "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

if(NOT TARGET WinAPI++::winpp)
    add_library(WinAPI++::winpp STATIC IMPORTED)

    if(MSVC)
        set(_winpp_lib "${WINPP_PREFIX}/lib/msvc-x64/winpp.lib")
    else()
        set(_winpp_lib "${WINPP_PREFIX}/lib/mingw-ucrt-x64/libwinpp.a")
    endif()

    set_target_properties(WinAPI++::winpp PROPERTIES
        IMPORTED_LOCATION "${_winpp_lib}"
        INTERFACE_INCLUDE_DIRECTORIES "${WINPP_PREFIX}/include"
        INTERFACE_COMPILE_DEFINITIONS
            "WIN32_LEAN_AND_MEAN;NOMINMAX;UNICODE;_UNICODE;_WIN32_WINNT=0x0A00"
        INTERFACE_LINK_LIBRARIES
            "user32;gdi32;shell32;advapi32;ole32"
    )

    if(MSVC)
        target_compile_features(WinAPI++::winpp INTERFACE cxx_std_17)
    else()
        target_compile_features(WinAPI++::winpp INTERFACE cxx_std_11)
    endif()
endif()

set(WinAPI++_FOUND TRUE)
