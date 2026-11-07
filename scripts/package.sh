#!/usr/bin/env bash
# 打包静态库分发目录与 zip
set -eu

VERSION="1.0.0"
NAME="WinAPI++-${VERSION}-win64"

cd "$(dirname "$0")/.."

OUT="dist/${NAME}"
rm -rf "$OUT"
mkdir -p "$OUT"

echo "复制头文件"
cp -r include "$OUT/"

echo "复制静态库"
mkdir -p "$OUT/lib"
cp -r dist/lib/mingw-ucrt-x64 "$OUT/lib/"
cp -r dist/lib/msvc-x64 "$OUT/lib/"

echo "复制 CMake 配置"
mkdir -p "$OUT/cmake"
cp cmake/WinAPI++Config.cmake "$OUT/cmake/"

echo "复制安装程序"
cp installer/install.ps1 "$OUT/"
cp installer/install.bat "$OUT/"

echo "复制示例源码"
mkdir -p "$OUT/examples"
cp examples/*.cpp "$OUT/examples/"

echo "复制文档"
cp README.md "$OUT/README.md" 2>/dev/null || true

echo "打包 zip"
cd dist
rm -f "${NAME}.zip"
if command -v 7z >/dev/null 2>&1; then
    7z a -tzip "${NAME}.zip" "$NAME" >/dev/null
elif command -v zip >/dev/null 2>&1; then
    zip -rq "${NAME}.zip" "$NAME"
elif command -v powershell >/dev/null 2>&1; then
    powershell -NoProfile -Command "Compress-Archive -Path '$(cygpath -w "$NAME" 2>/dev/null || echo "$NAME")' -DestinationPath '$(cygpath -w "${NAME}.zip" 2>/dev/null || echo "${NAME}.zip")' -Force" >/dev/null
else
    echo "警告：未找到 7z / zip / powershell，仅生成目录 $OUT"
    exit 0
fi

echo "完成：dist/${NAME}.zip"
ls -lh "${NAME}.zip"
