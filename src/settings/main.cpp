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

// CLSIDs
static const LPCWSTR CLSID_THUMB_KEY = L"{e357fccd-a995-4576-b01f-234630154e96}";
static const LPCWSTR CLSID_PREV_KEY  = L"{8895b1c6-b41f-4c1c-a562-0d564250836f}";
static const LPCWSTR CLSID_THUMB_VAL = L"{D37F2633-149C-4CD8-B60F-FCE3B40B7E98}";
static const LPCWSTR CLSID_PREV_VAL  = L"{B88E5A73-611D-41D1-A909-54E3A94D2517}";

struct FormatInfo {
    std::wstring ext;
    std::wstring label;
    std::wstring category;
    HWND hCheckbox;
};

static std::vector<FormatInfo> g_formats = {
    // CAD
    { L".step", L".step (STEP 工业零件)", L"CAD 工业标准格式", NULL },
    { L".stp",  L".stp (STEP 工业装配)", L"CAD 工业标准格式", NULL },
    { L".iges", L".iges (IGES 曲线/实体)", L"CAD 工业标准格式", NULL },
    { L".igs",  L".igs (IGES 模型)", L"CAD 工业标准格式", NULL },
    { L".brep", L".brep (OpenCASCADE 边界表示)", L"CAD 工业标准格式", NULL },
    { L".brp",  L".brp (B-Rep 拓扑模型)", L"CAD 工业标准格式", NULL },
    // Mesh
    { L".stl",  L".stl (Stereolithography 三角网格)", L"通用三维网格格式", NULL },
    { L".obj",  L".obj (Wavefront 3D 对象)", L"通用三维网格格式", NULL },
    { L".glb",  L".glb (二进制 glTF 传输模型)", L"通用三维网格格式", NULL },
    { L".gltf", L".gltf (glTF 3D 交换格式)", L"通用三维网格格式", NULL },
    { L".3mf",  L".3mf (3D Manufacturing 制造格式)", L"通用三维网格格式", NULL },
    { L".fbx",  L".fbx (Autodesk 多边形与 NURBS)", L"通用三维网格格式", NULL },
    // Scan & Toolpath & CG
    { L".ply",  L".ply (Stanford 扫描点云网格)", L"逆向工程与制造格式", NULL },
    { L".gcode",L".gcode (CNC / 3D 打印切片刀轨)", L"逆向工程与制造格式", NULL },
    { L".dae",  L".dae (Collada 交互式 3D 资产)", L"经典 3D 格式", NULL },
    { L".3ds",  L".3ds (3D Studio 经典网格模型)", L"经典 3D 格式", NULL }
};

static HWND g_hTab = NULL;
static HWND g_hPanelFormats = NULL;
static HWND g_hPanelStatus = NULL;
static HWND g_hPanelCache = NULL;
static HFONT g_hFontNormal = NULL;
static HFONT g_hFontBold = NULL;
static HFONT g_hFontTitle = NULL;

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
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        WCHAR val[128] = {0};
        DWORD size = sizeof(val);
        RegQueryValueExW(hKey, NULL, NULL, NULL, (LPBYTE)val, &size);
        RegCloseKey(hKey);
        if (_wcsicmp(val, CLSID_THUMB_VAL) == 0) return true;
    }
    // Also check HKCU
    if (RegOpenKeyExW(HKEY_CURRENT_USER, subKey.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        WCHAR val[128] = {0};
        DWORD size = sizeof(val);
        RegQueryValueExW(hKey, NULL, NULL, NULL, (LPBYTE)val, &size);
        RegCloseKey(hKey);
        if (_wcsicmp(val, CLSID_THUMB_VAL) == 0) return true;
    }
    return false;
}

