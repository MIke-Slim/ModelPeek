# ModelPeek 开发思路

## 1. 产品定位
ModelPeek 是一个 **Windows Explorer 的 3D/CAD 预览插件**。

目标不是替代 SolidWorks、NX、Blender 等专业软件，而是让三维文件像图片、PDF 一样，可以直接在资源管理器中快速查看。

## 2. 核心交互
用户在 Windows Explorer 中选中文件后：

- 左侧/文件区显示模型缩略图
- 右侧 Preview Pane 自动显示可交互的 3D 模型
- 支持旋转、缩放、适配视图和基本信息查看
- 双击文件时，仍然由系统原来的默认软件打开

例如：

- `.step` → 双击仍可用 SolidWorks / NX / FreeCAD 打开
- `.blend` → 双击仍由 Blender 打开
- `.prt` → 双击仍由 NX 打开

ModelPeek **不主动抢占默认文件关联**。

## 3. 第一版支持格式
优先支持：

- STEP / STP
- STL
- OBJ
- FBX
- GLB / GLTF
- 3MF

后续再扩展：

- BLEND
- SLDPRT / SLDASM
- NX PRT
- CATPart / CATProduct
- Creo
- Inventor

## 4. 核心架构
整体流程：

```text
3D/CAD 文件
    ↓
格式解析器 / Importer
    ↓
统一 Scene 数据层
    ↓
3D Renderer
    ↓
Windows Explorer Preview Pane
```

不同格式通过插件化 Importer 接入，避免每增加一种格式都重新修改整个程序。

## 5. Mesh 与 B-Rep 分开处理
CAD 和 Mesh 文件不能全部当成同一种数据。

### CAD 文件
例如 STEP、IGES：

```text
STEP
 ↓
Open CASCADE
 ↓
B-Rep
 ↓
生成 Preview Mesh 用于显示
```

保留 B-Rep，以便未来支持：

- 精确测量
- 面、边识别
- 半径、直径
- 体积和面积
- 装配结构
- 几何特征识别

### Mesh 文件
例如 STL、OBJ：

```text
STL / OBJ
 ↓
Triangle Mesh
 ↓
直接渲染
```

## 6. Windows Explorer 集成
第一版重点实现：

### Thumbnail Provider
让 STEP、STL 等文件在文件夹中直接显示真实模型缩略图。

### Preview Handler
用户选中文件后，在 Windows Explorer 右侧预览窗格中显示交互式 3D 模型。

第一版 **不以 Space 快速预览为核心功能**。

## 7. 稳定性设计
不能让 `explorer.exe` 直接解析大型 3D/CAD 文件。

推荐结构：

```text
Windows Explorer
      ↓
ModelPeek Shell Extension
      ↓ IPC
ModelPeek Worker
      ↓
模型解析 / 生成预览
```

真正的模型解析放到独立 Worker 进程。

即使某个模型损坏、解析失败或 Worker 崩溃，也尽量不影响 Windows Explorer。

## 8. 缓存
模型第一次解析后生成缓存：

- 缩略图
- Preview Mesh
- 基本模型信息
- 文件状态信息

再次选中同一个文件时直接读取缓存，提高预览速度。

目标是让用户感觉接近“图片预览”的体验。

## 9. 推荐技术栈
初步考虑：

- C++20
- Qt 6
- libf3d / VTK
- Open CASCADE
- Assimp
- Win32 / COM
- IThumbnailProvider
- IPreviewHandler
- SQLite
- Named Pipe / Shared Memory

## 10. 后续发展
第一版完成后再逐步增加：

1. Everything 搜索集成
2. `.blend` 预览
3. SolidWorks / NX / CATIA 等原生 CAD 文件
4. 装配树
5. 测量
6. 剖切
7. 爆炸图
8. 质量属性
9. 格式转换
10. Mesh → 几何识别 → B-Rep → 参数化 CAD

## 11. 第一版 MVP
第一版只聚焦：

> **STEP / STL / OBJ / FBX / GLB / 3MF + Explorer 缩略图 + 右侧交互式 3D 预览**

双击仍然进入原来的专业软件。

一句话总结：

> **ModelPeek = Windows Explorer 的 3D/CAD 预览插件。**
