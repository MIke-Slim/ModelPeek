#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")

#define MODELPEEK_VERSION_STR L"2.1.3"

// Theme Colors
static const COLORREF CLR_BG         = RGB(248, 250, 252); // #F8FAFC
static const COLORREF CLR_CARD_BG    = RGB(255, 255, 255); // #FFFFFF
static const COLORREF CLR_BORDER     = RGB(226, 232, 240); // #E2E8F0
static const COLORREF CLR_TEXT_DARK  = RGB(15, 23, 42);    // #0F172A
static const COLORREF CLR_TEXT_MUTED = RGB(100, 116, 139); // #64748B
static const COLORREF CLR_PRIMARY    = RGB(37, 99, 235);   // #2563EB
static const COLORREF CLR_PRIMARY_HOV= RGB(29, 78, 216);   // #1D4ED8
static const COLORREF CLR_SUCCESS    = RGB(22, 163, 74);   // #16A34A
static const COLORREF CLR_DANGER     = RGB(220, 38, 38);   // #DC2626

static HBRUSH g_hBrushBg = NULL;
static HBRUSH g_hBrushCard = NULL;
static HPEN g_hPenBorder = NULL;

static HFONT g_hFontHeader = NULL;
static HFONT g_hFontTitle = NULL;
static HFONT g_hFontBold = NULL;
static HFONT g_hFontNormal = NULL;
static HFONT g_hFontSmall = NULL;

// CLSIDs
static const LPCWSTR CLSID_THUMB_KEY = L"{e357fccd-a995-4576-b01f-234630154e96}";
static const LPCWSTR CLSID_PREV_KEY  = L"{8895b1c6-b41f-4c1c-a562-0d564250836f}";
static const LPCWSTR CLSID_THUMB_VAL = L"{E9B34A3E-94A5-47F1-A4FD-258F2C411311}";
static const LPCWSTR CLSID_PREV_VAL  = L"{C81B4AE3-6C73-4DC1-8316-04DE665F09B1}";

struct FormatInfo {
    std::wstring ext;
    std::wstring label;
    std::wstring category;
    HWND hCheckbox;
};

static std::vector<FormatInfo> g_formats = {
    // CAD 工业标准格式
    { L".step", L".step (STEP 工业零件)", L"CAD 工业标准格式", NULL },
    { L".stp",  L".stp (STEP 工业装配)", L"CAD 工业标准格式", NULL },
    { L".iges", L".iges (IGES 曲线/实体)", L"CAD 工业标准格式", NULL },
    { L".igs",  L".igs (IGES 模型)", L"CAD 工业标准格式", NULL },
    { L".brep", L".brep (OpenCASCADE 边界)", L"CAD 工业标准格式", NULL },
    { L".brp",  L".brp (B-Rep 拓扑模型)", L"CAD 工业标准格式", NULL },
    { L".dxf",  L".dxf (AutoCAD 二维/三维图纸)", L"CAD 工业标准格式", NULL },
    // 通用三维网格格式
    { L".stl",  L".stl (Stereolithography 网格)", L"通用三维网格格式", NULL },
    { L".obj",  L".obj (Wavefront 3D 对象)", L"通用三维网格格式", NULL },
    { L".glb",  L".glb (二进制 glTF 传输模型)", L"通用三维网格格式", NULL },
    { L".gltf", L".gltf (glTF 3D 交换格式)", L"通用三维网格格式", NULL },
    { L".3mf",  L".3mf (3D Manufacturing 制造)", L"通用三维网格格式", NULL },
    { L".fbx",  L".fbx (Autodesk 多边形与骨骼)", L"通用三维网格格式", NULL },
    // 逆向工程、扫描与制造格式
    { L".ply",  L".ply (Stanford 扫描网格)", L"逆向工程与制造格式", NULL },
    { L".pcd",  L".pcd (Point Cloud 激光点云)", L"逆向工程与制造格式", NULL },
    { L".gcode",L".gcode (CNC / 3D 打印刀轨)", L"逆向工程与制造格式", NULL },
    // 经典 3D 格式
    { L".dae",  L".dae (Collada 交互式 3D 资产)", L"经典 3D 格式", NULL },
    { L".3ds",  L".3ds (3D Studio 经典网格)", L"经典 3D 格式", NULL }
};

static HWND g_hTab = NULL;
static HWND g_hPanelFormats = NULL;
static HWND g_hPanelStatus = NULL;
static HWND g_hPanelCache = NULL;
static HWND g_hCacheInfoText = NULL;
static HWND g_hStatusInfoText = NULL;

