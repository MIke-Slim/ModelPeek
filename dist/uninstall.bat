@echo off
setlocal
cd /d "%~dp0"

:: Request Administrator privileges
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [ModelPeek] 请求管理员提权运行卸载程序...
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Start-Process cmd.exe -ArgumentList '/c \"\"%~f0\"\"' -Verb RunAs"
    exit /b
)

:: If native GUI uninstaller exists, delegate to it
if exist "%~dp0ModelPeekUninstall.exe" (
    start "" "%~dp0ModelPeekUninstall.exe"
    exit /b
)

echo ========================================================
echo       ModelPeek 3D/CAD 预览扩展 - 彻底卸载向导
echo ========================================================
echo.

set /p CONFIRM="是否确定彻底卸载 ModelPeek 并清理所有相关文件与关联？(Y/N): "
if /i "%CONFIRM%" neq "Y" (
    echo 取消卸载。
    pause
    exit /b
)

echo.
echo [1/5] 正在终止相关进程...
taskkill /f /im prevhost.exe >nul 2>&1
taskkill /f /im ModelPeekWorker.exe >nul 2>&1
taskkill /f /im ModelPeekSettings.exe >nul 2>&1

echo [2/5] 正在注销 64 位 COM 资源管理器扩展...
if exist "%~dp0ModelPeekExtension.dll" (
    regsvr32.exe /u /s "%~dp0ModelPeekExtension.dll"
)

echo [3/5] 正在清理系统注册表关联与控制面板卸载项...
reg delete "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\ModelPeek" /f >nul 2>&1
reg delete "HKLM\Software\Classes\CLSID\{E9B34A3E-94A5-47F1-A4FD-258F2C411311}" /f >nul 2>&1
reg delete "HKLM\Software\Classes\CLSID\{C81B4AE3-6C73-4DC1-8316-04DE665F09B1}" /f >nul 2>&1
reg delete "HKCR\CLSID\{E9B34A3E-94A5-47F1-A4FD-258F2C411311}" /f >nul 2>&1
reg delete "HKCR\CLSID\{C81B4AE3-6C73-4DC1-8316-04DE665F09B1}" /f >nul 2>&1
reg delete "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\PreviewHandlers" /v "{C81B4AE3-6C73-4DC1-8316-04DE665F09B1}" /f >nul 2>&1

for %%E in (.step .stp .iges .igs .brep .brp .dxf .stl .obj .3mf .glb .gltf .fbx .ply .pcd .gcode .dae .3ds) do (
    reg delete "HKLM\Software\Classes\%%E\ShellEx\{e357fccd-a995-4576-b01f-234630154e96}" /f >nul 2>&1
    reg delete "HKLM\Software\Classes\%%E\ShellEx\{8895b1c6-b41f-4c1c-a562-0d564250836f}" /f >nul 2>&1
    reg delete "HKCR\%%E\ShellEx\{e357fccd-a995-4576-b01f-234630154e96}" /f >nul 2>&1
    reg delete "HKCR\%%E\ShellEx\{8895b1c6-b41f-4c1c-a562-0d564250836f}" /f >nul 2>&1
)

echo [4/5] 正在删除桌面与开始菜单快捷方式...
del /f /q "%ProgramData%\Microsoft\Windows\Start Menu\Programs\ModelPeek\*.lnk" >nul 2>&1
rmdir /s /q "%ProgramData%\Microsoft\Windows\Start Menu\Programs\ModelPeek" >nul 2>&1
del /f /q "%APPDATA%\Microsoft\Windows\Start Menu\Programs\ModelPeek\*.lnk" >nul 2>&1
rmdir /s /q "%APPDATA%\Microsoft\Windows\Start Menu\Programs\ModelPeek" >nul 2>&1
del /f /q "%PUBLIC%\Desktop\ModelPeek 控制中心.lnk" >nul 2>&1
del /f /q "%USERPROFILE%\Desktop\ModelPeek 控制中心.lnk" >nul 2>&1
rmdir /s /q "%LOCALAPPDATA%\ModelPeek" >nul 2>&1

echo [5/5] 准备彻底删除程序文件夹...
set "TARGET_DIR=%~dp0"
:: Remove trailing backslash if present
if "%TARGET_DIR:~-1%"=="\" set "TARGET_DIR=%TARGET_DIR:~0,-1%"

:: Create self-deleting cleanup batch in %TEMP%
(
    echo @echo off
    echo timeout /t 1 /nobreak ^>nul
    echo taskkill /f /im prevhost.exe ^>nul 2^>^&1
    echo rd /s /q "%TARGET_DIR%"
    echo if exist "%TARGET_DIR%" ^(
    echo     timeout /t 1 /nobreak ^>nul
    echo     rd /s /q "%TARGET_DIR%"
    echo ^)
    echo powershell -NoProfile -Command "Start-Process explorer.exe" ^>nul 2^>^&1
    echo msg * "ModelPeek 已成功彻底卸载，所有程序文件与注册表关联均已清理完毕！"
    echo del "%%~f0"
) > "%TEMP%\modelpeek_uninstall_cleanup.bat"

echo.
echo ========================================================
echo [SUCCESS] 卸载清理已交接执行，该文件夹即将被彻底删除！
echo ========================================================
start "" /b "%TEMP%\modelpeek_uninstall_cleanup.bat"
exit
