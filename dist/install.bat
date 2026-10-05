@echo off
setlocal
cd /d "%~dp0"

:: Request Administrator privileges
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [ModelPeek] Requesting Administrator privileges...
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Start-Process cmd.exe -ArgumentList '/c \"\"%~f0\"\"' -Verb RunAs"
    exit /b
)

echo ========================================================
echo      ModelPeek Windows Explorer 3D Extension Setup
echo ========================================================
echo.
echo [1/3] Checking ModelPeekExtension.dll...
if not exist "%~dp0ModelPeekExtension.dll" (
    echo [ERROR] ModelPeekExtension.dll not found in current folder!
    pause
    exit /b 1
)

echo [2/3] Registering 64-bit COM Shell Extension...
taskkill /f /im prevhost.exe >nul 2>&1
regsvr32.exe /s "%~dp0ModelPeekExtension.dll"
if %errorLevel% equ 0 (
    echo   [SUCCESS] COM Extension registered successfully!
) else (
    echo   [ERROR] Registration failed. Error code: %errorLevel%
    pause
    exit /b %errorLevel%
)

echo [3/3] Refreshing Windows Explorer shell cache...
powershell -NoProfile -Command "[void][System.Reflection.Assembly]::LoadWithPartialName('System.Windows.Forms'); [System.Windows.Forms.SendKeys]::SendWait('{F5}')" >nul 2>&1

echo.
echo ========================================================
echo [SUCCESS] ModelPeek has been installed and activated!
echo.
echo Supported Formats:
echo   - CAD:  .step / .stp, .iges / .igs, .brep / .brp
echo   - Mesh: .stl, .obj, .glb, .gltf, .3mf, .fbx, .ply, .dae, .3ds, .gcode
echo.
echo How to use:
echo   1. Open any folder containing 3D/CAD files to view 3D thumbnails.
echo   2. Press Alt + P in Explorer to toggle the 3D Preview Pane.
echo   3. Double-clicking files still opens your default CAD software.
echo ========================================================
echo.
pause
