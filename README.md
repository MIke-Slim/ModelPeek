# ModelPeek - Windows 资源管理器 3D/CAD 极速预览神器

[![GitHub Release](https://img.shields.io/github/v/release/MIke-Slim/ModelPeek?color=0078D7&label=Release)](https://github.com/MIke-Slim/ModelPeek/releases)
[![Platform](https://img.shields.io/badge/Platform-Windows%2010%20%7C%2011%20(x64)-blue)](https://github.com/MIke-Slim/ModelPeek)
[![License](https://img.shields.io/badge/License-MIT-green)](LICENSE)

**ModelPeek** 是一个专为 Windows 资源管理器打造的轻量、高效、极其稳定的 **3D/CAD 预览扩展插件**。

无需启动庞大笨重的专业工业 CAD 软件，让 3D 与 CAD 模型文件像普通图片、PDF 一样，可以在 Windows 文件夹中直接呈现立体高质量缩略图，并在右侧预览窗格（`Alt + P`）中进行全交互式的 3D 旋转、缩放、剖切、空间测距及零部件装配树层级检视。

---

## 🚀 v2.0.0 重磅全新特性

1. **🌳 零部件装配结构树 (Assembly Model Tree)**
   - 交互式侧边抽屉面板，智能识别并层级展示装配体子零件（支持 3MF、FBX、OBJ 及多实体 CAD 模型）。
   - **子部件高亮隔离**：鼠标点击目标零件即时聚焦高亮，一目了然。
   - **可见性开关**：每个零件配备独立眼睛图标，支持自由隐藏/显示目标结构。
   - **实时统计**：直观展示各零部件的三角面数（Faces）与顶点数（Vertices）。

2. **🎨 四大视口背景主题 (Viewport Themes)**
   - **深色科技 (Dark Tech)**：经典暗色，专业护眼，高光反差鲜明（默认）。
   - **工业蓝图 (Engineering Blueprint)**：经典工程图纸深蓝背景，尽显硬核工程质感。
   - **摄影白底 (Studio White)**：纯净高光摄影棚风格，适合报告截图与展示。
   - **透明棋盘格 (Transparent Grid)**：经典透明网格背景，方便观察模型轮廓透明度。
   - 配套自适应光照系统与动态地网格，一键即时无缝切换。

3. **⚙️ 原生可视化控制中心 (`ModelPeekSettings.exe`)**
   - 纯 Win32 原生打造，秒速启动，零运行时依赖。
   - **格式开关矩阵**：16 种格式自由勾选，即时生效，随时启用/关闭任意文件扩展名关联。
   - **缓存智能管理**：实时计算磁盘缓存占用（`%LOCALAPPDATA%\ModelPeek\cache`），支持一键清理。
   - **缩略图缓存刷新**：一键清除 Windows 资源管理器缩略图历史缓存，解决系统图标不刷新的顽疾。
   - **系统环境自检**：一键诊断 COM 组件注册、Worker 守护进程、WebView2 运行库等健康状态。

4. **⚡ 命名管道守护进程 IPC (Daemon Mode & Fast-path IPC)**
   - Worker 进驻守护模式（`ModelPeekWorker.exe --daemon`），通过 Windows 高速命名管道（`\\.\pipe\ModelPeekWorkerPipe`）毫秒级通信。
   - 彻底免除高频打开大文件夹时反复创建/销毁进程的开销，批量缩略图生成吞吐量大幅跃升。

5. **📦 独立图形化安装向导 (`ModelPeek_v2.0.0_Setup.exe`)**
   - 单文件便携可执行包，无需预装解压软件，零外部网络依赖。
   - 具备 UAC 自动提权、自定义安装路径、静默部署、COM 注册、桌面/开始菜单快捷方式生成以及 Windows 规范卸载条目注册。

---

## 🌟 核心特性与架构设计

- **原生物理级 3D 缩略图 (`IThumbnailProvider`)**
  - 文件夹图标直接显示真实三维光照立体图。
  - 内置高性能离线**软件光栅化引擎 (Software Rasterizer)**，完全不占用 GPU 显存，毫秒级快速生成，永不因显卡驱动重置拖死资源管理器。
  - 本地二级磁盘缓存机制，二次打开文件夹瞬间秒开。

- **右侧 3D 交互式预览窗格 (`IPreviewHandler`)**
  - 选中模型文件后，右侧预览窗格（快捷键 `Alt + P`）自动加载自包含 3D 视口。
  - **交互操作**：
    - **鼠标左键拖拽**：360° 轨道旋转视角
    - **鼠标右键拖拽**：平移视角
    - **鼠标滚轮**：平滑缩放
    - **顶部快捷工具栏**：一键切换【等轴测】、【顶视】、【前视】、【右视】、【自适应重置】视角。
    - **渲染模式**：支持【实体+边线特征】、【纯高光实体】、【线框网格】三种显示风格。
    - **📏 3D 空间三维测距工具**：射线拾取表面任意两点，实时精确测量真实欧几里得空间距离及 ΔX / ΔY / ΔZ 轴向分量（单位：mm）。
    - **✂️ 动态剖切截面工具**：沿 X / Y / Z 轴向实时剖切观察内部中空腔体、螺纹和装配结构，支持深度滑块调节与反向剖切。
    - **HUD 模型信息面板**：实时计算并显示外形包围盒尺寸（长 × 宽 × 高 mm）、三角面数、顶点数量。

- **双击不抢占默认软件关联**
  - 双击 `.step` / `.stp` 仍由您原本的 SolidWorks / NX / FreeCAD 打开；
  - 纯粹的 Preview 扩展，绝不破坏用户现有的工作流关联。

- **进程级隔离稳定性设计**
  - 资源管理器仅加载极轻量的 64 位 COM 宿主 DLL；
  - CAD 几何解析与网格化均由独立的后台 Worker 进程执行；
  - 哪怕遇到损坏模型或复杂超大装配，也绝不会引起 `explorer.exe` 崩溃。

---

## 📁 支持格式（全 16 种工业与通用格式）

| 分类 | 支持格式 | 技术亮点 |
| :--- | :--- | :--- |
| **CAD 工业级格式** | `.step`, `.stp`, `.iges`, `.igs`, `.brep`, `.brp` | 采用 OpenCASCADE B-Rep / NURBS 工业级几何内核，支持实体面与曲线几何线框自适应渲染 |
| **通用 3D 网格** | `.stl`, `.obj`, `.3mf`, `.glb`, `.gltf`, `.fbx` | 支持 ASCII/Binary STL、材质贴图、多网格装配树与 FBX NURBS 曲线 |
| **高精度点云与逆向工程** | `.ply` | 支持真彩顶点色（Vertex Color）高效点云渲染 |
| **制造加工与切片路径** | `.gcode` | 逐层刀轨路径 3D 空间可视化 |
| **多媒体与经典 3D 格式** | `.dae` (Collada), `.3ds` (3D Studio) | 经典三维交换资产无缝预览 |

---

## 📥 下载与安装

进入 [Releases 页面](https://github.com/MIke-Slim/ModelPeek/releases) 下载最新发行版：

### 方式一：独立安装包（推荐）
1. 下载 **`ModelPeek_v2.0.0_Setup.exe`**；
2. 双击运行安装向导，按提示点击【下一步】完成安装；
3. 安装程序会自动完成 COM 注册、文件类型关联并在开始菜单创建快捷方式。

### 方式二：免安装便携绿色版
1. 下载 **`ModelPeek_v2.0.0_Portable_x64.zip`**；
2. 解压到您希望存放的任意目录（如 `C:\Program Files\ModelPeek`）；
3. 右键点击 `install.bat`，选择 **【以管理员身份运行】** 即可完成激活；
4. 若需卸载，右键管理员运行 `uninstall.bat` 即可干净清除，无残留。

---

## 💡 使用指南

1. **查看缩略图**：打开任意包含 3D/CAD 文件的文件夹，将资源管理器视图切换为“大图标”或“超大图标”，即可看到立体渲染图。
2. **开启预览窗格**：在资源管理器中按下键盘快捷键 **`Alt + P`**（或点击上方菜单栏的【查看】-【预览窗格】）。
3. **交互操作**：单击任意模型文件，右侧立即加载 3D 视口：
   - 鼠标左键旋转视角、右键平移、滚轮缩放；
   - 点击右上角主题图标随时切换背景风格；
   - 点击零件树图标展开装配体，点击眼睛图标隐藏部件；
   - 顶部工具栏启用三维测距尺或动态剖切滑块。
4. **控制中心**：启动 `ModelPeekSettings.exe`，自由开关支持格式或清理磁盘缓存。

---

## 🛠️ 本地编译与构建

项目采用完全便携的离线工具链，克隆代码后可一键编译打包：

```powershell
# 1. 编译核心 DLL、Worker 与控制中心
powershell -ExecutionPolicy Bypass -File scripts\build.ps1

# 2. 一键生成便携 Zip 与 Setup.exe 安装包
powershell -ExecutionPolicy Bypass -File scripts\package_release.ps1
```

### 目录结构说明
- `src/shell_ext/`：64 位 COM 原生扩展（`IThumbnailProvider`、`IPreviewHandler`、WebView2 宿主桥接、命名管道 IPC 客户端）
- `src/viewer/`：自包含 Three.js 交互视口（离线 HTML/CSS/JS、装配结构树、主题切换）
- `src/worker/`：独立后台 Worker（CAD 几何解析、软件光栅化渲染器、命名管道守护进程服务端）
- `src/settings/`：原生 Win32 GUI 配置中心（`ModelPeekSettings.exe`）
- `src/installer/`：原生单文件安装向导（`ModelPeek_Setup.exe`）
- `sample_models/`：测试验证用例集合
- `dist/`：打包编译产物发布目录

---

## 📄 开源许可证

本项目基于 [MIT License](LICENSE) 开源。欢迎 Star 与贡献代码！
