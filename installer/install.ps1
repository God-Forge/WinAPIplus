<#
.SYNOPSIS
    WinAPI++ 静态库安装程序

.DESCRIPTION
    1. 复制头文件与静态库到安装目录
    2. 设置用户环境变量 WINPP_HOME
    3. 检测 Visual Studio / Dev-C++ / RedPanda-C++
    4. 生成可直接打开的示例项目

.PARAMETER InstallDir
    安装目录，默认 %LOCALAPPDATA%\WinAPI++

.PARAMETER NoEnv
    不设置 WINPP_HOME 环境变量

.PARAMETER Force
    目录已存在时直接覆盖，不询问
#>
param(
    [string]$InstallDir = "$env:LOCALAPPDATA\WinAPI++",
    [switch]$NoEnv,
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$PackageRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

# 统一转绝对路径：生成的示例项目里写的是绝对路径，避免相对路径解析出错
$InstallDir = [System.IO.Path]::GetFullPath($InstallDir)

function Write-Step([string]$msg) { Write-Host "[*] $msg" -ForegroundColor Cyan }
function Write-Ok([string]$msg)   { Write-Host "[+] $msg" -ForegroundColor Green }
function Write-Warn([string]$msg) { Write-Host "[!] $msg" -ForegroundColor Yellow }

Write-Host ""
Write-Host "  WinAPI++ static library installer" -ForegroundColor White
Write-Host ""

# ---------- 1. 准备安装目录 ----------
if (Test-Path $InstallDir) {
    if (-not $Force) {
        $answer = Read-Host "Directory '$InstallDir' already exists. Overwrite? (y/N)"
        if ($answer -notmatch '^[yY]') {
            Write-Warn "Aborted."
            exit 1
        }
    }
    Remove-Item -Recurse -Force $InstallDir
}
New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null
Write-Ok "Install directory: $InstallDir"

# ---------- 2. 复制文件 ----------
Write-Step "Copying files..."
foreach ($item in @('include', 'lib', 'cmake')) {
    $src = Join-Path $PackageRoot $item
    if (Test-Path $src) {
        Copy-Item $src -Destination $InstallDir -Recurse -Force
    }
}
Write-Ok "Files copied."

# ---------- 3. 环境变量 ----------
if (-not $NoEnv) {
    [Environment]::SetEnvironmentVariable('WINPP_HOME', $InstallDir, 'User')
    $env:WINPP_HOME = $InstallDir
    Write-Ok "WINPP_HOME = $InstallDir (user environment)"
}

# ---------- 4. 检测 IDE ----------
Write-Step "Detecting IDEs..."

$vs = $null
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vswhere) {
    $vs = & $vswhere -latest -property installationPath 2>$null
}
if ($vs) { Write-Ok "Visual Studio : $vs" } else { Write-Warn "Visual Studio : not found" }

$devcpp = $null
foreach ($p in @("C:\Program Files (x86)\Dev-Cpp", "C:\Program Files\Dev-Cpp",
                 "D:\Program Files (x86)\Dev-Cpp", "D:\Program Files\Dev-Cpp")) {
    if (Test-Path (Join-Path $p 'devcpp.exe')) { $devcpp = $p; break }
}
if (-not $devcpp) {
    try {
        $found = Get-ChildItem 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall' -ErrorAction SilentlyContinue |
                 ForEach-Object { Get-ItemProperty $_.PSPath -ErrorAction SilentlyContinue } |
                 Where-Object { $_.DisplayName -like '*Dev-C++*' } |
                 Select-Object -First 1
        if ($found -and $found.InstallLocation) { $devcpp = $found.InstallLocation }
    } catch { }
}
if ($devcpp) { Write-Ok "Dev-C++       : $devcpp" } else { Write-Warn "Dev-C++       : not found" }

$redpanda = $null
foreach ($p in @("C:\Program Files\RedPanda-Cpp", "D:\Program Files\RedPanda-Cpp",
                 "C:\Program Files (x86)\RedPanda-Cpp")) {
    if (Test-Path (Join-Path $p 'RedPandaIDE.exe')) { $redpanda = $p; break }
}
if ($redpanda) { Write-Ok "RedPanda-C++  : $redpanda" } else { Write-Warn "RedPanda-C++  : not found" }

# ---------- 5. 生成示例项目 ----------
Write-Step "Generating example projects..."
$exDir = Join-Path $InstallDir 'examples'
New-Item -ItemType Directory -Force -Path $exDir | Out-Null

# 5.1 示例源码
$mainSrc = @"
#include <winpp/winpp.hpp>

#include <cstdio>

int main() {
    auto r = winpp::ok(42);
    std::printf("Result: %d\n", r.value());

    winpp::sys::process_options opts;
    opts.command_line = "cmd /c echo hello from WinAPI++";
    opts.capture_stdout = true;
    auto p = winpp::sys::run(opts);
    if (p) std::printf("%s", p.value().out.c_str());

    std::printf("time: %s\n", winpp::sys::format_time(winpp::sys::local_time()).c_str());
    return 0;
}
"@
Set-Content -Path (Join-Path $exDir 'main.cpp') -Value $mainSrc -Encoding UTF8

