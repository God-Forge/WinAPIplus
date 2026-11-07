@echo off
setlocal
rem vcvars64.bat is broken on this machine, set toolchain env manually.
set VSROOT=C:\Program Files\Microsoft Visual Studio\18\Community
set MSVCVER=14.51.36231
set MSVC=%VSROOT%\VC\Tools\MSVC\%MSVCVER%
set SDK=C:\Program Files (x86)\Windows Kits\10
set SDKVER=10.0.26100.0

set PATH=%MSVC%\bin\Hostx64\x64;%PATH%
set INCLUDE=%MSVC%\include;%SDK%\Include\%SDKVER%\ucrt;%SDK%\Include\%SDKVER%\shared;%SDK%\Include\%SDKVER%\um
set LIB=%MSVC%\lib\x64;%SDK%\Lib\%SDKVER%\ucrt\x64;%SDK%\Lib\%SDKVER%\um\x64

cd /d "%~dp0.."
if not exist "dist\lib\msvc-x64" mkdir "dist\lib\msvc-x64"
if not exist "dist\obj\msvc" mkdir "dist\obj\msvc"

cl /c /O2 /std:c++17 /utf-8 /EHsc /MD /nologo /I include ^
   /DWIN32_LEAN_AND_MEAN /DNOMINMAX /DUNICODE /D_UNICODE /D_WIN32_WINNT=0x0A00 ^
   src\*.cpp /Fo:dist\obj\msvc\
if errorlevel 1 (
  echo [FAIL] compile
  exit /b 1
)

lib /nologo /OUT:"dist\lib\msvc-x64\winpp.lib" dist\obj\msvc\*.obj
if errorlevel 1 (
  echo [FAIL] lib
  exit /b 1
)

echo [OK] dist\lib\msvc-x64\winpp.lib
dir "dist\lib\msvc-x64\winpp.lib"