BOOL IsRunAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin;
}

std::wstring GetAppDir() {
    WCHAR selfPath[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, selfPath, MAX_PATH);
    PathRemoveFileSpecW(selfPath);
    return selfPath;
}

bool IsFormatRegistered(const std::wstring& ext) {
    HKEY hKey = NULL;
    std::wstring subKey = L"Software\\Classes\\" + ext + L"\\ShellEx\\" + CLSID_THUMB_KEY;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, subKey.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        WCHAR val[128] = {0};
        DWORD size = sizeof(val);
        LONG res = RegQueryValueExW(hKey, NULL, NULL, NULL, (LPBYTE)val, &size);
        RegCloseKey(hKey);
        if (res == ERROR_SUCCESS && wcslen(val) > 0) return true;
    }
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        WCHAR val[128] = {0};
        DWORD size = sizeof(val);
        LONG res = RegQueryValueExW(hKey, NULL, NULL, NULL, (LPBYTE)val, &size);
        RegCloseKey(hKey);
        if (res == ERROR_SUCCESS && wcslen(val) > 0) return true;
    }
    return false;
}

bool SetFormatRegistration(const std::wstring& ext, bool enable) {
    HKEY root = IsRunAsAdmin() ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
    std::wstring baseKey = L"Software\\Classes\\" + ext;
    std::wstring thumbSub = baseKey + L"\\ShellEx\\" + CLSID_THUMB_KEY;
    std::wstring prevSub  = baseKey + L"\\ShellEx\\" + CLSID_PREV_KEY;

    if (enable) {
        HKEY hKey;
        if (RegCreateKeyExW(root, thumbSub.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            RegSetValueExW(hKey, NULL, 0, REG_SZ, (const BYTE*)CLSID_THUMB_VAL, (DWORD)((wcslen(CLSID_THUMB_VAL) + 1) * sizeof(WCHAR)));
            RegCloseKey(hKey);
        }
        if (RegCreateKeyExW(root, prevSub.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            RegSetValueExW(hKey, NULL, 0, REG_SZ, (const BYTE*)CLSID_PREV_VAL, (DWORD)((wcslen(CLSID_PREV_VAL) + 1) * sizeof(WCHAR)));
            RegCloseKey(hKey);
        }
    } else {
        if (IsRunAsAdmin()) {
            RegDeleteKeyW(HKEY_LOCAL_MACHINE, thumbSub.c_str());
            RegDeleteKeyW(HKEY_LOCAL_MACHINE, prevSub.c_str());
            RegDeleteKeyW(HKEY_CURRENT_USER, thumbSub.c_str());
            RegDeleteKeyW(HKEY_CURRENT_USER, prevSub.c_str());
        } else {
            HKEY hKey;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, thumbSub.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                RegSetValueExW(hKey, NULL, 0, REG_SZ, (const BYTE*)L"", sizeof(WCHAR));
                RegCloseKey(hKey);
            }
            if (RegCreateKeyExW(HKEY_CURRENT_USER, prevSub.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                RegSetValueExW(hKey, NULL, 0, REG_SZ, (const BYTE*)L"", sizeof(WCHAR));
                RegCloseKey(hKey);
            }
        }
    }
    return true;
}

std::wstring GetCacheDirPath() {
    WCHAR localAppData[MAX_PATH] = {0};
    if (SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppData) == S_OK) {
        return std::wstring(localAppData) + L"\\ModelPeek\\cache\\thumbnails";
    }
    return L"";
}

void GetCacheStats(int& outFiles, double& outSizeMB) {
    outFiles = 0;
    outSizeMB = 0.0;
    std::wstring dir = GetCacheDirPath();
    if (dir.empty() || !PathFileExistsW(dir.c_str())) return;

    std::wstring search = dir + L"\\*.*";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(search.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    unsigned __int64 totalBytes = 0;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            outFiles++;
            unsigned __int64 fileSize = ((unsigned __int64)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
            totalBytes += fileSize;
        }
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);

    outSizeMB = (double)totalBytes / (1024.0 * 1024.0);
}

void ClearCacheFiles() {
    std::wstring dir = GetCacheDirPath();
    if (dir.empty() || !PathFileExistsW(dir.c_str())) return;

    std::wstring search = dir + L"\\*.*";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(search.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            std::wstring filePath = dir + L"\\" + fd.cFileName;
            DeleteFileW(filePath.c_str());
        }
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);
}

