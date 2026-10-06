# -*- coding: utf-8 -*-
import os
import json

base_dirs = [
    r"c:\Users\20858\Desktop\ModelPeek_开发\AI视频生成物料包_ModelPeek",
    r"C:\Users\20858\Desktop\AI视频生成物料包_ModelPeek"
]

shots_data = [
    {
        "shot_id": 1,
        "name": "镜头1_痛点开场_CAD加载卡顿",
        "duration_seconds": 4,
        "ref_image": "01_分镜头参考帧_Images/镜头1_痛点开场_CAD加载卡顿.jpg",
        "voiceover": "还在为了看一眼零件，苦等大型CAD软件慢吞吞加载吗？",
        "camera_motion": "缓慢推进特写 (Slow Zoom In)",
        "prompt_zh": "电影级写实镜头，特写镜头。深夜科技感工作室内，一位年轻疲惫的机械工程师坐在电脑前，鼠标焦急地反复双击一个3D模型文件。屏幕中央卡在厚重的CAD软件启动加载圈上，时钟指针飞速转动的全息虚影掠过，暗调冷光与暖色台灯对比，慢速推镜头，体现等待的焦急与漫长，超高清4K写实。",
        "prompt_en": "Cinematic close-up, photorealistic 8k. Late night dimly lit modern engineering office. A tired mechanical engineer sits in front of computer monitors, impatiently double-clicking a file. On the screen, a stuck loading spinner animation keeps freezing on heavy CAD software startup. Neon blue and warm desk ambient lighting, subtle camera slow push-in, shallow depth of field, dramatic frustration mood, hyper-detailed."
    },
    {
        "shot_id": 2,
        "name": "镜头2_神器登场_资源管理器3D缩略图",
        "duration_seconds": 4,
        "ref_image": "01_分镜头参考帧_Images/镜头2_神器登场_资源管理器3D缩略图.png",
        "voiceover": "ModelPeek：Windows 原生 3D/CAD 缩略图引擎，一眼看清每个零件！",
        "camera_motion": "横向微摇平移 (Pan Right / Dolly)",
        "prompt_zh": "惊艳反转，微距推轨镜头。Windows 11 资源管理器文件夹窗口，一道璀璨的科技流光自左向右横扫过文件夹界面。原本单调的 STEP 和 DXF 图标瞬间激活，蜕变成漂浮立体、色彩鲜活的高清 3D 机械零件模型（金属法兰、金色涡轮、精密机械臂齿轮）。金属质感反光，玻璃拟态透明特效，极具视觉爽感，4K 高清。",
        "prompt_en": "Macro dolly shot, extreme clarity. Windows 11 File Explorer interface. A beam of bright cyan futuristic digital energy sweeps across the file list. The flat CAD file icons instantly transform into vibrant, illuminated, realistic 3D floating models: metallic turbines, golden gear assemblies, and robotic parts. Soft reflections, fluid motion graphics, tech product commercial aesthetic, octane render, 8k."
    },
    {
        "shot_id": 3,
        "name": "镜头3_核心功能_右侧窗格3D旋转",
        "duration_seconds": 5,
        "ref_image": "01_分镜头参考帧_Images/镜头3_核心功能_右侧窗格3D旋转.png",
        "voiceover": "按下 Alt+P，右侧窗格直接 360° 无死角 3D 旋转预览，无需打开任何建模软件！",
        "camera_motion": "环绕 360 度旋转 (Orbit 360 / Smooth Pan)",
        "prompt_zh": "科技广告特写镜头。电脑屏幕右侧的 ModelPeek 预览视口内，一个极度精密的机械涡轮模型正在平滑优雅地进行 360 度三维全景旋转。鼠标光标在屏幕上拖拽，伴随着动态环境反射与柔和的工作室打光。界面操作如丝般顺滑，毫无任何卡顿，充满高端工业设计质感，科技感十足。",
        "prompt_en": "Smooth product showcase camera track. Inside the right-pane preview viewport of Windows Explorer, an intricate industrial aerospace engine model smoothly rotates 360 degrees in full 3D. Sleek cursor interaction, realistic metallic reflections, clean studio rim light, fluid framerate, buttery smooth interactivity, Apple product commercial style, ultra high definition."
    },
    {
        "shot_id": 4,
        "name": "镜头4_黑科技_线框网格与工程尺寸标注",
        "duration_seconds": 5,
        "ref_image": "01_分镜头参考帧_Images/镜头4_黑科技_线框网格与工程尺寸标注.png",
        "voiceover": "一键透视内部结构，实体长宽高毫米级工程尺寸、面数参数全息标注！",
        "camera_motion": "向前推进微仰 (Forward Zoom + Tilt Up)",
        "prompt_zh": "科幻全息风格，极具视觉冲击力。3D 机械零件模型突然半透明化，透出内部密集的绿色与青色发光网格线框（Wireframe）。模型四周自动弹射生成未来感三维包围盒与毫米级工程尺寸标注线（3D Bounding Box Dimension），数字数据流HUD在零件周围律动悬浮，宛如钢铁侠贾维斯的全息装配台，极致精密，超高清。",
        "prompt_en": "Sci-fi holographic transformation. The solid 3D cad part seamlessly transitions into a glowing neon wireframe mesh. Around the object, dynamic holographic 3D bounding box dimension lines and glowing millimeter measurements pop out in mid-air. Futuristic HUD telemetry showing vertex counts and geometry data, Iron Man holographic lab style, clean high-tech CGI aesthetic, 8k."
    },
    {
        "shot_id": 5,
        "name": "镜头5_正交三视图_HUD参数测量",
        "duration_seconds": 4,
        "ref_image": "01_分镜头参考帧_Images/镜头5_正交三视图_HUD参数测量.png",
        "voiceover": "支持标准正交工程视图与技术指标即时测量，严谨高效！",
        "camera_motion": "俯视推平 (Top-down Dolly)",
        "prompt_zh": "工业工程美学特写。机械零件快速切换至俯视正交工程视角，发光的科技工程网格底板平铺展开。画面角落的半透明HUD控制台跳动显示零件精密参数：长宽高毫米读数、三角面数、体积数据，画面干净利落，极具专业感。",
        "prompt_en": "Industrial engineering aesthetic. The 3D model snaps into a precise top orthographic engineering blueprint view over a glowing technical grid plane. Translucent HUD displays real-time CAD metrics: exact dimensions in millimeters, face count, and vertex telemetry. Clean, crisp, professional aerospace design style, 8k."
    },
    {
        "shot_id": 6,
        "name": "镜头6_18种格式矩阵与开源汇总",
        "duration_seconds": 4,
        "ref_image": "01_分镜头参考帧_Images/镜头6_18种格式矩阵与开源汇总.png",
        "voiceover": "全面支持 18 种工业与 3D 格式，完全开源、纯净免费！立即前往 GitHub 体验！",
        "camera_motion": "广角快速拉远 (Zoom Out / Pull Back)",
        "prompt_zh": "史诗感大结局，广角镜头迅速拉远。十八种工业CAD与3D格式名称（.STEP .IGES .DXF .STL .OBJ .GCODE）化为流光卡片矩阵，迅速环绕飞入并汇聚到 ModelPeek 极简软件图标中。背景散开微弱粒子光芒，屏幕居中浮现出白色加粗艺术字体：“ModelPeek 纯开源 · 极速免费”，伴随 GitHub 猫咪标志，充满质感与高级感。",
        "prompt_en": "Epic cinematic outro, zoom out wide shot. 18 CAD and 3D file format badges (.STEP, .DXF, .STL, .OBJ, .GCODE) swirl like particles and merge into the glowing ModelPeek logo. A clean modern workspace background with soft ambient lighting. Text overlays boldly in center: \"Free & Open Source on GitHub\". Sleek, inspiring, premium open-source software commercial, 8k."
    }
]

