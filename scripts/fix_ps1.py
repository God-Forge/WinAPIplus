"""修正 install.ps1：
1. 保证文件只有一个 UTF-8 BOM（PowerShell 5.1 才会按 UTF-8 解析）
2. 把 here-string 里的双反斜杠还原成单反斜杠（PowerShell 字符串不需要转义反斜杠）
"""
import sys

path = sys.argv[1]

with open(path, "rb") as f:
    data = f.read()

# 去掉开头所有 BOM，保证只留一个
while data.startswith(b"\xef\xbb\xbf"):
    data = data[3:]

text = data.decode("utf-8")
text = text.replace("\\\\", "\\")

with open(path, "wb") as f:
    f.write(b"\xef\xbb\xbf" + text.encode("utf-8"))

print("fixed:", path)
