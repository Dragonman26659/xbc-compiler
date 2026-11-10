@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\build-and-run.ps1" -BuildQemu %*
set "result=%ERRORLEVEL%"
if "%~1"=="" pause
exit /b %result%
