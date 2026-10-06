# ModelPeek v2.1.2 Automated Test Suite
$ErrorActionPreference = "Stop"

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "   ModelPeek v2.1.2 Verification Suite   " -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

$Root = Split-Path -Parent $PSScriptRoot
$Passed = 0
$Failed = 0

function Assert-Check($name, [scriptblock]$condition) {
    try {
        $result = & $condition
        if ($result) {
            Write-Host "  [PASS] $name" -ForegroundColor Green
            $script:Passed++
        } else {
            Write-Host "  [FAIL] $name" -ForegroundColor Red
            $script:Failed++
        }
    } catch {
        Write-Host "  [FAIL] $name - Exception: $_" -ForegroundColor Red
        $script:Failed++
    }
}

Write-Host "`n1. Verifying Built Executables & Binaries..." -ForegroundColor Yellow
Assert-Check "ModelPeekExtension.dll exists" { Test-Path "$Root\dist\ModelPeekExtension.dll" }
Assert-Check "ModelPeekWorker.exe exists" { Test-Path "$Root\dist\ModelPeekWorker.exe" }
Assert-Check "ModelPeekSettings.exe exists" { Test-Path "$Root\dist\ModelPeekSettings.exe" }
Assert-Check "WebView2Loader.dll exists" { Test-Path "$Root\dist\WebView2Loader.dll" }

Write-Host "`n2. Verifying Viewer Engine & Loaders..." -ForegroundColor Yellow
Assert-Check "dist/viewer/index.html exists" { Test-Path "$Root\dist\viewer\index.html" }
Assert-Check "dist/viewer/viewer.js exists" { Test-Path "$Root\dist\viewer\viewer.js" }
Assert-Check "dist/viewer/libs/DXFLoader.js exists" { Test-Path "$Root\dist\viewer\libs\DXFLoader.js" }
Assert-Check "dist/viewer/libs/PCDLoader.js exists" { Test-Path "$Root\dist\viewer\libs\PCDLoader.js" }
Assert-Check "viewer.js Node syntax valid" { 
    $proc = Start-Process node -ArgumentList "-c `"$Root\dist\viewer\viewer.js`"" -Wait -PassThru -NoNewWindow
    $proc.ExitCode -eq 0
}
Assert-Check "DXFLoader.js Node syntax valid" { 
    $proc = Start-Process node -ArgumentList "-c `"$Root\dist\viewer\libs\DXFLoader.js`"" -Wait -PassThru -NoNewWindow
    $proc.ExitCode -eq 0
}

Write-Host "`n3. Verifying Multi-Format Thumbnail Rasterization..." -ForegroundColor Yellow
$Worker = "$Root\dist\ModelPeekWorker.exe"
$TempDir = [System.IO.Path]::GetTempPath()

$formatsToTest = @(
    @{ Name = "STL";        File = "sample_models\test_flange.stl" },
    @{ Name = "OBJ";        File = "sample_models\tree.obj" },
    @{ Name = "PLY";        File = "sample_models\dolphins.ply" },
    @{ Name = "DXF";        File = "sample_models\mechanical_layout.dxf" },
    @{ Name = "PCD";        File = "sample_models\lidar_scan.pcd" },
    @{ Name = "GCODE";      File = "sample_models\benchy.gcode" },
    @{ Name = "GLTF";       File = "sample_models\box_embedded.gltf" },
    @{ Name = "GLB";        File = "sample_models\DamagedHelmet.glb" },
    @{ Name = "3DS";        File = "sample_models\portalgun.3ds" },
    @{ Name = "DAE";        File = "sample_models\elf.dae" },
    @{ Name = "3MF";        File = "sample_models\cube_gears.3mf" },
    @{ Name = "FBX";        File = "sample_models\stanford_bunny.fbx" },
    @{ Name = "STEP";       File = "sample_models\test_flange.step" },
    @{ Name = "IGES";       File = "sample_models\sample_bracket.iges" },
    @{ Name = "BREP";       File = "sample_models\sample_bracket.brep" }
)

foreach ($fmt in $formatsToTest) {
    $outBmp = Join-Path $TempDir "test_thumb_$($fmt.Name).bmp"
    if (Test-Path $outBmp) { Remove-Item -Force $outBmp }
    $inFile = Join-Path $Root $fmt.File
    
    $proc = Start-Process $Worker -ArgumentList "thumbnail `"$inFile`" `"$outBmp`" 256" -Wait -PassThru -NoNewWindow
    $ok = ($proc.ExitCode -eq 0) -and (Test-Path $outBmp) -and ((Get-Item $outBmp).Length -eq 262198)
    Assert-Check "Thumbnail Rasterizer: $($fmt.Name)" { $ok }
    if (Test-Path $outBmp) { Remove-Item -Force $outBmp }
}

Write-Host "`n4. Verifying Native CAD Converter (STEP/IGES -> STL)..." -ForegroundColor Yellow
$testStep = Join-Path $Root "sample_models\test_flange.step"
$testOutStl = Join-Path $TempDir "test_flange_converted.stl"
if (Test-Path $testOutStl) { Remove-Item -Force $testOutStl }
$procConv = Start-Process $Worker -ArgumentList "convert `"$testStep`" `"$testOutStl`"" -Wait -PassThru -NoNewWindow
$convOk = ($procConv.ExitCode -eq 0) -and (Test-Path $testOutStl) -and ((Get-Item $testOutStl).Length -gt 100)
Assert-Check "Native STEP -> STL Conversion" { $convOk }
if (Test-Path $testOutStl) { Remove-Item -Force $testOutStl }

Write-Host "`n5. Verifying Winget Manifests & Community Standards..." -ForegroundColor Yellow
$WingetDir = "$Root\manifests\m\MIke-Slim\ModelPeek\2.1.0"
Assert-Check "Winget version manifest exists" { Test-Path "$WingetDir\MIke-Slim.ModelPeek.yaml" }
Assert-Check "Winget installer manifest exists" { Test-Path "$WingetDir\MIke-Slim.ModelPeek.installer.yaml" }
Assert-Check "Winget en-US locale exists" { Test-Path "$WingetDir\MIke-Slim.ModelPeek.locale.en-US.yaml" }
Assert-Check "Winget zh-CN locale exists" { Test-Path "$WingetDir\MIke-Slim.ModelPeek.locale.zh-CN.yaml" }
Assert-Check "Bug report template exists" { Test-Path "$Root\.github\ISSUE_TEMPLATE\bug_report.md" }
Assert-Check "Feature request template exists" { Test-Path "$Root\.github\ISSUE_TEMPLATE\feature_request.md" }
Assert-Check "PR template exists" { Test-Path "$Root\.github\PULL_REQUEST_TEMPLATE.md" }

Write-Host "`n=========================================" -ForegroundColor Cyan
Write-Host "Test Results: $Passed Passed, $Failed Failed" -ForegroundColor $(if ($Failed -eq 0) { "Green" } else { "Red" })
Write-Host "=========================================" -ForegroundColor Cyan

if ($Failed -gt 0) {
    exit 1
} else {
    exit 0
}
