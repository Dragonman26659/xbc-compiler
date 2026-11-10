@echo off
rem SPDX-License-Identifier: GPL-2.0-or-later
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0run.ps1" %*
exit /b %ERRORLEVEL%
pause