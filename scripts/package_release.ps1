# ModelPeek Release Packaging Script
$ErrorActionPreference = "Stop"

$Version = "v2.1.2"
$VerNum = "2.1.2"
$Root = Split-Path -Parent $PSScriptRoot
$Gxx = Join-Path $Root "tools\w64devkit\bin\g++.exe"
$Windres = Join-Path $Root "tools\w64devkit\bin\windres.exe"

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "   ModelPeek $Version Packaging Pipeline" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

# 1. Trigger fresh build
Write-Host "[1/5] Running automated build pipeline..." -ForegroundColor Yellow
& pwsh -ExecutionPolicy Bypass -File "$Root\scripts\build.ps1"

# 2. Prepare Staging
$Staging = Join-Path $Root "release_staging\ModelPeek_$Version"
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
     ModelPeek Windows Explorer 3D Extension $Version
========================================================

ModelPeek 是一个轻量、高效、极其稳定的 Windows 资源管理器 3D/CAD 预览插件。
支持在文件夹中直接查看真实立体 3D 缩略图，并在右侧预览窗格（Alt+P）中进行全交互式的 3D 旋转、缩放、剖切、零部件装配树与工程三维标注。

【v2.1.1 重磅更新与稳定性加固】
1. Windows 11 多标签页（Tabbed Explorer）死句柄自愈：彻底根治标签页切换导致预览空白的问题；
2. AutoCAD 2023 专属图纸关联与防劫持：根除第三方办公套件对 DXF 图纸的强行接管，恢复原生 AutoCAD 图标与双击打开；
3. 全新高精度工业测试套件：提供包含点云 PCD、CAD IGES/STEP、刀路 GCODE 等全 18 种格式的 sample_models2 示例库；
4. 三维包围盒工程尺寸标注 (3D BBox Dimensions)：一键标注长宽高实体工程尺寸线与药丸标签；
5. 全格式覆盖 18 种工业/3D 数据格式；
6. 完整中英双语国际化跟随系统。

【支持文件格式（全 18 种）】
- CAD 工业级格式：.step, .stp, .iges, .igs, .brep, .brp, .dxf (AutoCAD 二维/三维图纸)
- 通用 3D 网格：.stl, .obj, .3mf, .glb, .gltf, .fbx (含网格与骨骼曲线)
- 点云与逆向扫描：.ply, .pcd (Point Cloud 激光雷达点云)
- 制造加工刀路：.gcode (CNC / 3D 打印切片刀轨)
- 经典 3D 格式：.dae, .3ds

【安装方法（二选一）】
方法一（推荐）：双击运行 ModelPeek_${Version}_Setup.exe 图形化安装向导；
方法二（便携免安装）：解压当前文件夹，右键以【管理员身份运行】install.bat。

【使用技巧】
1. 打开任意包含 3D 模型的文件夹即可看到立体缩略图；
2. 资源管理器中按键盘 Alt + P 快捷键开启/关闭右侧 3D 预览窗格；
3. 视口操作：左键旋转、右键平移、滚轮缩放；顶部工具栏可切换视图、线框模式、动态截面剖切、三维标注及零部件装配树；
4. 双击文件仍由您默认的 CAD/3D 专业软件打开，绝不抢占关联。

【卸载方法】
- 在 Windows“设置 - 安装的应用”中卸载，或运行 uninstall.bat 即可一键彻底清理。

GitHub: https://github.com/MIke-Slim/ModelPeek
"@

Set-Content -Path (Join-Path $Staging "README.txt") -Value $ReadmeContent -Encoding utf8

# 3. Create Portable Zip
$PortableZip = Join-Path $Root "ModelPeek_${Version}_Portable_x64.zip"
Write-Host "[3/5] Compressing portable release package: $PortableZip" -ForegroundColor Yellow
if (Test-Path $PortableZip) { Remove-Item -Force $PortableZip }
Compress-Archive -Path "$Staging\*" -DestinationPath $PortableZip -CompressionLevel Optimal
$ZipItem = Get-Item $PortableZip
Write-Host "  -> Success: $($ZipItem.Name) ($([math]::Round($ZipItem.Length / 1MB, 2)) MB)" -ForegroundColor Green

# 4. Build Standalone Installer (Setup.exe)
Write-Host "[4/5] Building standalone installer (ModelPeek_${Version}_Setup.exe)..." -ForegroundColor Yellow
$PayloadZip = Join-Path $Root "src\installer\ModelPeek_payload.zip"
Copy-Item -Force $PortableZip $PayloadZip

$InstallerRes = Join-Path $Root "dist\installer_res.o"
& $Windres -i "$Root\src\installer\installer.rc" -o $InstallerRes
if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to compile installer resource"
    exit $LASTEXITCODE
}

$SetupExe = Join-Path $Root "dist\ModelPeek_${Version}_Setup.exe"
& $Gxx -O2 -march=x86-64 -mwindows -municode -std=c++20 `
    "$Root\src\installer\main.cpp" $InstallerRes `
    -o $SetupExe `
    -lcomctl32 -lshlwapi -lshell32 -lole32 -luuid -ladvapi32 -luser32 -lgdi32 -static-libgcc -static-libstdc++

if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to compile ModelPeek_${Version}_Setup.exe"
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
Write-Host "ModelPeek $Version Release Packages Ready!" -ForegroundColor Green
Write-Host "1. Installer: $SetupExe ($([math]::Round($SetupItem.Length / 1MB, 2)) MB)" -ForegroundColor Green
Write-Host "2. Portable:  $PortableZip ($([math]::Round($ZipItem.Length / 1MB, 2)) MB)" -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor Cyan
