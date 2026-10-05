# ModelPeek v2.0 Release Packaging Script
$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$Gxx = Join-Path $Root "tools\w64devkit\bin\g++.exe"
$Windres = Join-Path $Root "tools\w64devkit\bin\windres.exe"

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "   ModelPeek v2.0 Packaging Pipeline" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

# 1. Trigger fresh build
Write-Host "[1/5] Running automated build pipeline..." -ForegroundColor Yellow
& powershell -ExecutionPolicy Bypass -File "$Root\scripts\build.ps1"

# 2. Prepare Staging
$Staging = Join-Path $Root "release_staging\ModelPeek_v2.0.0"
Write-Host "[2/5] Creating staging directory: $Staging" -ForegroundColor Yellow
if (Test-Path $Staging) { Remove-Item -Recurse -Force $Staging }
New-Item -ItemType Directory -Path $Staging | Out-Null

Copy-Item -Force "$Root\dist\ModelPeekExtension.dll" $Staging
Copy-Item -Force "$Root\dist\ModelPeekWorker.exe" $Staging
Copy-Item -Force "$Root\dist\ModelPeekSettings.exe" $Staging
Copy-Item -Force "$Root\dist\WebView2Loader.dll" $Staging
Copy-Item -Force "$Root\dist\cad_processor.py" $Staging
Copy-Item -Force "$Root\dist\install.bat" $Staging
Copy-Item -Force "$Root\dist\uninstall.bat" $Staging
Copy-Item -Recurse -Force "$Root\dist\viewer" $Staging

$ReadmeContent = @"
========================================================
     ModelPeek Windows Explorer 3D Extension v2.0.0
========================================================

ModelPeek 是一个轻量、高效、极其稳定的 Windows 资源管理器 3D/CAD 预览插件。
支持在文件夹中直接查看真实立体 3D 缩略图，并在右侧预览窗格（Alt+P）中进行全交互式的 3D 旋转、缩放、剖切与三维空间尺寸测量。

【v2.0 重磅新特性】
1. 零部件装配结构树 (Assembly Model Tree)：支持单选子零件高亮隔离、单独隐藏/显示切换；
2. 四大视口背景主题：深色科技、工业蓝图、摄影白底、透明棋盘格一键切换；
3. ModelPeek 控制中心 (ModelPeekSettings.exe)：可视化管理 16 种格式开关、缓存清理与状态诊断；
4. 命名管道守护进程加速 (Named Pipe IPC)：秒级连续批量缩略图生成；
5. 全新独立图形化安装向导 (ModelPeek_v2.0.0_Setup.exe)。

【支持文件格式（全 16 种）】
- CAD 工业级格式：.step, .stp, .iges, .igs, .brep, .brp
- 通用 3D 网格：.stl, .obj, .3mf, .glb, .gltf, .fbx (含网格与 NURBS 曲线)
- 点云与逆向扫描：.ply
- 制造加工刀路：.gcode
- 经典 3D 格式：.dae, .3ds

【安装方法（二选一）】
方法一（推荐）：双击运行 ModelPeek_v2.0.0_Setup.exe 图形化安装向导；
方法二（免安装便携版）：解压当前文件夹，右键以【管理员身份运行】install.bat。

【使用技巧】
1. 打开任意包含 3D 模型的文件夹即可看到立体缩略图；
2. 资源管理器中按键盘 Alt + P 快捷键开启/关闭右侧 3D 预览窗格；
3. 视口操作：左键旋转、右键平移、滚轮缩放；顶部工具栏可切换视图、线框模式、动态截面剖切、三维测距及零部件装配树；
4. 双击文件仍由您默认的 CAD/3D 专业软件打开，绝不抢占关联。

【卸载方法】
- 在 Windows“设置 - 安装的应用”中卸载，或运行 uninstall.bat 即可一键彻底清理。

GitHub: https://github.com/MIke-Slim/ModelPeek
"@

Set-Content -Path (Join-Path $Staging "README.txt") -Value $ReadmeContent -Encoding utf8

# 3. Create Portable Zip
$PortableZip = Join-Path $Root "ModelPeek_v2.0.0_Portable_x64.zip"
Write-Host "[3/5] Compressing portable release package: $PortableZip" -ForegroundColor Yellow
if (Test-Path $PortableZip) { Remove-Item -Force $PortableZip }
Compress-Archive -Path "$Staging\*" -DestinationPath $PortableZip -CompressionLevel Optimal
$ZipItem = Get-Item $PortableZip
Write-Host "  -> Success: $($ZipItem.Name) ($([math]::Round($ZipItem.Length / 1MB, 2)) MB)" -ForegroundColor Green

# 4. Build Standalone Installer (Setup.exe)
Write-Host "[4/5] Building standalone installer (ModelPeek_v2.0.0_Setup.exe)..." -ForegroundColor Yellow
$PayloadZip = Join-Path $Root "src\installer\ModelPeek_payload.zip"
Copy-Item -Force $PortableZip $PayloadZip

$InstallerRes = Join-Path $Root "dist\installer_res.o"
& $Windres -i "$Root\src\installer\installer.rc" -o $InstallerRes
if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to compile installer resource"
    exit $LASTEXITCODE
}

$SetupExe = Join-Path $Root "dist\ModelPeek_v2.0.0_Setup.exe"
& $Gxx -O2 -march=x86-64 -mwindows -municode -std=c++20 `
    "$Root\src\installer\main.cpp" $InstallerRes `
    -o $SetupExe `
    -lcomctl32 -lshlwapi -lshell32 -lole32 -luuid -ladvapi32 -luser32 -lgdi32 -static-libgcc -static-libstdc++

if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to compile ModelPeek_v2.0.0_Setup.exe"
    exit $LASTEXITCODE
}
$SetupItem = Get-Item $SetupExe
Write-Host "  -> Success: $($SetupItem.Name) ($([math]::Round($SetupItem.Length / 1MB, 2)) MB)" -ForegroundColor Green

# 5. Clean up temporary payload and staging
Write-Host "[5/5] Cleaning up temporary staging..." -ForegroundColor Yellow
Remove-Item -Force $PayloadZip -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force $Staging -ErrorAction SilentlyContinue
Remove-Item -Force $InstallerRes -ErrorAction SilentlyContinue

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "ModelPeek v2.0 Release Packages Ready!" -ForegroundColor Green
Write-Host "1. Installer: $SetupExe ($([math]::Round($SetupItem.Length / 1MB, 2)) MB)" -ForegroundColor Green
Write-Host "2. Portable:  $PortableZip ($([math]::Round($ZipItem.Length / 1MB, 2)) MB)" -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor Cyan