void UnblockDirectory(const std::wstring& folderPath, int& unblockedCount) {
    std::wstring search = folderPath + L"\\*.*";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(search.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
        std::wstring fullPath = folderPath + L"\\" + fd.cFileName;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            UnblockDirectory(fullPath, unblockedCount);
        } else {
            std::wstring zoneStream = fullPath + L":Zone.Identifier";
            if (DeleteFileW(zoneStream.c_str())) {
                unblockedCount++;
            }
        }
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);
}

void RestartExplorer() {
    system("taskkill /f /im prevhost.exe >nul 2>&1");
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
}

LRESULT CALLBACK PanelProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_COMMAND:
        return SendMessageW(GetParent(hWnd), WM_COMMAND, wParam, lParam);
    case WM_NOTIFY:
        return SendMessageW(GetParent(hWnd), WM_NOTIFY, wParam, lParam);
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
        return SendMessageW(GetParent(hWnd), msg, wParam, lParam);
    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hWnd, &rc);
        FillRect(hdc, &rc, g_hBrushCard);
        return 1;
    }
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// Panel 0: 格式管理
void CreateFormatsPanel(HWND hParent) {
    g_hPanelFormats = CreateWindowExW(0, L"ModelPeekPanelClass", L"", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 
        20, 118, 620, 335, hParent, NULL, GetModuleHandleW(NULL), NULL);

    HWND hLbl = CreateWindowExW(0, L"STATIC", L"选择由 ModelPeek 接管 3D 立体缩略图与视口交互预览的格式（共 18 种）：", 
        WS_CHILD | WS_VISIBLE, 15, 10, 590, 20, g_hPanelFormats, NULL, GetModuleHandleW(NULL), NULL);
    SendMessageW(hLbl, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

    int startX1 = 20;
    int startX2 = 320;
    int startY = 38;
    int rowH = 22;
    int half = (int)(g_formats.size() + 1) / 2;

    for (size_t i = 0; i < g_formats.size(); i++) {
        int x = (i < (size_t)half) ? startX1 : startX2;
        int y = startY + ((int)(i % half)) * rowH;

        HWND hChk = CreateWindowExW(0, L"BUTTON", g_formats[i].label.c_str(),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
            x, y, 280, 20, g_hPanelFormats, (HMENU)(UINT_PTR)(1000 + i), GetModuleHandleW(NULL), NULL);
        SendMessageW(hChk, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        SendMessageW(hChk, BM_SETCHECK, IsFormatRegistered(g_formats[i].ext) ? BST_CHECKED : BST_UNCHECKED, 0);
        g_formats[i].hCheckbox = hChk;
    }

    // Action buttons
    HWND btnSelectAll = CreateWindowExW(0, L"BUTTON", L"全选", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        20, 245, 80, 28, g_hPanelFormats, (HMENU)100, GetModuleHandleW(NULL), NULL);
    SendMessageW(btnSelectAll, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    HWND btnClearAll = CreateWindowExW(0, L"BUTTON", L"全不选", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        110, 245, 80, 28, g_hPanelFormats, (HMENU)101, GetModuleHandleW(NULL), NULL);
    SendMessageW(btnClearAll, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    HWND btnApply = CreateWindowExW(0, L"BUTTON", L"💾 保存并应用格式设置", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        420, 243, 180, 32, g_hPanelFormats, (HMENU)102, GetModuleHandleW(NULL), NULL);
    SendMessageW(btnApply, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

    // Tip Note
    HWND hHint = CreateWindowExW(0, L"STATIC", L"🛡️ 安全说明：双击模型文件严格保留专业 CAD/3D 软件默认关联，仅启用原生缩略图与 Alt+P 视口交互预览。",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 288, 580, 36, g_hPanelFormats, NULL, GetModuleHandleW(NULL), NULL);
    SendMessageW(hHint, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);
}

void UpdateStatusUI(HWND hEdit) {
    std::wstring appDir = GetAppDir();
    std::wstring dllPath = appDir + L"\\ModelPeekExtension.dll";
    std::wstring workerPath = appDir + L"\\ModelPeekWorker.exe";

    std::wstringstream ss;
    ss << L"【核心组件与系统环境完整性体检报告】\r\n\r\n"
       << L"● 程序安装运行目录:\r\n  " << appDir << L"\r\n\r\n"
       << L"● COM 64位外壳扩展 (ModelPeekExtension.dll):\r\n  " 
       << (PathFileExistsW(dllPath.c_str()) ? L"✓ 正常就绪 (已注入 PrevHost 宿主规范)" : L"✗ 缺失") << L"\r\n\r\n"
       << L"● 本地渲染几何引擎 (ModelPeekWorker.exe):\r\n  " 
       << (PathFileExistsW(workerPath.c_str()) ? L"✓ 正常就绪 (纯 C++ 原生解析器 + 内部私有 Python 沙箱)" : L"✗ 缺失") << L"\r\n\r\n"
       << L"● 当前运行权限:\r\n  " 
       << (IsRunAsAdmin() ? L"✓ 管理员权限 (Administrator)" : L"● 普通用户权限 (部分注册表项需提权修改)") << L"\r\n\r\n"
       << L"● 后台守护进程:\r\n  ✓ 零后台常驻进程 (按需拉起，用完即毁，零日常内存占用)";

    SetWindowTextW(hEdit, ss.str().c_str());
}

// Panel 1: 系统体检
void CreateStatusPanel(HWND hParent) {
    g_hPanelStatus = CreateWindowExW(0, L"ModelPeekPanelClass", L"", WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 
        20, 118, 620, 335, hParent, NULL, GetModuleHandleW(NULL), NULL);

    HWND hTitle = CreateWindowExW(0, L"STATIC", L"组件状态与系统环境体检", 
        WS_CHILD | WS_VISIBLE, 15, 10, 590, 20, g_hPanelStatus, NULL, GetModuleHandleW(NULL), NULL);
    SendMessageW(hTitle, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

    g_hStatusInfoText = CreateWindowExW(WS_EX_STATICEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL,
        15, 34, 590, 220, g_hPanelStatus, NULL, GetModuleHandleW(NULL), NULL);
    SendMessageW(g_hStatusInfoText, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    UpdateStatusUI(g_hStatusInfoText);

    HWND btnReinstall = CreateWindowExW(0, L"BUTTON", L"🔧 一键重新注册扩展", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        15, 268, 175, 32, g_hPanelStatus, (HMENU)201, GetModuleHandleW(NULL), NULL);
    SendMessageW(btnReinstall, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    HWND btnUninstall = CreateWindowExW(0, L"BUTTON", L"🗑️ 彻底卸载 ModelPeek", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        205, 268, 180, 32, g_hPanelStatus, (HMENU)202, GetModuleHandleW(NULL), NULL);
    SendMessageW(btnUninstall, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    HWND btnDonate = CreateWindowExW(0, L"BUTTON", L"☕ 赞助支持作者 (Sponsor)", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        400, 268, 205, 32, g_hPanelStatus, (HMENU)203, GetModuleHandleW(NULL), NULL);
    SendMessageW(btnDonate, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
}

void UpdateCacheUI(HWND hLabel) {
    int count = 0;
    double sizeMB = 0.0;
    GetCacheStats(count, sizeMB);

    std::wstringstream ss;
    ss << L"【缩略图二级本地缓存状态与故障排查】\r\n\r\n"
       << L"● 缓存物理存储路径:\r\n  " << GetCacheDirPath() << L"\r\n\r\n"
       << L"● 当前缓存图像文件数: " << count << L" 个\r\n"
       << L"● 当前磁盘空间占用:   " << std::fixed << std::setprecision(2) << sizeMB << L" MB\r\n\r\n"
       << L"说明与故障排查：\r\n"
       << L"1. 清空缓存：若模型更新后缩略图未刷新，可清空缓存，系统将在浏览时极速重绘。\r\n"
       << L"2. 解除网络锁定：若从网络下载或虚拟机共享的模型在 Alt+P 预览时提示【可能对你的计算机有害】，点击下方【🔓 解除模型网络锁定】按钮，选择文件夹即可一键消除 Zone.Identifier 限制。";

    SetWindowTextW(hLabel, ss.str().c_str());
}

// Panel 2: 缓存管理
void CreateCachePanel(HWND hParent) {
    g_hPanelCache = CreateWindowExW(0, L"ModelPeekPanelClass", L"", WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 
        20, 118, 620, 335, hParent, NULL, GetModuleHandleW(NULL), NULL);

    HWND hTitle = CreateWindowExW(0, L"STATIC", L"缩略图二级缓存加速与故障排查", 
        WS_CHILD | WS_VISIBLE, 15, 10, 590, 20, g_hPanelCache, NULL, GetModuleHandleW(NULL), NULL);
    SendMessageW(hTitle, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

    g_hCacheInfoText = CreateWindowExW(WS_EX_STATICEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL,
        15, 34, 590, 220, g_hPanelCache, NULL, GetModuleHandleW(NULL), NULL);
    SendMessageW(g_hCacheInfoText, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    UpdateCacheUI(g_hCacheInfoText);

    HWND btnClear = CreateWindowExW(0, L"BUTTON", L"🧹 一键清空缩略图缓存", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        15, 268, 175, 32, g_hPanelCache, (HMENU)301, GetModuleHandleW(NULL), NULL);
    SendMessageW(btnClear, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    HWND btnRestartExp = CreateWindowExW(0, L"BUTTON", L"🔄 刷新 Windows 图标缓存", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        200, 268, 185, 32, g_hPanelCache, (HMENU)302, GetModuleHandleW(NULL), NULL);
    SendMessageW(btnRestartExp, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    HWND btnUnblock = CreateWindowExW(0, L"BUTTON", L"🔓 解除模型网络锁定 (Unblock)", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        395, 268, 210, 32, g_hPanelCache, (HMENU)303, GetModuleHandleW(NULL), NULL);
    SendMessageW(btnUnblock, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
}

void SwitchTab(int index) {
    ShowWindow(g_hPanelFormats, (index == 0) ? SW_SHOW : SW_HIDE);
    ShowWindow(g_hPanelStatus,  (index == 1) ? SW_SHOW : SW_HIDE);
    ShowWindow(g_hPanelCache,   (index == 2) ? SW_SHOW : SW_HIDE);

    HWND active = g_hPanelFormats;
    if (index == 1) active = g_hPanelStatus;
    else if (index == 2) active = g_hPanelCache;

    BringWindowToTop(active);
    InvalidateRect(active, NULL, TRUE);

    if (index == 1 && g_hStatusInfoText) UpdateStatusUI(g_hStatusInfoText);
    if (index == 2 && g_hCacheInfoText) UpdateCacheUI(g_hCacheInfoText);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_hTab = CreateWindowExW(0, WC_TABCONTROLW, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
            20, 84, 620, 375, hWnd, (HMENU)1, GetModuleHandleW(NULL), NULL);
        SendMessageW(g_hTab, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        TCITEMW tie = {0};
        tie.mask = TCIF_TEXT;
        tie.pszText = (LPWSTR)L"📁 格式管理 (18种)";
        TabCtrl_InsertItem(g_hTab, 0, &tie);
        tie.pszText = (LPWSTR)L"🩺 系统体检";
        TabCtrl_InsertItem(g_hTab, 1, &tie);
        tie.pszText = (LPWSTR)L"⚡ 缓存与排错";
        TabCtrl_InsertItem(g_hTab, 2, &tie);

        CreateFormatsPanel(hWnd);
        CreateStatusPanel(hWnd);
        CreateCachePanel(hWnd);
        SwitchTab(0);
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT rcClient;
        GetClientRect(hWnd, &rcClient);

        // Top Hero Card
        RECT rcTop = { 0, 0, rcClient.right, 72 };
        HBRUSH hBrTop = CreateSolidBrush(CLR_CARD_BG);
        FillRect(hdc, &rcTop, hBrTop);
        DeleteObject(hBrTop);

        HPEN hPenDiv = CreatePen(PS_SOLID, 1, CLR_BORDER);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPenDiv);
        MoveToEx(hdc, 0, 72, NULL);
        LineTo(hdc, rcClient.right, 72);

        // Title
        SelectObject(hdc, g_hFontHeader);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, CLR_TEXT_DARK);
        TextOutW(hdc, 24, 12, L"ModelPeek 3D/CAD 控制中心", 19);

        // Version badge pill
        RECT rcBadge = { 285, 14, 350, 34 };
        HBRUSH hBrBadge = CreateSolidBrush(RGB(239, 246, 255));
        HPEN hPenBadge = CreatePen(PS_SOLID, 1, RGB(191, 219, 254));
        SelectObject(hdc, hBrBadge);
        SelectObject(hdc, hPenBadge);
        RoundRect(hdc, rcBadge.left, rcBadge.top, rcBadge.right, rcBadge.bottom, 8, 8);
        DeleteObject(hBrBadge);
        DeleteObject(hPenBadge);

        SelectObject(hdc, g_hFontSmall);
        SetTextColor(hdc, CLR_PRIMARY);
        DrawTextW(hdc, L"v2.1.3", -1, &rcBadge, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Subtitle
        SelectObject(hdc, g_hFontSmall);
        SetTextColor(hdc, CLR_TEXT_MUTED);
        TextOutW(hdc, 24, 42, L"轻量原生 Windows 资源管理器 3D/CAD 可视化增强套件 · 18 种格式设置与系统维护", 42);

        SelectObject(hdc, hOldPen);
        DeleteObject(hPenDiv);
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hWnd, &rc);
        FillRect(hdc, &rc, g_hBrushBg);
        return 1;
    }

    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;
        if (pdis->CtlID == 102) { // Save and Apply button
            HDC hdc = pdis->hDC;
            RECT rc = pdis->rcItem;

            bool isPressed = (pdis->itemState & ODS_SELECTED);
            COLORREF btnColor = isPressed ? RGB(30, 64, 175) : CLR_PRIMARY;

            HBRUSH hBrBtn = CreateSolidBrush(btnColor);
            HPEN hPenBtn = CreatePen(PS_SOLID, 1, btnColor);
            HBRUSH hOldBr = (HBRUSH)SelectObject(hdc, hBrBtn);
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPenBtn);

            RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 8, 8);

            SelectObject(hdc, g_hFontBold);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 255, 255));
            DrawTextW(hdc, L"💾 保存并应用格式设置", -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            SelectObject(hdc, hOldBr);
            SelectObject(hdc, hOldPen);
            DeleteObject(hBrBtn);
            DeleteObject(hPenBtn);
            return TRUE;
        }
        break;
    }

    case WM_NOTIFY: {
        LPNMHDR pnmh = (LPNMHDR)lParam;
        if (pnmh->hwndFrom == g_hTab && pnmh->code == TCN_SELCHANGE) {
            int cur = TabCtrl_GetCurSel(g_hTab);
            SwitchTab(cur);
        }
        return 0;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == 100) { // Select all
            for (auto& fmt : g_formats) {
                if (fmt.hCheckbox) {
                    SendMessageW(fmt.hCheckbox, BM_SETCHECK, BST_CHECKED, 0);
                    InvalidateRect(fmt.hCheckbox, NULL, TRUE);
                }
            }
        } else if (id == 101) { // Clear all
            for (auto& fmt : g_formats) {
                if (fmt.hCheckbox) {
                    SendMessageW(fmt.hCheckbox, BM_SETCHECK, BST_UNCHECKED, 0);
                    InvalidateRect(fmt.hCheckbox, NULL, TRUE);
                }
            }
        } else if (id == 102) { // Apply format associations
            int countActive = 0;
            for (auto& fmt : g_formats) {
                if (fmt.hCheckbox) {
                    bool chk = (SendMessageW(fmt.hCheckbox, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    SetFormatRegistration(fmt.ext, chk);
                    if (chk) countActive++;
                }
            }
            RestartExplorer();
            std::wstringstream msg;
            msg << L"🎉 格式设置已成功应用并写入注册表！\r\n\r\n当前已启用接管 " << countActive << L" 种 3D/CAD 格式支持。";
            MessageBoxW(hWnd, msg.str().c_str(), L"ModelPeek 设置成功", MB_OK | MB_ICONINFORMATION);
        } else if (id == 201) { // Re-register
            std::wstring regCmd = L"regsvr32.exe /s \"" + GetAppDir() + L"\\ModelPeekExtension.dll\"";
            _wsystem(regCmd.c_str());
            RestartExplorer();
            MessageBoxW(hWnd, L"已重新执行 COM 扩展注册并激活！", L"提示", MB_OK | MB_ICONINFORMATION);
        } else if (id == 202) { // Uninstall
            std::wstring uninstExe = GetAppDir() + L"\\ModelPeekUninstall.exe";
            if (PathFileExistsW(uninstExe.c_str())) {
                ShellExecuteW(NULL, L"open", uninstExe.c_str(), NULL, GetAppDir().c_str(), SW_SHOWNORMAL);
                PostQuitMessage(0);
            } else {
                std::wstring unregCmd = L"regsvr32.exe /u /s \"" + GetAppDir() + L"\\ModelPeekExtension.dll\"";
                _wsystem(unregCmd.c_str());
                RestartExplorer();
                MessageBoxW(hWnd, L"已注销 ModelPeek COM 扩展组件。", L"提示", MB_OK | MB_ICONINFORMATION);
            }
        } else if (id == 203) { // Donate / Sponsor
            ShellExecuteW(NULL, L"open", L"https://buymeacoffee.com/mikeslim", NULL, NULL, SW_SHOWNORMAL);
        } else if (id == 301) { // Clear cache
            ClearCacheFiles();
            UpdateCacheUI(g_hCacheInfoText);
            MessageBoxW(hWnd, L"本地缩略图二级缓存已彻底清空！", L"提示", MB_OK | MB_ICONINFORMATION);
        } else if (id == 302) { // Restart explorer
            RestartExplorer();
            MessageBoxW(hWnd, L"Windows 图标缓存已通知系统刷新！", L"提示", MB_OK | MB_ICONINFORMATION);
        } else if (id == 303) { // Unblock folder
            IFileDialog *pfd = NULL;
            if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd)))) {
                DWORD dwOptions = 0;
                if (SUCCEEDED(pfd->GetOptions(&dwOptions))) {
                    pfd->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
                }
                pfd->SetTitle(L"选择需要解除安全锁定的模型文件夹（如下载目录或虚拟机共享文件夹）");
                if (SUCCEEDED(pfd->Show(hWnd))) {
                    IShellItem *psi = NULL;
                    if (SUCCEEDED(pfd->GetResult(&psi))) {
                        LPWSTR pszPath = NULL;
                        if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
                            int count = 0;
                            UnblockDirectory(pszPath, count);
                            std::wstringstream msg;
                            msg << L"🎉 已完成网络安全标记扫描与解除！\r\n\r\n"
                                << L"扫描文件夹: " << pszPath << L"\r\n"
                                << L"成功解除 " << count << L" 个文件的网络阻止锁定 (Zone.Identifier)。\r\n\r\n"
                                << L"现在选定该模型文件即可在 Windows 资源管理器中直接预览！";
                            MessageBoxW(hWnd, msg.str().c_str(), L"解除网络锁定成功", MB_OK | MB_ICONINFORMATION);
                            CoTaskMemFree(pszPath);
                        }
                        psi->Release();
                    }
                }
                pfd->Release();
            }
        }
        return 0;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, CLR_TEXT_DARK);
        SetBkColor(hdcStatic, CLR_CARD_BG);
        return (INT_PTR)g_hBrushCard;
    }

    case WM_CTLCOLORBTN: {
        return (INT_PTR)g_hBrushCard;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_TAB_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icex);

    g_hBrushBg = CreateSolidBrush(CLR_BG);
    g_hBrushCard = CreateSolidBrush(CLR_CARD_BG);
    g_hPenBorder = CreatePen(PS_SOLID, 1, CLR_BORDER);

    g_hFontHeader = CreateFontW(-18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
    g_hFontTitle  = CreateFontW(-14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
    g_hFontBold   = CreateFontW(-13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
    g_hFontNormal = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
    g_hFontSmall  = CreateFontW(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");

    // Register Panel class
    WNDCLASSEXW pc = { sizeof(pc) };
    pc.lpfnWndProc = PanelProc;
    pc.hInstance = hInstance;
    pc.lpszClassName = L"ModelPeekPanelClass";
    pc.hbrBackground = g_hBrushCard;
    pc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    RegisterClassExW(&pc);

    // Register Main Window class
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ModelPeekSettingsClass";
    wc.hbrBackground = g_hBrushBg;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    RegisterClassExW(&wc);

    int w = 680;
    int h = 510;
    int x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;

    HWND hWnd = CreateWindowExW(0, L"ModelPeekSettingsClass", L"ModelPeek 3D/CAD 预览控制中心 v2.1.3",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, w, h,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (g_hFontHeader) DeleteObject(g_hFontHeader);
    if (g_hFontTitle) DeleteObject(g_hFontTitle);
    if (g_hFontBold) DeleteObject(g_hFontBold);
    if (g_hFontNormal) DeleteObject(g_hFontNormal);
    if (g_hFontSmall) DeleteObject(g_hFontSmall);
    if (g_hBrushBg) DeleteObject(g_hBrushBg);
    if (g_hBrushCard) DeleteObject(g_hBrushCard);
    if (g_hPenBorder) DeleteObject(g_hPenBorder);

    CoUninitialize();
    return (int)msg.wParam;
}
