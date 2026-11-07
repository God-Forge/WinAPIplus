"""给 install.ps1 的 vcxproj 模板补上 /utf-8 编译选项。"""
import sys

path = "installer/install.ps1"
with open(path, "r", encoding="utf-8-sig") as f:
    text = f.read()

old = "      <LanguageStandard>stdcpp17</LanguageStandard>\n"
new = (
    "      <LanguageStandard>stdcpp17</LanguageStandard>\n"
    "      <AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>\n"
)

if old not in text:
    print("NOT FOUND")
    sys.exit(1)

text = text.replace(old, new, 1)

with open(path, "w", encoding="utf-8-sig", newline="\n") as f:
    f.write(text)

print("patched")
