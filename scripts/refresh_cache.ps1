# ModelPeek Cache Refresh and Touch Script
Stop-Process -Name prevhost -Force -ErrorAction SilentlyContinue
Stop-Process -Name ModelPeekPeek -Force -ErrorAction SilentlyContinue

$cacheDir = Join-Path $env:LOCALAPPDATA "ModelPeek\cache\thumbnails"
if (Test-Path $cacheDir) {
    Get-ChildItem -Path $cacheDir -Recurse | Remove-Item -Force -ErrorAction SilentlyContinue
}

$previewMeshDir = Join-Path $env:USERPROFILE "AppData\LocalLow\ModelPeek\cache\preview_mesh"
if (Test-Path $previewMeshDir) {
    Get-ChildItem -Path $previewMeshDir -Recurse | Remove-Item -Force -ErrorAction SilentlyContinue
}

$Root = Split-Path -Parent $PSScriptRoot
$SampleModels = Join-Path $Root "sample_models"
if (Test-Path $SampleModels) {
    Get-ChildItem -Path $SampleModels -File | ForEach-Object {
        $_.LastWriteTime = [DateTime]::Now
    }
}

# Clean temporary test files in dist
Get-ChildItem -Path (Join-Path $Root "dist") -Filter "test_*" -File | Remove-Item -Force -ErrorAction SilentlyContinue
Get-ChildItem -Path (Join-Path $Root "dist") -Filter "thumb_test_*" -File | Remove-Item -Force -ErrorAction SilentlyContinue

Write-Host "ModelPeek cache refreshed and sample models touched successfully!"
