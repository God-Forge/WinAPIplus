@echo off
setlocal
rem vcvars64.bat is broken in this VS install, so set the toolchain env manually.
set VSROOT=C:\Program Files\Microsoft Visual Studio\18\Community
set MSVCVER=14.51.36231
set MSVC=%VSROOT%\VC\Tools\MSVC\%MSVCVER%
set SDK=C:\Program Files (x86)\Windows Kits\10
set SDKVER=10.0.26100.0

set PATH=%MSVC%\bin\Hostx64\x64;%PATH%
set INCLUDE=%MSVC%\include;%SDK%\Include\%SDKVER%\ucrt;%SDK%\Include\%SDKVER%\shared;%SDK%\Include\%SDKVER%\um
set LIB=%MSVC%\lib\x64;%SDK%\Lib\%SDKVER%\ucrt\x64;%SDK%\Lib\%SDKVER%\um\x64

cd /d "%~dp0.."
set ROOT=%CD%
set T=%ROOT%\build\msvc
if not exist "%T%" mkdir "%T%"

set DEFS=/DWIN32_LEAN_AND_MEAN /DNOMINMAX /DUNICODE /D_UNICODE /D_WIN32_WINNT=0x0A00
set FLAGS=/std:c++17 /utf-8 /EHsc /W4 /nologo /I "%ROOT%\include" /I "%ROOT%\third_party"
set LIBS=user32.lib gdi32.lib shell32.lib advapi32.lib ole32.lib
set TESTS="%ROOT%\tests\main.cpp" "%ROOT%\tests\test_result.cpp" "%ROOT%\tests\test_result_advanced.cpp" "%ROOT%\tests\test_string.cpp" "%ROOT%\tests\test_handle.cpp" "%ROOT%\tests\test_fs.cpp" "%ROOT%\tests\test_process.cpp" "%ROOT%\tests\test_ui.cpp"
set PASS=0
set FAIL=0

echo === [1/3] MSVC c++17 header-only ===
cl %FLAGS% %DEFS% /DWINPP_HEADER_ONLY %TESTS% /Fe:"%T%\h.exe" /Fo:"%T%\\" %LIBS% > "%T%\h.log" 2>&1
if errorlevel 1 (
  echo [FAIL] header-only compile
  set /a FAIL+=1
) else (
  "%T%\h.exe" >nul 2>&1
  if errorlevel 1 (echo [FAIL] header-only run & set /a FAIL+=1) else (echo [PASS] header-only & set /a PASS+=1)
)

echo === [2/3] MSVC c++17 static ===
cl %FLAGS% %DEFS% /c "%ROOT%\src\error.cpp" "%ROOT%\src\string.cpp" /Fo:"%T%\\" > "%T%\st.log" 2>&1
if errorlevel 1 (
  echo [FAIL] static compile
  set /a FAIL+=1
) else (
  lib /nologo /OUT:"%T%\winpp.lib" "%T%\error.obj" "%T%\string.obj" >> "%T%\st.log" 2>&1
  cl %FLAGS% %DEFS% %TESTS% "%T%\winpp.lib" /Fe:"%T%\st.exe" /Fo:"%T%\\" %LIBS% >> "%T%\st.log" 2>&1
  if errorlevel 1 (
    echo [FAIL] static link
    set /a FAIL+=1
  ) else (
    "%T%\st.exe" >nul 2>&1
    if errorlevel 1 (echo [FAIL] static run & set /a FAIL+=1) else (echo [PASS] static & set /a PASS+=1)
  )
)

echo === [3/3] MSVC c++17 shared ===
cl %FLAGS% %DEFS% /DWINPP_BUILD_SHARED /c "%ROOT%\src\error.cpp" "%ROOT%\src\string.cpp" /Fo:"%T%\\" > "%T%\sh.log" 2>&1
if errorlevel 1 (
  echo [FAIL] shared compile
  set /a FAIL+=1
) else (
  link /nologo /DLL /OUT:"%T%\winpp.dll" /IMPLIB:"%T%\winpp.lib" "%T%\error.obj" "%T%\string.obj" %LIBS% >> "%T%\sh.log" 2>&1
  cl %FLAGS% %DEFS% /DWINPP_USE_SHARED %TESTS% "%T%\winpp.lib" /Fe:"%T%\sh.exe" /Fo:"%T%\\" %LIBS% >> "%T%\sh.log" 2>&1
  if errorlevel 1 (
    echo [FAIL] shared link
    set /a FAIL+=1
  ) else (
    "%T%\sh.exe" >nul 2>&1
    if errorlevel 1 (echo [FAIL] shared run & set /a FAIL+=1) else (echo [PASS] shared & set /a PASS+=1)
  )
)

echo.
echo MSVC pass=%PASS% fail=%FAIL%
if %FAIL% neq 0 exit /b 1
exit /b 0