# 5.2 Dev-C++ / RedPanda 共用的 .dev 项目
$incDir = Join-Path $InstallDir 'include'
$devContent = @"
[Project]
FileName=WinAPPDemo.dev
Name=WinAPPDemo
Type=1
Ver=2
ObjFiles=
Includes=$incDir
Libs=-L$InstallDir\lib\mingw-ucrt-x64 -lwinpp -luser32 -lgdi32 -lshell32 -ladvapi32 -lole32
PrivateResource=
ResourceIncludes=
MakeIncludes=
Compiler=-DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_WIN32_WINNT=0x0A00
CppCompiler=
Linker=
IsCpp=1
Icon=
ExeOutput=
ObjectOutput=
LogOutput=
LogOutputEnabled=0
OverrideOutput=0
OverrideOutputName=
HostApplication=
UseCustomMakefile=0
CustomMakefile=
CommandLine=
Folders=
IncludeVersionInfo=0
SupportXPThemes=0
CompilerSet=0
CompilerSettings=0000000000000000000000
UnitCount=1

[Unit1]
FileName=main.cpp
CompileCpp=1
Folder=
Compile=1
Link=1
Priority=1000
OverrideBuildCmd=0
BuildCmd=
"@
$devFile = Join-Path $exDir 'WinAPPDemo.dev'
Set-Content -Path $devFile -Value $devContent -Encoding ASCII
Write-Ok "Dev-C++ / RedPanda project: $devFile"

# 5.3 Visual Studio 项目（检测到时才生成）
if ($vs) {
    $guid = [guid]::NewGuid().ToString().ToUpper()
    $libDir = Join-Path $InstallDir 'lib\msvc-x64'
    $vcx = @"
<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" ToolsVersion="17.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup Label="ProjectConfigurations">
    <ProjectConfiguration Include="Release|x64">
      <Configuration>Release</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
  </ItemGroup>
  <PropertyGroup Label="Globals">
    <ProjectGuid>{$guid}</ProjectGuid>
    <RootNamespace>WinAPPDemo</RootNamespace>
    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>
  </PropertyGroup>
  <Import Project="`$(VCTargetsPath)\Microsoft.Cpp.Default.props" />
  <PropertyGroup Label="Configuration">
    <ConfigurationType>Application</ConfigurationType>
    <PlatformToolset>`$(DefaultPlatformToolset)</PlatformToolset>
  </PropertyGroup>
  <Import Project="`$(VCTargetsPath)\Microsoft.Cpp.props" />
  <ItemDefinitionGroup>
    <ClCompile>
      <AdditionalIncludeDirectories>$incDir;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
      <PreprocessorDefinitions>WIN32_LEAN_AND_MEAN;NOMINMAX;UNICODE;_UNICODE;_WIN32_WINNT=0x0A00;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <LanguageStandard>stdcpp17</LanguageStandard>
      <AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>
    </ClCompile>
    <Link>
      <AdditionalDependencies>winpp.lib;user32.lib;gdi32.lib;shell32.lib;advapi32.lib;ole32.lib;%(AdditionalDependencies)</AdditionalDependencies>
      <AdditionalLibraryDirectories>$libDir;%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>
    </Link>
  </ItemDefinitionGroup>
  <ItemGroup>
    <ClCompile Include="main.cpp" />
  </ItemGroup>
  <Import Project="`$(VCTargetsPath)\Microsoft.Cpp.targets" />
</Project>
"@
    $vcxFile = Join-Path $exDir 'WinAPPDemo.vcxproj'
    Set-Content -Path $vcxFile -Value $vcx -Encoding UTF8
    Write-Ok "Visual Studio project: $vcxFile"
}

# ---------- 6. 汇总 ----------
Write-Host ""
Write-Host "Installation complete." -ForegroundColor Green
Write-Host "  WINPP_HOME : $InstallDir"
Write-Host "  include    : $InstallDir\include"
Write-Host "  libraries  : $InstallDir\lib"
Write-Host ""
Write-Host "Open an example project:" -ForegroundColor Cyan
if ($redpanda) { Write-Host "  RedPanda-C++  -> $exDir\WinAPPDemo.dev" }
if ($devcpp)   { Write-Host "  Dev-C++       -> $exDir\WinAPPDemo.dev" }
if ($vs)       { Write-Host "  Visual Studio -> $exDir\WinAPPDemo.vcxproj" }
Write-Host ""
Write-Host "For your own project:" -ForegroundColor Cyan
Write-Host "  include : $InstallDir\include"
Write-Host "  libs    : $InstallDir\lib\msvc-x64 (MSVC) / $InstallDir\lib\mingw-ucrt-x64 (MinGW)"
Write-Host "  defines : WIN32_LEAN_AND_MEAN;NOMINMAX;UNICODE;_UNICODE;_WIN32_WINNT=0x0A00"
Write-Host "  link    : winpp.lib / libwinpp.a + user32 gdi32 shell32 advapi32 ole32"
Write-Host ""
