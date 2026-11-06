"""下载 CMake 安装包到工作区 build/ 目录。

网络说明：本机 curl(schannel) / certutil(WinHTTP) 均不可用，
Python 的 OpenSSL 栈可以直连 cmake.org。
"""
import hashlib
import os
import sys
import urllib.request

URL = "https://cmake.org/files/v4.5/cmake-4.5.0-rc3-windows-x86_64.msi"
DEST = os.path.join("build", "cmake-4.5.0-rc3-windows-x86_64.msi")
EXPECTED_SIZE = 40402944


def main() -> int:
    os.makedirs("build", exist_ok=True)
    req = urllib.request.Request(
        URL, headers={"User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64)"}
    )

    sha = hashlib.sha256()
    total = 0
    with urllib.request.urlopen(req, timeout=120) as resp, open(DEST, "wb") as f:
        length = resp.headers.get("Content-Length")
        print(f"HTTP {resp.status}，Content-Length={length}")
        while True:
            chunk = resp.read(1024 * 256)
            if not chunk:
                break
            f.write(chunk)
            sha.update(chunk)
            total += len(chunk)
            if total % (1024 * 1024 * 8) < 1024 * 256:
                print(f"  已下载 {total / 1024 / 1024:.1f} MB")

    size = os.path.getsize(DEST)
    print(f"完成：{DEST}")
    print(f"大小：{size} 字节")
    print(f"SHA256：{sha.hexdigest()}")
    if size != EXPECTED_SIZE:
        print(f"警告：大小与预期 {EXPECTED_SIZE} 不符", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
