# ModelPeek Automated Build Script
$ErrorActionPreference = "Stop"

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "   ModelPeek Build Pipeline" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

$Root = Split-Path -Parent $PSScriptRoot
$Gxx = Join-Path $Root "tools\w64devkit\bin\g++.exe"

if (-not (Test-Path $Gxx)) {
    Write-Error "Compiler not found at: $Gxx"
    exit 1
}

# 1. Build ModelPeekExtension.dll
Stop-Process -Name prevhost -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 200
Write-Host "[1/3] Compiling ModelPeekExtension.dll..." -ForegroundColor Yellow
& $Gxx -shared -O2 -march=x86-64 -std=c++20 `
    -I "$Root\src\shell_ext" -I "$Root\src\shell_ext\webview2" `
    "$Root\src\shell_ext\DllMain.cpp" `
    "$Root\src\shell_ext\ModelPeekThumbnailProvider.cpp" `
    "$Root\src\shell_ext\ModelPeekPreviewHandler.cpp" `
    "$Root\src\shell_ext\ModelPeekExtension.def" `
    -o "$Root\dist\ModelPeekExtension.dll" `
    "$Root\tools\WebView2Loader.dll.lib" `
    -lshlwapi -lole32 -luuid -lshell32 -luser32 -lgdi32 -ladvapi32 -static-libgcc -static-libstdc++

if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to build ModelPeekExtension.dll"
    exit $LASTEXITCODE
}
Write-Host "  -> Success: dist\ModelPeekExtension.dll" -ForegroundColor Green

# 2. Build ModelPeekWorker.exe
Write-Host "[2/3] Compiling ModelPeekWorker.exe..." -ForegroundColor Yellow
& $Gxx -O2 -march=x86-64 `
    "$Root\src\worker\main.cpp" `
    -o "$Root\dist\ModelPeekWorker.exe" `
    -lshlwapi -static-libgcc -static-libstdc++

if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to build ModelPeekWorker.exe"
    exit $LASTEXITCODE
}
Write-Host "  -> Success: dist\ModelPeekWorker.exe" -ForegroundColor Green

# 3. Build ModelPeekSettings.exe (GUI Control Panel)
Write-Host "[3/4] Compiling ModelPeekSettings.exe..." -ForegroundColor Yellow
& $Gxx -O2 -march=x86-64 -mwindows -municode -std=c++20 `
    "$Root\src\settings\main.cpp" `
    -o "$Root\dist\ModelPeekSettings.exe" `
    -lcomctl32 -lshlwapi -lshell32 -ladvapi32 -luser32 -lgdi32 -static-libgcc -static-libstdc++

if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to build ModelPeekSettings.exe"
    exit $LASTEXITCODE
}
Write-Host "  -> Success: dist\ModelPeekSettings.exe" -ForegroundColor Green

# 4. Synchronize Web Viewer Assets and Scripts
Write-Host "[4/4] Synchronizing Viewer and Worker scripts to dist..." -ForegroundColor Yellow
if (-not (Test-Path "$Root\dist\viewer")) { New-Item -ItemType Directory -Path "$Root\dist\viewer" | Out-Null }
Copy-Item -Recurse -Force "$Root\src\viewer\*" "$Root\dist\viewer"
Copy-Item -Force "$Root\src\worker\cad_processor.py" "$Root\dist\cad_processor.py"
Write-Host "  -> Success: dist\viewer and cad_processor.py updated" -ForegroundColor Green

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "ModelPeek Build Completed Successfully!" -ForegroundColor Green
Write-Host "Output Directory: $Root\dist" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
