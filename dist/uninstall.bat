@echo off
setlocal
cd /d "%~dp0"

net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [ModelPeek] Requesting Administrator privileges...
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Start-Process cmd.exe -ArgumentList '/c \"\"%~f0\"\"' -Verb RunAs"
    exit /b
)

echo ========================================================
echo     ModelPeek Windows Explorer 3D Extension Uninstall
echo ========================================================
echo.

echo [1/2] Unregistering COM Shell Extension...
if exist "%~dp0ModelPeekExtension.dll" (
    regsvr32.exe /u /s "%~dp0ModelPeekExtension.dll"
    echo   [SUCCESS] COM Shell Extension unregistered.
) else (
    echo   [INFO] ModelPeekExtension.dll not found, skipping.
)

echo [2/2] Cleaning up preview host process (prevhost.exe)...
taskkill /f /im prevhost.exe >nul 2>&1

echo.
echo ========================================================
echo [SUCCESS] ModelPeek has been cleanly removed from system.
echo ========================================================
echo.
pause
