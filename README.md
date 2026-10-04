# ModelPeek - Windows 资源管理器 3D/CAD 快速预览插件

ModelPeek 是一个轻量、高效、极其稳定的 **Windows 资源管理器 3D/CAD 预览插件**。
无需启动庞大的专业工业软件，让 3D 与 CAD 模型文件像图片、PDF 一样，可以在 Windows 文件夹中直接查看真实立体缩略图，并在右侧预览窗格中进行全交互式的 3D 旋转、缩放与尺寸检查。

---

## 🌟 核心特性与架构设计

1. **原生物理级 3D 缩略图 (`IThumbnailProvider`)**
   - 文件夹图标直接显示模型的真实三维光照渲染图（支持 STEP、STL、OBJ、3MF、GLB 等）。
   - 内置高性能离线软件光栅化引擎（Software Rasterizer），不占用 GPU 显存，毫秒级快速生成，永不因显卡驱动重置拖死资源管理器。
   - 拥有本地二级缓存机制（`%LOCALAPPDATA%\ModelPeek\cache\thumbnails\`），再次打开文件夹瞬间秒开。

2. **右侧 3D 交互式预览窗格 (`IPreviewHandler`)**
   - 选中模型文件后，右侧预览窗格（Alt+P）自动呈现 3D 视口。
   - **交互操作**：
     - **鼠标左键拖拽**：360° 轨道旋转视角
     - **鼠标右键拖拽**：平移视角
     - **鼠标滚轮**：平滑缩放
     - **顶部快捷工具栏**：一键切换【等轴测】、【顶视】、【前视】、【右视】、【自适应适配】视角。
     - **渲染模式**：支持【实体+边线特征】、【纯高光实体】、【线框网格】三种显示风格。
     - **📏 3D 空间三维测距工具**：射线拾取表面任意两点，实时精确测量真实欧几里得距离及 ΔX / ΔY / ΔZ 轴向分量（单位：mm）。
     - **✂️ 动态剖切截面工具**：支持沿 X / Y / Z 轴向实时剖切观察内部中空腔体、螺纹和装配结构，支持深度滑块调节与反向剖切。
   - **HUD 模型信息面板**：实时计算并显示外形包围盒尺寸（长 × 宽 × 高 mm）、三角面数、顶点数量。

3. **双击不抢占默认软件关联**
   - 双击 `.step` / `.stp` 仍由您原本的 SolidWorks / NX / FreeCAD 打开；
   - 双击 `.blend` 仍由 Blender 打开。
   - 纯粹的 Preview 扩展，绝不破坏用户现有的工作流关联。

4. **进程级隔离稳定性设计**
   - 资源管理器仅加载极轻量的 64 位 COM 宿主 DLL；
   - CAD 几何解析与网格化均由独立的后台 Worker 进程（`ModelPeekWorker.exe`）执行；
   - 哪怕遇到损坏模型或复杂大装配，也不会影响 `explorer.exe` 的正常运行。

---

## 📁 支持格式（第一版 MVP）

- **CAD 工业格式**：`.step`、`.stp`（OpenCASCADE B-Rep 工业级几何内核）
- **通用 3D 网格**：`.stl`（ASCII 与 Binary）、`.obj`、`.3mf`、`.glb`、`.gltf`、`.fbx`

---

## 🚀 安装与使用方法（全 Windows 电脑通用）

发布目录位于工程下的 `dist/`，完全便携独立。

### 1. 快速安装
1. 将 `dist/` 文件夹放置在您希望存放的位置（例如 `C:\Program Files\ModelPeek` 或任意目录）；
2. 鼠标右键点击 `dist\install.bat`，选择 **【以管理员身份运行】**；
3. 脚本会自动完成 64 位 COM 组件注册与 3D 文件类型关联，并在完成后提示成功。

### 2. 预览体验
1. 打开任意包含 `.step` 或 `.stl` 模型的文件夹，查看图标缩略图；
2. 在资源管理器上方点击【查看】-【预览窗格】（快捷键 `Alt + P`）；
3. 鼠标单击任意 3D 模型文件，右侧立即加载交互式 3D 视口！

### 3. 一键卸载
1. 鼠标右键点击 `dist\uninstall.bat`，选择 **【以管理员身份运行】**；
2. 系统将彻底注销所有 COM 扩展与注册表关联，干净利落，无残留。

---

## 🛠️ 工程开发与自动化编译

工程提供便携绿色的免安装编译链路：

- **自动化一键编译**：
  ```powershell
  powershell -ExecutionPolicy Bypass -File scripts\build.ps1
  ```
- **核心源码目录**：
  - `src/shell_ext/`：64 位 COM Shell Extension 原生代码（`IThumbnailProvider`、`IPreviewHandler`、`WebView2` 嵌入桥接）
  - `src/viewer/`：Three.js 交互式 3D 视口前端（完全离线自包含，无需网络）
  - `src/worker/`：后台 Worker 进程源码（CAD 解析、网格化转换、软件光栅化渲染器）
  - `sample_models/`：测试验证模型
  - `dist/`：最终交付物发布目录
