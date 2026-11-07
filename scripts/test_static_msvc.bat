@echo off
setlocal
set VSROOT=C:\Program Files\Microsoft Visual Studio\18\Community
set MSVCVER=14.51.36231
set MSVC=%VSROOT%\VC\Tools\MSVC\%MSVCVER%
set SDK=C:\Program Files (x86)\Windows Kits\10
set SDKVER=10.0.26100.0

set PATH=%MSVC%\bin\Hostx64\x64;%PATH%
set INCLUDE=%MSVC%\include;%SDK%\Include\%SDKVER%\ucrt;%SDK%\Include\%SDKVER%\shared;%SDK%\Include\%SDKVER%\um
set LIB=%MSVC%\lib\x64;%SDK%\Lib\%SDKVER%\ucrt\x64;%SDK%\Lib\%SDKVER%\um\x64

cd /d "%~dp0.."

if not exist build\msvc_link_test.cpp (
  echo [FAIL] missing build\msvc_link_test.cpp
  exit /b 1
)

cl /nologo /std:c++17 /utf-8 /EHsc /I include ^
   /DWIN32_LEAN_AND_MEAN /DNOMINMAX /DUNICODE /D_UNICODE /D_WIN32_WINNT=0x0A00 ^
   build\msvc_link_test.cpp dist\lib\msvc-x64\winpp.lib ^
   user32.lib gdi32.lib shell32.lib advapi32.lib ole32.lib ^
   /Fe:build\msvc_link_test.exe /Fo:build\msvc_link_test.obj
if errorlevel 1 (
  echo [FAIL] link
  exit /b 1
)

build\msvc_link_test.exe
