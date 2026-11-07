#!/usr/bin/env bash
# 构建 MinGW 静态库产物
set -eu

GXX="${GXX:-/c/Program Files/RedPanda-Cpp/mingw64/bin/g++.exe}"
AR="${AR:-/c/Program Files/RedPanda-Cpp/mingw64/bin/ar.exe}"

cd "$(dirname "$0")/.."

OUT_LIB="dist/lib/mingw-ucrt-x64"
OUT_OBJ="dist/obj/mingw"
mkdir -p "$OUT_LIB" "$OUT_OBJ"

DEFS="-DWIN32_LEAN_AND_MEAN -DNOMINMAX -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0A00"
CXXFLAGS="-std=c++11 -O2 -I include $DEFS"

OBJS=()
for src in src/*.cpp; do
    name="$(basename "${src%.cpp}")"
    obj="$OUT_OBJ/$name.o"
    echo "编译 $src"
    "$GXX" $CXXFLAGS -c "$src" -o "$obj"
    OBJS+=("$obj")
done

echo "打包 $OUT_LIB/libwinpp.a"
rm -f "$OUT_LIB/libwinpp.a"
"$AR" rcs "$OUT_LIB/libwinpp.a" "${OBJS[@]}"

echo "完成：$OUT_LIB/libwinpp.a"
ls -lh "$OUT_LIB/libwinpp.a"
