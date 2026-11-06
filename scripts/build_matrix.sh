#!/usr/bin/env bash
# MinGW-w64 三标准 × 三形态构建验证
# 用途：本机 VS2026 自带的 CMake 4.3.1 在 configure 阶段必崩（已确认与项目无关），
#       故用直接调用编译器的方式验证同一批组合。
set -u

GXX="${GXX:-/c/Program Files/RedPanda-Cpp/mingw64/bin/g++.exe}"
AR="${AR:-/c/Program Files/RedPanda-Cpp/mingw64/bin/ar.exe}"

cd "$(dirname "$0")/.." || exit 1
T=build/matrix
mkdir -p "$T"

DEFS="-DWIN32_LEAN_AND_MEAN -DNOMINMAX -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0A00"
LIBS="-luser32 -lgdi32 -lshell32 -ladvapi32 -lole32"
TESTS="tests/main.cpp tests/test_result.cpp tests/test_result_advanced.cpp tests/test_string.cpp tests/test_handle.cpp tests/test_fs.cpp tests/test_process.cpp tests/test_ui.cpp"

PASS=0
FAIL=0
FAILED=""

report() {
    local name="$1" rc="$2"
    if [ "$rc" -eq 0 ]; then
        PASS=$((PASS + 1))
        echo "[通过] $name"
    else
        FAIL=$((FAIL + 1))
        FAILED="$FAILED
  - $name"
        echo "[失败] $name"
    fi
}

for STD in c++11 c++17 c++23; do
    BASE="-std=$STD -I include -Wall -Wextra $DEFS"

    # ---------- header-only ----------
    "$GXX" $BASE -DWINPP_HEADER_ONLY -I third_party $TESTS -o "$T/h-$STD.exe" $LIBS \
        > "$T/h-$STD.log" 2>&1 && "./$T/h-$STD.exe" > /dev/null 2>&1
    report "MinGW $STD  header-only" $?

    # ---------- static ----------
    "$GXX" $BASE -c src/error.cpp -o "$T/error-$STD.o" > "$T/st-$STD.log" 2>&1 \
        && "$GXX" $BASE -c src/string.cpp -o "$T/string-$STD.o" >> "$T/st-$STD.log" 2>&1 \
        && "$GXX" $BASE -c src/fs.cpp -o "$T/fs-$STD.o" >> "$T/st-$STD.log" 2>&1 \
        && "$GXX" $BASE -c src/process.cpp -o "$T/process-$STD.o" >> "$T/st-$STD.log" 2>&1 \
        && "$AR" rcs "$T/libwinpp-$STD.a" "$T/error-$STD.o" "$T/string-$STD.o" "$T/fs-$STD.o" "$T/process-$STD.o" >> "$T/st-$STD.log" 2>&1 \
        && "$GXX" $BASE -I third_party $TESTS "$T/libwinpp-$STD.a" -o "$T/st-$STD.exe" $LIBS >> "$T/st-$STD.log" 2>&1 \
        && "./$T/st-$STD.exe" > /dev/null 2>&1
    report "MinGW $STD  static" $?

    # ---------- shared ----------
    "$GXX" $BASE -DWINPP_BUILD_SHARED -shared -o "$T/winpp-$STD.dll" \
        "$T/error-$STD.o" "$T/string-$STD.o" "$T/fs-$STD.o" "$T/process-$STD.o" $LIBS \
        -Wl,--out-implib,"$T/libwinpp-$STD.dll.a" > "$T/sh-$STD.log" 2>&1 \
        && "$GXX" $BASE -DWINPP_USE_SHARED -I third_party $TESTS "$T/libwinpp-$STD.dll.a" \
            -o "$T/sh-$STD.exe" $LIBS >> "$T/sh-$STD.log" 2>&1 \
        && "./$T/sh-$STD.exe" > /dev/null 2>&1
    report "MinGW $STD  shared" $?
done

echo
echo "通过 $PASS 组，失败 $FAIL 组"
if [ "$FAIL" -ne 0 ]; then
    echo "失败项：$FAILED"
    exit 1
fi
