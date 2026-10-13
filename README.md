# ModelPeek - Windows 资源管理器 3D/CAD 极速预览神器

<p align="center">
  <b><a href="README.md">简体中文</a></b> | <b><a href="README_EN.md">English</a></b>
</p>

<p align="center">
  <a href="https://github.com/MIke-Slim/ModelPeek/releases"><img src="https://img.shields.io/github/v/release/MIke-Slim/ModelPeek?color=0078D7&label=Release" alt="GitHub Release"></a>
  <a href="https://github.com/MIke-Slim/ModelPeek"><img src="https://img.shields.io/badge/Platform-Windows%2010%20%7C%2011%20(x64)-blue" alt="Platform"></a>
  <a href="https://modelpeek.mythickoala2.chatgpt.site"><img src="https://img.shields.io/badge/Website-modelpeek-orange" alt="Official Website"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-MIT-green" alt="License"></a>
  <a href="https://buymeacoffee.com/mikeslim"><img src="https://img.shields.io/badge/Buy%20Me%20A%20Coffee-Donate-yellow.svg?logo=buy-me-a-coffee" alt="Buy Me A Coffee"></a>
  <a href="https://github.com/microsoft/winget-pkgs"><img src="https://img.shields.io/badge/Winget-MIke--Slim.ModelPeek-brightgreen" alt="Winget"></a>
</p>

**ModelPeek** 是一个专为 Windows 资源管理器打造的轻量、高效、极其稳定的 **3D/CAD 原生预览与缩略图扩展套件**。

无需启动庞大笨重的专业工业 CAD 软件，让 3D 与 CAD 模型文件像普通图片、PDF 一样，可以在 Windows 文件夹中直接呈现立体高清 3D 缩略图、在右侧预览窗格（快捷键 `Alt + P`）中进行全交互式的 3D 旋转缩放、动态剖切与工程尺寸标注！

