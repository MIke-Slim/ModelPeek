# ModelPeek - Ultra-Fast 3D/CAD Preview Extension for Windows Explorer

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

**ModelPeek** is a lightweight, blazing-fast, and robust **native 3D/CAD preview and thumbnail extension suite** built for Windows File Explorer.

No need to launch heavy and slow industrial CAD software. ModelPeek lets you inspect 3D & CAD models just like standard images or PDFs: view vivid, high-resolution 3D thumbnails directly in Windows folders, and rotate, zoom, slice, measure, and annotate models in the native Explorer preview pane (`Alt + P`)!

🌐 **Official Website**: [https://modelpeek.mythickoala2.chatgpt.site](https://modelpeek.mythickoala2.chatgpt.site)  
📦 **GitHub Repository**: [https://github.com/MIke-Slim/ModelPeek](https://github.com/MIke-Slim/ModelPeek)

---

## 📥 Download & Installation

Visit the [GitHub Releases page](https://github.com/MIke-Slim/ModelPeek/releases) or the [Official Download Page](https://modelpeek.mythickoala2.chatgpt.site) for the latest release:

### Option 1: Standalone Installer (Recommended)
1. Download **`ModelPeek_v2.2.0_Setup.exe`**;
2. Double-click the installer and click **[Install Now]**;
3. The installer automatically registers COM shell extensions, associates all 24 formats, unblocks network files (MOTW), and creates shortcuts in the Start Menu and Desktop.

### Option 2: Windows Package Manager (Winget)
Run in Windows Terminal or PowerShell:
```powershell
winget install MIke-Slim.ModelPeek
```

### Option 3: Portable Green Edition (No-Install)
1. Download **`ModelPeek_v2.2.0_Portable_x64.zip`**;
2. Extract to any directory of your choice (e.g., `D:\Tools\ModelPeek`);
3. Right-click `install.bat` and select **[Run as administrator]** to activate;
4. To uninstall, run `uninstall.bat` as administrator to cleanly remove all registry entries.

> 💡 **Uninstallation**: To uninstall at any time, go to Windows **Settings -> Installed Apps**, find ModelPeek and click Uninstall, or run `ModelPeekUninstall.exe` in the installation directory. It performs a 100% clean teardown of all registry keys, shell extensions, shortcuts, and cache files with zero residue.

---

## 💡 Quick Start & Usage Tutorial

### 1. View High-Resolution 3D Thumbnails
Once installed, open any directory containing 3D/CAD files (e.g., STEP, STL, OBJ, DXF, 3MF, etc.):
- Switch Windows Explorer view to **[Large icons]** or **[Extra large icons]**;
- Model files will immediately render realistic 3D thumbnails with directional lighting and shading—no more guessing what is inside a file.

### 2. Open the 3D Interactive Preview Pane
- In Windows Explorer, press the keyboard shortcut **`Alt + P`** (or click the top menu **View -> Preview pane**);
- Click any supported 3D file, and the preview pane on the right will smoothly load an interactive 3D viewport.

### 3. Viewport Interaction Guide
| Action / Gesture | Description |
| :--- | :--- |
| **Left-click + Drag** | 360° free orbit camera rotation |
| **Right-click + Drag** | Pan camera view |
| **Mouse Wheel Scroll** | Smooth zoom in and out |
| **Top View Toolbar** | Instant camera snap: [Isometric (ISO)], [Top], [Front], [Right], and [Fit All] |
| **Display Mode Switch** | Toggle between [Shaded + Edges], [Glossy Shaded], and [Wireframe Mesh] |

### 4. Advanced Engineering Tools
- **📐 3D Bounding Box Dimensioning**: Click the **[📐 Dimension]** button to automatically generate 3D bounding box dimension lines and camera-facing millimeter pill labels (`X: ... mm`, `Y: ... mm`, `Z: ... mm`).
- **📏 3D Spatial Distance Measurement**: Click the **[📏 Measure]** button to ray-cast pick any two points on the model surface. Measure true Euclidean distance and ΔX / ΔY / ΔZ axis components in real time.
- **✂️ Dynamic Cross-Section Clipping**: Click the **[✂️ Section]** button and drag the X/Y/Z clipping sliders to inspect internal hollow chambers, threads, and assemblies.
- **🌲 Component Assembly Tree**: Click the tree icon in the top-left corner to inspect individual parts, isolate/highlight components, toggle visibility, and view triangle/vertex counts.
- **🎨 4 Viewport Themes**: Dark Tech, Blueprint, Studio White, and Checkerboard Transparent backgrounds with a single click.
- **🌐 Bilingual UI**: Switch between `🇨🇳 简体中文` and `🇺🇸 English` on the fly via the top-right language selector.

### 5. Control Center (`ModelPeekSettings.exe`)
Double-click **ModelPeek Control Center** on your desktop:
- **Format Toggles**: Enable or disable thumbnails and preview handlers for any of the 24 supported formats individually;
- **🔓 Unblock Network Models**: One-click removal of Windows Mark-of-the-Web (MOTW) security flags on downloaded models to avoid warning prompts;
- **🧹 Clear Thumbnail Cache**: Instantly clear and refresh Windows thumbnail cache to force re-rendering.

> 📖 **Comprehensive User Manual**: For more details, see the accompanying [ModelPeek User Manual (Chinese)](ModelPeek_使用说明书.md).

---

## 📁 Supported Formats (All 24 Industrial & Standard Formats)

| Category | Formats | Technical Highlights |
| :--- | :--- | :--- |
| **Industrial CAD & Assemblies** | `.step`, `.stp`, `.iges`, `.igs`, `.brep`, `.brp`, `.dxf`, `.sldprt`, `.ipt` | Powered by OpenCASCADE B-Rep kernel and native AutoCAD vector parser; native Shell extraction with 3D panoramic cards for SolidWorks (`.sldprt`) & Inventor (`.ipt`) |
| **Standard 3D Meshes** | `.stl`, `.obj`, `.3mf`, `.glb`, `.gltf`, `.fbx` | Supports ASCII/Binary STL, PBR textures/materials, multi-part assembly trees, and FBX NURBS curves |
| **Point Clouds & Scanning** | `.ply`, `.pcd`, `.las` | Supports Stanford PLY true vertex colors, LiDAR LAS point clouds (pure native binary parser), and PCL particle rendering |
| **CAE & Engineering Simulation** | `.inp`, `.vtu` | Zero-dependency native parsers for Abaqus/NASTRAN (`.inp`) and VTK Unstructured Grid (`.vtu`) with automatic boundary skin extraction |
| **BIM & Architecture** | `.rvt` | Autodesk Revit high-resolution native physical render snapshots with interactive 3D preview cards |
| **CNC & Slicer Toolpaths** | `.gcode` | Layer-by-layer 3D spatial visualization of extrusion and travel paths |
| **Multimedia & Legacy 3D** | `.dae` (Collada), `.3ds` (3D Studio) | Seamless preview for classic 3D interchange assets |

---

## 🚀 What's New in v2.2.0: Major Expansion to CAD, CAE, BIM & LiDAR

1. **🌟 Expanded to 24 Mainstream Formats (6 New Heavyweight Formats)**
   - **LiDAR Survey Point Clouds**: Added `.las` (ASPRS binary);
   - **CAE Simulation & FEA Grids**: Added `.inp` (Abaqus / NASTRAN) and `.vtu` (VTK Unstructured Grid);
   - **Commercial Industrial CAD**: Added `.sldprt` (SolidWorks) and `.ipt` (Autodesk Inventor);
   - **BIM Architectural Models**: Added `.rvt` (Autodesk Revit).

2. **⚡ Pure Native C++ Zero-Dependency CAE & Point Cloud Parsers**
   - **LiDAR Point Cloud (`.las`)**: Fast native binary parsing supporting LAS 1.1~1.4, generating dense particle clouds for the offline software rasterizer and interactive 3D viewport.
   - **Simulation Meshes (`.inp`, `.vtu`)**: Built-in support for hex (C3D8), tet (C3D4), and shell (S4/S3) element topologies; innovative shared-face cancellation algorithm extracts watertight boundary skins in milliseconds.

3. **🖼️ Commercial CAD & BIM Native Render Extraction (`.sldprt`, `.ipt`, `.rvt`)**
   - Deep integration with Windows Shell COM `IShellItemImageFactory` instantly extracts high-fidelity native thumbnails with zero delay, presenting interactive 3D panoramic cards in the preview pane.

4. **🧪 Automated Test Suite Expanded to 38 Test Cases**
   - Full end-to-end regression testing across all 24 supported formats with a 100% pass rate.

---

## 🚀 What's New in v2.1.3 & Zero-Dependency Hardening

1. **🌟 100% Clean System Support with Embedded Python Runtime**
   - Core industrial CAD formats (STEP, IGES, BREP, DXF, STL, OBJ, PLY, PCD, GCODE, 3DS) are parsed via **native C++ with millisecond response time**;
   - For modern mesh and scene formats (GLTF, GLB, 3MF, DAE, FBX), ModelPeek bundles an official 11MB embedded green Python runtime. It does not modify system `PATH` nor pollute the registry, delivering 100% out-of-the-box 3D thumbnails even on brand-new, clean virtual machines and offline air-gapped enterprise computers.

2. **🔓 One-Click Windows Security Unblock (Mark-of-the-Web)**
   - The Control Center (`ModelPeekSettings.exe`) and installer feature an automated unblock mechanism, eliminating Windows warnings on files downloaded from browsers or VM shared folders.

3. **🩺 Windows 11 Tabbed Explorer Handle Recovery**
   - Completely resolved `CreateWindowEx err=1400` (invalid window handle) and blank preview pane issues caused by stale handles passed by `prevhost.exe` during tab switching in Windows 11.
   - Built-in 3-tier parent handle redirection and positive dimension fallback (`ResolveValidParent`) guarantees 100% stable viewport loading.

4. **📐 AutoCAD 2023 Association Guard & Anti-Hijack**
   - Eradicated file association hijacking of `.dxf` by third-party suites; preserves AutoCAD 2023 official default double-click association and document icons while maintaining ModelPeek's 3D/2D thumbnails and preview pane.

5. **📁 Brand-New 19-Model High-Precision Test Suite (`sample_models2`)**
   - Includes real-world industrial models spanning all 18 formats: PCD LiDAR point clouds, IGES supersonic nozzle surfaces, BREP/STEP stepped shafts, GCODE helical towers, and DXF mechanical flanges.

6. **📐 3D Bounding Box Dimensioning**
   - One-click engineering bounding box visualization with camera-aligned anti-aliased sprite capsules displaying length, width, and height with depth-aware occlusion.

---

## 🌟 Key Architecture & Reliability Design

- **Native Physics-Grade 3D Thumbnails (`IThumbnailProvider`)**
  - Explorer folder icons render genuine 3D illuminated scenes.
  - Built-in high-performance offline **Software Rasterizer** consumes zero GPU VRAM, renders in milliseconds, and never freezes File Explorer during GPU driver resets.
  - Two-tier local disk cache ensures instant loading on subsequent folder visits.

- **Interactive 3D Preview Handler (`IPreviewHandler`)**
  - Integrated with Microsoft Edge WebView2 runtime for 60 FPS GPU-accelerated smooth rendering.

- **Non-Intrusive File Association**
  - Double-clicking `.step`, `.stp`, or `.dxf` continues to open in your default software (SolidWorks, NX, FreeCAD, AutoCAD).
  - ModelPeek acts strictly as a shell preview provider and never disrupts existing professional CAD workflows.

- **Process-Level Isolation for Maximum Stability**
  - File Explorer loads only an ultra-lightweight 64-bit COM host DLL (`ModelPeekExtension.dll`).
  - All heavy CAD geometry parsing and meshing run in an isolated background Worker process (`ModelPeekWorker.exe`).
  - Even if a model file is corrupted or an assembly is massive, `explorer.exe` will never crash.

---

## 🛠️ Local Build & Development

The repository uses a fully self-contained offline build toolchain:

```powershell
# 1. Compile COM DLL, Worker service, and Settings GUI
powershell -ExecutionPolicy Bypass -File scripts\build.ps1

# 2. Run the automated 38-point feature test suite
powershell -ExecutionPolicy Bypass -File tests\test_all_features.ps1

# 3. Package portable ZIP and modern Setup.exe installer
powershell -ExecutionPolicy Bypass -File scripts\package_release.ps1
```

### Directory Structure
- `src/shell_ext/`: 64-bit native COM shell extension (`IThumbnailProvider`, `IPreviewHandler`, WebView2 bridge, Named Pipe IPC client)
- `src/viewer/`: Self-contained Three.js interactive viewport (offline HTML/CSS/JS, component tree, 3D dimensioning, i18n, theme switching)
- `src/worker/`: Isolated background Worker (native C++ STEP/IGES/BREP/DXF/PCD/LAS/INP/VTU parsers, software rasterizer, Named Pipe daemon)
- `src/settings/`: Native Win32 GUI Control Center (`ModelPeekSettings.exe`)
- `src/installer/`: Modern Fluent UI installer and clean uninstaller (`ModelPeek_Setup.exe`, `ModelPeekUninstall.exe`)
- `manifests/`: Official Microsoft Winget package manifests
- `sample_models/` & `sample_models2/`: Industrial and 3D test datasets for all 24 supported formats
- `tests/`: Automated test suite
- `dist/`: Build outputs and release packages

---

## ☕ Sponsor & Support (Buy Me a Coffee)

If **ModelPeek** has helped you or your team improve productivity and made browsing 3D/CAD assets on Windows a delight, consider buying the author a coffee! ☕  
Every bit of support fuels continuous development, new format additions, and performance optimizations! ❤️

| 🌍 International Support (Buy Me a Coffee) | 🇨🇳 Alipay Support (China) |
| :---: | :---: |
| [![Buy Me A Coffee](https://img.shields.io/badge/Buy%20Me%20A%20Coffee-Donate%20$3-FFDD00?style=for-the-badge&logo=buy-me-a-coffee&logoColor=black)](https://buymeacoffee.com/mikeslim) | Click below to expand QR code |
| [👉 Click to visit Buy Me a Coffee page](https://buymeacoffee.com/mikeslim) | <details><summary><b>📱 Scan with Alipay app</b></summary><br><img src="assets/donate/alipay.jpg" width="220" alt="Alipay QR Code"><br><i>Open Alipay to scan</i></details> |

> Thank you to all creators and engineers who support open-source software! ✨

---

## 📄 License

This project is licensed under the [MIT License](LICENSE). Stars and contributions are warmly welcome!
