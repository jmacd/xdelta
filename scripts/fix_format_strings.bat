@echo off
echo Running format string fix script for all C/C++ files...
powershell -ExecutionPolicy Bypass -File "%~dp0\fix_format_strings.ps1"
echo.
echo Script completed.
pause
