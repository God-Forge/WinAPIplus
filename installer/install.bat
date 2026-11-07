@echo off
chcp 65001 >nul
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0install.ps1" %*
if errorlevel 1 (
  echo.
  echo Install failed.
) else (
  echo.
  echo Done.
)
pause