# Generate Markdown
md_content = """# 🎬 ModelPeek AI 视频生成物料包与分镜提示词 (Prompts)

本文件夹专门为 **豆包 (Doubao) / 即梦 (Dreamina) / 可灵 (Kling) / 海螺 (MiniMax) / Runway** 等 AI 视频生成工具定制。
包含完整分镜脚本、双语提示词、镜头运镜指令以及对应的参考帧图片。

---

## 📌 豆包 AI 一键使用指令（直接发给豆包）
> **给豆包的指令模板**：  
> “你好豆包，我提供了一个专门制作《ModelPeek 3D/CAD 预览扩展》产品宣传片的分镜物料包。请根据本目录中的参考帧图片与对应的中英文提示词，按照 6 个分镜头顺序帮我生成高质量科技感视频，每个镜头的运镜、时长与旁白请严格按照配置文件执行。”

---

## 🎞️ 分镜头提示词与参考帧映射表

"""

for shot in shots_data:
    md_content += f"""### 【{shot['name']}】
- **推荐时长**：{shot['duration_seconds']} 秒
- **参考帧图片**：`{shot['ref_image']}`
- **运镜方式**：{shot['camera_motion']}
- **画面旁白/字幕**：“{shot['voiceover']}”

#### 🇨🇳 中文提示词（适用：豆包、即梦、可灵、海螺、Vidu）：
```text
{shot['prompt_zh']}
```

#### 🌍 英文提示词（适用：Runway Gen-3、Sora、Luma Dream Machine、Pika）：
```text
{shot['prompt_en']}
```

---
"""