bool SetFormatRegistration(const std::wstring& ext, bool enable) {
    HKEY root = IsRunAsAdmin() ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
    std::wstring thumbSub = L"Software\\Classes\\" + ext + L"\\ShellEx\\" + CLSID_THUMB_KEY;
    std::wstring prevSub  = L"Software\\Classes\\" + ext + L"\\ShellEx\\" + CLSID_PREV_KEY;

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
        RegDeleteKeyW(root, thumbSub.c_str());
        RegDeleteKeyW(root, prevSub.c_str());
        // Clean both HKLM and HKCU if admin
        if (IsRunAsAdmin()) {
            RegDeleteKeyW(HKEY_CURRENT_USER, thumbSub.c_str());
            RegDeleteKeyW(HKEY_CURRENT_USER, prevSub.c_str());
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

void RestartExplorer() {
    // Terminate prevhost and notify shell
    system("taskkill /f /im prevhost.exe >nul 2>&1");
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
}

// Panel Creation
void CreateFormatsPanel(HWND hParent) {
    g_hPanelFormats = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 
        15, 45, 590, 410, hParent, NULL, GetModuleHandleW(NULL), NULL);

    HWND hLbl = CreateWindowExW(0, L"STATIC", L"选择由 ModelPeek 接管 3D 缩略图与视口预览的文件格式：", 
        WS_CHILD | WS_VISIBLE, 10, 5, 570, 20, g_hPanelFormats, NULL, GetModuleHandleW(NULL), NULL);
    SendMessageW(hLbl, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    int startX1 = 15;
    int startX2 = 300;
    int startY = 32;
    int rowH = 24;

    for (size_t i = 0; i < g_formats.size(); i++) {
        int x = (i < 8) ? startX1 : startX2;
        int y = startY + ((int)(i % 8)) * rowH;

        HWND hChk = CreateWindowExW(0, L"BUTTON", g_formats[i].label.c_str(),
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            x, y, 270, 20, g_hPanelFormats, (HMENU)(UINT_PTR)(1000 + i), GetModuleHandleW(NULL), NULL);
        SendMessageW(hChk, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        SendMessageW(hChk, BM_SETCHECK, IsFormatRegistered(g_formats[i].ext) ? BST_CHECKED : BST_UNCHECKED, 0);
        g_formats[i].hCheckbox = hChk;
    }

    // Buttons
    HWND btnSelectAll = CreateWindowExW(0, L"BUTTON", L"全选", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        15, 235, 75, 26, g_hPanelFormats, (HMENU)100, GetModuleHandleW(NULL), NULL);
    SendMessageW(btnSelectAll, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    HWND btnClearAll = CreateWindowExW(0, L"BUTTON", L"全不选", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        100, 235, 75, 26, g_hPanelFormats, (HMENU)101, GetModuleHandleW(NULL), NULL);
    SendMessageW(btnClearAll, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    HWND btnApply = CreateWindowExW(0, L"BUTTON", L"💾 保存并应用设置", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        420, 235, 150, 28, g_hPanelFormats, (HMENU)102, GetModuleHandleW(NULL), NULL);
    SendMessageW(btnApply, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

    HWND hHint = CreateWindowExW(0, L"STATIC", L"提示：双击文件仍保持原本的 SolidWorks / NX / Blender 等软件关联，仅开启缩略图与 Alt+P 预览。",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 15, 280, 560, 40, g_hPanelFormats, NULL, GetModuleHandleW(NULL), NULL);
    SendMessageW(hHint, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
}

void CreateStatusPanel(HWND hParent) {
    g_hPanelStatus = CreateWindowExW(0, L"STATIC", L"", WS_CHILD, 
        15, 45, 590, 410, hParent, NULL, GetModuleHandleW(NULL), NULL);

    HWND hTitle = CreateWindowExW(0, L"STATIC", L"组件状态与系统环境体检", 
        WS_CHILD | WS_VISIBLE, 10, 5, 570, 25, g_hPanelStatus, NULL, GetModuleHandleW(NULL), NULL);
    SendMessageW(hTitle, WM_SETFONT, (WPARAM)g_hFontTitle, TRUE);

    std::wstring appDir = GetAppDir();
    std::wstring dllPath = appDir + L"\\ModelPeekExtension.dll";
    std::wstring workerPath = appDir + L"\\ModelPeekWorker.exe";

    std::wstringstream ss;
    ss << L"● 安装运行目录: " << appDir << L"\r\n\r\n";
    ss << L"● COM 核心扩展 (ModelPeekExtension.dll): " << (PathFileExistsW(dllPath.c_str()) ? L"✓ 正常就绪" : L"✗ 缺失") << L"\r\n";
    ss << L"● 后台渲染进程 (ModelPeekWorker.exe): " << (PathFileExistsW(workerPath.c_str()) ? L"✓ 正常就绪" : L"✗ 缺失") << L"\r\n";
    ss << L"● 当前运行权限: " << (IsRunAsAdmin() ? L"管理员模式 (Administrator)" : L"普通用户权限") << L"\r\n";
    ss << L"● 注册表状态: " << (IsFormatRegistered(L".stl") ? L"已激活并接管模型" : L"未完全激活") << L"\r\n";

    HWND hEdit = CreateWindowExW(WS_EX_STATICEDGE, L"EDIT", ss.str().c_str(),
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL,
        15, 35, 560, 150, g_hPanelStatus, NULL, GetModuleHandleW(NULL), NULL);
    SendMessageW(hEdit, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    HWND btnReinstall = CreateWindowExW(0, L"BUTTON", L"🔧 一键重新注册激活扩展", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        15, 200, 200, 30, g_hPanelStatus, (HMENU)201, GetModuleHandleW(NULL), NULL);
    SendMessageW(btnReinstall, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    HWND btnUninstall = CreateWindowExW(0, L"BUTTON", L"🗑️ 彻底注销扩展组件", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        230, 200, 160, 30, g_hPanelStatus, (HMENU)202, GetModuleHandleW(NULL), NULL);
    SendMessageW(btnUninstall, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
}

void UpdateCacheUI(HWND hLabel) {
    int count = 0;
    double sizeMB = 0.0;
    GetCacheStats(count, sizeMB);

    std::wstringstream ss;
    ss << L"当前本地缩略图二级缓存目录：\r\n"
       << GetCacheDirPath() << L"\r\n\r\n"
       << L"当前已缓存文件数: " << count << L" 个\r\n"
       << L"已占用磁盘空间: " << std::fixed << std::setprecision(2) << sizeMB << L" MB\r\n\r\n"
       << L"说明：ModelPeek 自动缓存已生成的快速光栅化三维缩略图，再次进入文件夹无需重复计算。如遇模型更新未能及时刷新，可在此一键清空。";
    SetWindowTextW(hLabel, ss.str().c_str());
}

static HWND g_hCacheInfoText = NULL;

void CreateCachePanel(HWND hParent) {
    g_hPanelCache = CreateWindowExW(0, L"STATIC", L"", WS_CHILD, 
        15, 45, 590, 410, hParent, NULL, GetModuleHandleW(NULL), NULL);

    HWND hTitle = CreateWindowExW(0, L"STATIC", L"缩略图本地磁盘缓存管理", 
        WS_CHILD | WS_VISIBLE, 10, 5, 570, 25, g_hPanelCache, NULL, GetModuleHandleW(NULL), NULL);
    SendMessageW(hTitle, WM_SETFONT, (WPARAM)g_hFontTitle, TRUE);

    g_hCacheInfoText = CreateWindowExW(WS_EX_STATICEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL,
        15, 35, 560, 150, g_hPanelCache, NULL, GetModuleHandleW(NULL), NULL);
    SendMessageW(g_hCacheInfoText, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    UpdateCacheUI(g_hCacheInfoText);

    HWND btnClear = CreateWindowExW(0, L"BUTTON", L"🧹 一键清空缩略图缓存", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        15, 200, 180, 30, g_hPanelCache, (HMENU)301, GetModuleHandleW(NULL), NULL);
    SendMessageW(btnClear, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    HWND btnRestartExp = CreateWindowExW(0, L"BUTTON", L"🔄 刷新 Windows 图标缓存", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        210, 200, 190, 30, g_hPanelCache, (HMENU)302, GetModuleHandleW(NULL), NULL);
    SendMessageW(btnRestartExp, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
}

void SwitchTab(int index) {
    ShowWindow(g_hPanelFormats, (index == 0) ? SW_SHOW : SW_HIDE);
    ShowWindow(g_hPanelStatus,  (index == 1) ? SW_SHOW : SW_HIDE);
    ShowWindow(g_hPanelCache,   (index == 2) ? SW_SHOW : SW_HIDE);
    if (index == 2 && g_hCacheInfoText) {
        UpdateCacheUI(g_hCacheInfoText);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // Init tabs
        g_hTab = CreateWindowExW(0, WC_TABCONTROLW, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
            10, 10, 605, 360, hWnd, (HMENU)1, GetModuleHandleW(NULL), NULL);
        SendMessageW(g_hTab, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

        TCITEMW tie = {0};
        tie.mask = TCIF_TEXT;
        tie.pszText = (LPWSTR)L"📁 格式管理";
        TabCtrl_InsertItem(g_hTab, 0, &tie);
        tie.pszText = (LPWSTR)L"🩺 系统体检";
        TabCtrl_InsertItem(g_hTab, 1, &tie);
        tie.pszText = (LPWSTR)L"⚡ 缓存管理";
        TabCtrl_InsertItem(g_hTab, 2, &tie);

        CreateFormatsPanel(hWnd);
        CreateStatusPanel(hWnd);
        CreateCachePanel(hWnd);
        SwitchTab(0);
        return 0;
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
                if (fmt.hCheckbox) SendMessageW(fmt.hCheckbox, BM_SETCHECK, BST_CHECKED, 0);
            }
        } else if (id == 101) { // Clear all
            for (auto& fmt : g_formats) {
                if (fmt.hCheckbox) SendMessageW(fmt.hCheckbox, BM_SETCHECK, BST_UNCHECKED, 0);
            }
        } else if (id == 102) { // Apply
            int countActive = 0;
            for (auto& fmt : g_formats) {
                if (fmt.hCheckbox) {
                    bool chk = (SendMessageW(fmt.hCheckbox, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    SetFormatRegistration(fmt.ext, chk);
                    if (chk) countActive++;
                }
            }
            RestartExplorer();
            std::wstringstream ss;
            ss << L"设置已成功应用！\r\n当前已激活接管 " << countActive << L" 种 3D/CAD 文件格式。";
            MessageBoxW(hWnd, ss.str().c_str(), L"ModelPeek 设置成功", MB_OK | MB_ICONINFORMATION);
        } else if (id == 201) { // Re-register
            std::wstring bat = GetAppDir() + L"\\install.bat";
            ShellExecuteW(hWnd, L"open", bat.c_str(), NULL, GetAppDir().c_str(), SW_SHOWNORMAL);
        } else if (id == 202) { // Uninstall
            if (MessageBoxW(hWnd, L"确定要注销 ModelPeek 扩展吗？这会取消资源管理器的所有 3D 预览关联。", L"确认注销", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                std::wstring bat = GetAppDir() + L"\\uninstall.bat";
                ShellExecuteW(hWnd, L"open", bat.c_str(), NULL, GetAppDir().c_str(), SW_SHOWNORMAL);
            }
        } else if (id == 301) { // Clear Cache
            ClearCacheFiles();
            UpdateCacheUI(g_hCacheInfoText);
            RestartExplorer();
            MessageBoxW(hWnd, L"本地缩略图缓存已彻底清空！", L"提示", MB_OK | MB_ICONINFORMATION);
        } else if (id == 302) { // Restart explorer
            RestartExplorer();
            MessageBoxW(hWnd, L"Windows 图标缓存已通知刷新！", L"提示", MB_OK | MB_ICONINFORMATION);
        }
        return 0;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, RGB(30, 41, 59));
        SetBkColor(hdcStatic, GetSysColor(COLOR_BTNFACE));
        return (INT_PTR)GetSysColorBrush(COLOR_BTNFACE);
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_TAB_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icex);

    g_hFontNormal = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
    g_hFontBold = CreateFontW(-12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
    g_hFontTitle = CreateFontW(-14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ModelPeekSettingsClass";
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    RegisterClassExW(&wc);

    HWND hWnd = CreateWindowExW(0, L"ModelPeekSettingsClass", L"ModelPeek 控制中心 (Settings v2.0)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 640, 420,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (g_hFontNormal) DeleteObject(g_hFontNormal);
    if (g_hFontBold) DeleteObject(g_hFontBold);
    if (g_hFontTitle) DeleteObject(g_hFontTitle);

    return (int)msg.wParam;
}
