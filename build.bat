@echo off
setlocal

REM Check if configuration parameter is provided
echo Building in Release mode
powershell -ExecutionPolicy Bypass -File "%~dp0build.ps1"

if %errorlevel% neq 0 (
    echo Build failed with error code %errorlevel%
    exit /b %errorlevel%
)

echo Build completed successfully
exit /b 0