# Generate Plain Text
txt_content = """============================================================
       ModelPeek AI 视频生成分镜头脚本与提示词总汇 (豆包专用)
============================================================

【使用说明】
本文件包含 6 个核心分镜头的完整提示词。每个镜头都配有对应的参考图，请使用「图生视频 (Image-to-Video)」模式，将参考图作为首帧（Start Frame），并复制对应的中文或英文提示词生成。

------------------------------------------------------------
"""
for shot in shots_data:
    txt_content += f"""
【镜头 {shot['shot_id']}】：{shot['name']}
- 时长：{shot['duration_seconds']} 秒
- 参考图：{shot['ref_image']}
- 运镜：{shot['camera_motion']}
- 旁白配音：“{shot['voiceover']}”

[中文提示词]：
{shot['prompt_zh']}

[英文提示词]：
{shot['prompt_en']}
------------------------------------------------------------
"""

guide_txt = """【豆包 (Doubao) AI 专属对话指令】

直接复制下方整段话发送给豆包：

------------------------------------------------------------
豆包你好！我已经将《ModelPeek 原生 Windows 3D/CAD 预览扩展》的产品宣传片物料全部整理在这个文件夹里了。
文件夹内包含了 6 个镜头的首帧参考图片（位于 01_分镜头参考帧_Images 目录）以及详细的分镜头提示词（见 02_分镜脚本与提示词_Prompts.md 和 04_分镜配置数据_Prompts.json）。

请你读取这些分镜头内容，帮我完成以下两件事：
1. 按照这 6 个分镜头的顺序（痛点等待 -> 3D缩略图觉醒 -> 3D旋转交互 -> 线框尺寸标注 -> 正交三视图 -> 18种格式开源结尾），调用你的视频生成能力或给出精确的图生视频生成指导；
2. 为这段 26 秒的宣传片设计一套完整的旁白配音节奏与背景音乐卡点建议。
------------------------------------------------------------
"""

for b in base_dirs:
    os.makedirs(b, exist_ok=True)
    with open(os.path.join(b, "02_分镜脚本与提示词_Prompts.md"), "w", encoding="utf-8") as f:
        f.write(md_content)
    with open(os.path.join(b, "03_分镜脚本与提示词_Prompts.txt"), "w", encoding="utf-8") as f:
        f.write(txt_content)
    with open(os.path.join(b, "04_分镜配置数据_Prompts.json"), "w", encoding="utf-8") as f:
        json.dump(shots_data, f, ensure_ascii=False, indent=2)
    with open(os.path.join(b, "豆包AI一键对话指令_必读.txt"), "w", encoding="utf-8") as f:
        f.write(guide_txt)

print("Prompt files successfully generated in both locations!")
