# ModelPeek Release Packaging Script
$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$Staging = Join-Path $Root "release_staging\ModelPeek_v1.0.0"
$ZipPath = Join-Path $Root "ModelPeek_v1.0.0_Portable_x64.zip"

Write-Host "Creating staging directory: $Staging" -ForegroundColor Cyan
if (Test-Path $Staging) { Remove-Item -Recurse -Force $Staging }
New-Item -ItemType Directory -Path $Staging | Out-Null

# Copy release components from dist
Copy-Item -Force "$Root\dist\ModelPeekExtension.dll" $Staging
Copy-Item -Force "$Root\dist\ModelPeekWorker.exe" $Staging
Copy-Item -Force "$Root\dist\WebView2Loader.dll" $Staging
Copy-Item -Force "$Root\dist\cad_processor.py" $Staging
Copy-Item -Force "$Root\dist\install.bat" $Staging
Copy-Item -Force "$Root\dist\uninstall.bat" $Staging
Copy-Item -Recurse -Force "$Root\dist\viewer" $Staging

# Create user-friendly README.txt
$ReadmeContent = @"
========================================================
     ModelPeek Windows Explorer 3D Extension v1.0.0
========================================================

ModelPeek 是一个轻量、高效、极其稳定的 Windows 资源管理器 3D/CAD 预览插件。
支持在文件夹中直接查看真实立体 3D 缩略图，并在右侧预览窗格（Alt+P）中进行全交互式的 3D 旋转、缩放、剖切与三维空间尺寸测量。

【支持文件格式】
- CAD 工业级格式：.step, .stp, .iges, .igs, .brep, .brp
- 通用 3D 网格：.stl, .obj, .3mf, .glb, .gltf, .fbx
- 点云与逆向扫描：.ply
- 制造加工刀路：.gcode
- 经典 3D 格式：.dae, .3ds

【安装方法】
1. 将当前文件夹解压并存放在您想要的位置（如 C:\Program Files\ModelPeek 或任意固定路径）；
2. 鼠标右键以【管理员身份运行】 install.bat；
3. 提示成功后即可立即生效！

【使用技巧】
1. 打开任意包含 3D 模型的文件夹即可看到立体缩略图；
2. 资源管理器中按键盘 Alt + P 快捷键开启/关闭右侧 3D 预览窗格；
3. 视口操作：左键旋转、右键平移、滚轮缩放；顶部工具栏可切换视图、线框模式、动态截面剖切及三维测距；
4. 双击文件仍由您默认的 CAD/3D 专业软件打开，绝不抢占关联。

【卸载方法】
- 鼠标右键以【管理员身份运行】 uninstall.bat 即可一键彻底清理注销。

GitHub: https://github.com/MIke-Slim/ModelPeek
"@

Set-Content -Path (Join-Path $Staging "README.txt") -Value $ReadmeContent -Encoding utf8

# Create zip archive
Write-Host "Compressing to $ZipPath..." -ForegroundColor Cyan
if (Test-Path $ZipPath) { Remove-Item -Force $ZipPath }
Compress-Archive -Path "$Staging\*" -DestinationPath $ZipPath -CompressionLevel Optimal

$ZipItem = Get-Item $ZipPath
$SizeMB = [math]::Round($ZipItem.Length / 1MB, 2)
Write-Host "=========================================" -ForegroundColor Green
Write-Host "Release package ready: $($ZipItem.Name) ($SizeMB MB)" -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor Green