🌐 **官方网站**：[https://modelpeek.mythickoala2.chatgpt.site](https://modelpeek.mythickoala2.chatgpt.site)  
📦 **项目源码**：[https://github.com/MIke-Slim/ModelPeek](https://github.com/MIke-Slim/ModelPeek)

---

## 📥 下载与安装 (Download & Installation)

进入 [GitHub Releases 页面](https://github.com/MIke-Slim/ModelPeek/releases) 或 [官网下载页面](https://modelpeek.mythickoala2.chatgpt.site) 获取最新安装包：

### 方式一：独立安装向导（强烈推荐）
1. 下载 **`ModelPeek_v2.1.3_Setup.exe`**；
2. 双击运行安装程序，点击 **【立即安装】**；
3. 程序自动完成 COM 扩展注册、关联 18 种三维格式、自动解除网络锁定，并在桌面和开始菜单生成管理快捷方式。

### 方式二：Windows 包管理器 (Winget)
在终端或 PowerShell 中运行：
```powershell
winget install MIke-Slim.ModelPeek
```

### 方式三：免安装便携绿色版 (Portable)
1. 下载 **`ModelPeek_v2.1.3_Portable_x64.zip`**；
2. 解压到您希望存放的任意目录（如 `D:\Tools\ModelPeek`）；
3. 右键点击 `install.bat`，选择 **【以管理员身份运行】** 即可完成一键激活；
4. 若需卸载，右键管理员运行 `uninstall.bat` 即可干净清除所有系统注册项。

> 💡 **卸载说明**：如需卸载，可通过 Windows【设置】-【已安装的应用】找到 ModelPeek 点击卸载，或运行安装目录下的 `ModelPeekUninstall.exe`，将自动深度清除所有注册表键值、快捷方式与临时缓存，零残留保护系统。

---

## 💡 使用教程与快速上手 (Quick Start & Usage)

### 1. 查看 3D 高清立体缩略图
安装完成后，打开任何存有 3D/CAD 文件的文件夹（如 STEP、STL、OBJ、DXF、3MF 等）：
- 将资源管理器视图切换为 **【大图标】** 或 **【超大图标】**；
- 模型图标将自动呈现带有真实光照阴影的立体 3D 缩略图，告别盲盒式文件名猜测。

### 2. 开启右侧 3D 交互式预览窗格
- 在 Windows 资源管理器中，按下键盘快捷键 **`Alt + P`**（或点击顶部菜单栏【查看】-【预览窗格】）；
- 单击选中任意支持的 3D 模型文件，右侧窗格瞬间加载流畅的 3D 交互视口。

### 3. 视口交互操作指南
| 操作手势 | 功能描述 |
| :--- | :--- |
| **鼠标左键拖拽** | 360° 自由轨道旋转视角，全方位观察模型结构 |
| **鼠标右键拖拽** | 平移视角（Pan） |
| **鼠标滚轮滑动** | 平滑缩放视距（Zoom） |
| **顶部视角工具栏** | 一键瞬间切换【等轴测 ISO】、【顶视】、【前视】、【右视】及【全局适配】 |
| **显示模式切换** | 切换【实体+边线特征】、【纯高光实体】或【线框网格】三种工业显示风格 |

### 4. 核心工程工具
- **📐 3D 包围盒工程尺寸标注**：点击工具栏【📐 标注】按钮，自动生成长宽高三维实体包围盒与尺寸界线，相机正对渲染毫米国标尺寸胶囊标签。
- **📏 3D 空间精确测距**：点击【📏 测距】按钮，鼠标射线拾取表面任意两点，实时精确测量真实欧几里得距离及 ΔX / ΔY / ΔZ 轴向分量。
- **✂️ 动态截面剖切**：点击【✂️ 剖切】按钮，拖动 X/Y/Z 轴向滑块，直观剖切观察内部中空腔体、螺纹和内部装配细节。
- **🌲 零部件装配结构树**：点击左上角装配树图标展开零件列表，支持单选零件高亮聚焦、单独隐藏/显示切换及顶点面数统计。
- **🎨 四大背景主题**：深色科技、工业蓝图、摄影白底、透明棋盘格一键切换。
- **🌐 语言切换**：视口右上角支持一键切换 `🇨🇳 简体中文` 与 `🇺🇸 English`。

### 5. 控制中心与设置 (`ModelPeekSettings.exe`)
桌面双击【ModelPeek 控制中心】：
- **格式开关**：自由勾选或取消 18 种格式中任意格式的缩略图与预览扩展；
- **🔓 解除模型网络锁定**：一键清除网络下载文件附带的 Mark-of-the-Web 锁定，消除 Windows 弹窗警告；
- **🧹 清理缩略图缓存**：一键刷新资源管理器图标缓存，强制重新渲染。

> 📖 **图文详细说明**：更多细节请参阅 [《ModelPeek 下载与使用说明书》](ModelPeek_使用说明书.md)。

---

## 📁 支持格式（全 18 种工业与通用格式）

| 分类 | 支持格式 | 技术亮点 |
| :--- | :--- | :--- |
| **CAD 工业级格式** | `.step`, `.stp`, `.iges`, `.igs`, `.brep`, `.brp`, `.dxf` | 采用 OpenCASCADE B-Rep 内核与纯原生 AutoCAD 矢量解析，支持实体面与曲线几何线框自适应渲染 |
| **通用 3D 网格** | `.stl`, `.obj`, `.3mf`, `.glb`, `.gltf`, `.fbx` | 支持 ASCII/Binary STL、材质贴图、多网格装配树与 FBX NURBS 曲线 |
| **点云与逆向扫描** | `.ply`, `.pcd` | 支持 Stanford PLY 真彩顶点色与 LiDAR/PCL 激光雷达点云粒子化渲染 |
| **制造加工与切片路径** | `.gcode` | 逐层刀轨路径 3D 空间可视化 |
| **多媒体与经典 3D 格式** | `.dae` (Collada), `.3ds` (3D Studio) | 经典三维交换资产无缝预览 |

---

## 🚀 v2.1.3 重磅特性与全格式零依赖加固

1. **🌟 18 种格式 100% 纯净系统全覆盖 (内置绿色嵌入式 Python 环境)**
   - 工业 CAD 核心格式（STEP, IGES, BREP, DXF, STL, OBJ, PLY, PCD, GCODE, 3DS）采用**纯原生 C++ 毫秒级直接解析**；
   - 针对现代网络与切片模型（GLTF, GLB, 3MF, DAE, FBX），内置 11MB 官方绿色嵌入式 Python 运行时，不改系统 PATH、不写注册表，在任何全新纯净虚拟机与离线内网电脑上 **18 种格式 100% 秒出立体 3D 缩略图**！

2. **🔓 一键解除 Windows 网络安全锁定 (Mark of the Web / Unblock)**
   - 控制中心（`ModelPeekSettings.exe`）新增【🔓 解除模型网络锁定】功能，并优化安装流程，一键消除从网络下载或虚拟机共享文件夹传来的“文件可能对你的计算机有害”Windows 系统级拦截。

3. **🩺 Windows 11 多标签页（Tabbed Explorer）死句柄自愈机制**
   - 彻底修复 Windows 11 资源管理器在选项卡切换或多开时，`prevhost.exe` 传递失效宿主句柄导致的 `CreateWindowEx err=1400`（无效窗口句柄）及右侧预览窗格空白问题。
   - 内置三级宿主窗口智能重定向与正向尺寸保底（`ResolveValidParent`），确保在任何复杂标签页操作下 100% 稳定加载。

4. **📐 AutoCAD 2023 专属图纸关联保护与防劫持**
   - 彻底根除部分第三方办公套件对 `.dxf` 文件的强行劫持；恢复原生 AutoCAD 2023 官方默认双击关联与专属图标，同时保留 ModelPeek 的高清 3D/2D 缩略图与预览窗格交互。

5. **📁 全新 19 款高精度工业模型测试套件 (`sample_models2`)**
   - 新增包含 PCD 激光雷达点云、IGES 超音速喷管曲面、BREP/STEP 阶梯轴、GCODE 螺旋塔刀路、DXF 机械法兰等涵盖全部 18 种格式的完整实测数据集。

6. **📐 三维包围盒工程尺寸标注 (3D BBox Dimensions)**
   - 工具栏新增【📐 标注】按钮，一键生成模型三维包围盒工程线框与尺寸界线。
   - 沿 X、Y、Z 轴向动态渲染相机正对的 3D Sprite 胶囊标签（`X: ... mm`、`Y: ... mm`、`Z: ... mm`），带深度去遮挡与高清晰度抗锯齿。

---

## 🌟 核心特性与架构设计

- **原生物理级 3D 缩略图 (`IThumbnailProvider`)**
  - 文件夹图标直接显示真实三维光照立体图。
  - 内置高性能离线**软件光栅化引擎 (Software Rasterizer)**，完全不占用 GPU 显存，毫秒级快速生成，永不因显卡驱动重置拖死资源管理器。
  - 本地二级磁盘缓存机制，二次打开文件夹瞬间秒开。

- **右侧 3D 交互式预览窗格 (`IPreviewHandler`)**
  - 选中模型文件后，右侧预览窗格（`Alt + P`）自动加载自包含 3D 视口。
  - 深度集成微软官方 Edge WebView2 运行时，GPU 硬件加速丝滑渲染 60 帧。

- **双击不抢占默认软件关联**
  - 双击 `.step` / `.stp` 仍由您原本的 SolidWorks / NX / FreeCAD / AutoCAD 打开；
  - 纯粹的 Preview 扩展，绝不破坏用户现有的工作流关联。

- **进程级隔离稳定性设计**
  - 资源管理器仅加载极轻量的 64 位 COM 宿主 DLL；
  - CAD 几何解析与网格化均由独立的后台 Worker 进程执行；
  - 哪怕遇到损坏模型或复杂超大装配，也绝不会引起 `explorer.exe` 崩溃。

---

## 🛠️ 本地编译与构建

项目采用完全便携的离线工具链，克隆代码后可一键编译打包：

```powershell
# 1. 编译核心 DLL、Worker 与控制中心
powershell -ExecutionPolicy Bypass -File scripts\build.ps1

# 2. 运行自动化全功能测试验证套件 (24 项测试)
powershell -ExecutionPolicy Bypass -File tests\test_all_features.ps1

# 3. 一键生成便携 Zip 与 Setup.exe 安装包
powershell -ExecutionPolicy Bypass -File scripts\package_release.ps1
```

### 目录结构说明
- `src/shell_ext/`：64 位 COM 原生扩展（`IThumbnailProvider`、`IPreviewHandler`、WebView2 宿主桥接、命名管道 IPC 客户端）
- `src/viewer/`：自包含 Three.js 交互视口（离线 HTML/CSS/JS、装配结构树、3D 包围盒标注、双语 i18n、主题切换）
- `src/worker/`：独立后台 Worker（内置原生 C++ STEP/IGES/BREP/DXF/PCD 解析器、软件光栅化渲染器、命名管道守护进程服务端）
- `src/settings/`：原生 Win32 GUI 配置中心（`ModelPeekSettings.exe`）
- `src/installer/`：原生现代 Fluent UI 安装向导与卸载向导（`ModelPeek_Setup.exe`、`ModelPeekUninstall.exe`）
- `manifests/`：官方 Microsoft Winget 软件包清单
- `sample_models/` 与 `sample_models2/`：18 种格式工业与 3D 测试验证用例集合
- `tests/`：自动化测试脚本套件
- `dist/`：打包编译产物发布目录

---

## ☕ 赞助与支持 (Sponsor / Buy Me a Coffee)

如果您觉得 **ModelPeek** 帮助您或您的团队提升了工作效率、改善了 3D/CAD 浏览体验，欢迎请作者喝一杯咖啡！☕  
您的每一份支持都将成为 ModelPeek 持续迭代、适配更多格式与优化性能的强劲动力！❤️

| 🌍 海外 / 国际赞助 (Buy Me a Coffee) | 🇨🇳 国内扫码赞助 (支付宝) |
| :---: | :---: |
| [![Buy Me A Coffee](https://img.shields.io/badge/Buy%20Me%20A%20Coffee-Donate%20$3-FFDD00?style=for-the-badge&logo=buy-me-a-coffee&logoColor=black)](https://buymeacoffee.com/mikeslim) | 点击下方展开扫码 |
| [👉 点此访问 Buy Me a Coffee 赞助页面](https://buymeacoffee.com/mikeslim) | <details><summary><b>📱 展开支付宝赞赏码</b></summary><br><img src="assets/donate/alipay.jpg" width="220" alt="Alipay QR Code"><br><i>打开手机支付宝扫一扫</i></details> |

> 感谢每一位支持开源的创作者与同行！✨

---

## 📄 开源许可证

本项目基于 [MIT License](LICENSE) 开源。欢迎 Star 与贡献代码！
