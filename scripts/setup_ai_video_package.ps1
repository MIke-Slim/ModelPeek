$ErrorActionPreference = "Stop"

$workspaceDir = "c:\Users\20858\Desktop\ModelPeek_开发\AI视频生成物料包_ModelPeek"
$desktopDir = "c:\Users\20858\Desktop\AI视频生成物料包_ModelPeek"

foreach ($base in @($workspaceDir, $desktopDir)) {
    $imgDir = Join-Path $base "01_分镜头参考帧_Images"
    $moreDir = Join-Path $imgDir "更多高清备用素材"
    if (-not (Test-Path $moreDir)) {
        New-Item -ItemType Directory -Force -Path $moreDir | Out-Null
    }

    # Shot 1
    Copy-Item -Force "C:\Users\20858\.gemini\antigravity\brain\482ed2f6-f19b-4549-9e67-539cc453832a\shot_1_cad_loading_1791285376618.jpg" (Join-Path $imgDir "镜头1_痛点开场_CAD加载卡顿.jpg")
    
    # Shot 2-6
    Copy-Item -Force "c:\Users\20858\Desktop\ModelPeek_开发\media_generator\assets\scene_0_explorer_mockup.png" (Join-Path $imgDir "镜头2_神器登场_资源管理器3D缩略图.png")
    Copy-Item -Force "c:\Users\20858\Desktop\ModelPeek_开发\media_generator\assets\scene_2_rotated_3d.png" (Join-Path $imgDir "镜头3_核心功能_右侧窗格3D旋转.png")
    Copy-Item -Force "c:\Users\20858\Desktop\ModelPeek_开发\media_generator\assets\scene_3_wireframe.png" (Join-Path $imgDir "镜头4_黑科技_线框网格与工程尺寸标注.png")
    Copy-Item -Force "c:\Users\20858\Desktop\ModelPeek_开发\media_generator\assets\scene_4_top_ortho.png" (Join-Path $imgDir "镜头5_正交三视图_HUD参数测量.png")
    Copy-Item -Force "c:\Users\20858\Desktop\ModelPeek_开发\media_generator\assets\scene_7_fit_overview.png" (Join-Path $imgDir "镜头6_18种格式矩阵与开源汇总.png")

    # All original assets
    Get-ChildItem "c:\Users\20858\Desktop\ModelPeek_开发\media_generator\assets\*.png" | Copy-Item -Force -Destination $moreDir
}

Write-Host "Images initialized successfully in both workspace and desktop!"
