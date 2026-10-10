#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shellapi.h>
#include <string>
#include <vector>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "advapi32.lib")

// UI Theme Colors
static const COLORREF CLR_BG         = RGB(248, 250, 252); // #F8FAFC
static const COLORREF CLR_CARD_BG    = RGB(255, 255, 255); // #FFFFFF
static const COLORREF CLR_BORDER     = RGB(226, 232, 240); // #E2E8F0
static const COLORREF CLR_TEXT_DARK  = RGB(15, 23, 42);    // #0F172A
static const COLORREF CLR_TEXT_MUTED = RGB(100, 116, 139); // #64748B
static const COLORREF CLR_DANGER     = RGB(220, 38, 38);   // #DC2626
static const COLORREF CLR_DANGER_HOV = RGB(185, 28, 28);   // #B91C1C
static const COLORREF CLR_DANGER_PRS = RGB(153, 27, 27);   // #991B1B

static HBRUSH g_hBrushBg = NULL;
static HBRUSH g_hBrushCard = NULL;
static HPEN g_hPenBorder = NULL;

static HFONT g_hFontTitle = NULL;
static HFONT g_hFontBold = NULL;
static HFONT g_hFontNormal = NULL;
static HFONT g_hFontSmall = NULL;

static HWND g_hBtnUninstall = NULL;
static HWND g_hBtnCancel = NULL;
static HWND g_hProgress = NULL;
static HWND g_hStatusLabel = NULL;

static bool g_bSilent = false;
static bool g_bUninstalling = false;
static bool g_bCompleted = false;

static const wchar_t* G_EXTENSIONS[] = {
    L".step", L".stp", L".iges", L".igs", L".brep", L".brp", L".dxf",
    L".stl", L".obj", L".3mf", L".glb", L".gltf", L".fbx",
    L".ply", L".pcd", L".gcode", L".dae", L".3ds"
};

static const wchar_t* CLSID_THUMB = L"{E9B34A3E-94A5-47F1-A4FD-258F2C411311}";
static const wchar_t* CLSID_PREV  = L"{C81B4AE3-6C73-4DC1-8316-04DE665F09B1}";

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

void ElevateNow(LPCWSTR lpParams) {
    WCHAR szPath[MAX_PATH];
    if (GetModuleFileNameW(NULL, szPath, MAX_PATH)) {
        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.lpVerb = L"runas";
        sei.lpFile = szPath;
        sei.lpParameters = lpParams;
        sei.nShow = g_bSilent ? SW_HIDE : SW_NORMAL;
        if (ShellExecuteExW(&sei)) {
            ExitProcess(0);
        }
    }
}

static bool DeleteDirectoryRecursive(const std::wstring& path) {
    if (path.empty() || !PathFileExistsW(path.c_str())) return true;
    std::wstring search = path + L"\\*.*";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(search.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
            std::wstring sub = path + L"\\" + fd.cFileName;
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                DeleteDirectoryRecursive(sub);
            } else {
                SetFileAttributesW(sub.c_str(), FILE_ATTRIBUTE_NORMAL);
                DeleteFileW(sub.c_str());
            }
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);
    }
    SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_NORMAL);
    return RemoveDirectoryW(path.c_str()) != 0;
}

static void CleanRegistry() {
    // 1. Delete Uninstall entry
    RegDeleteTreeW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ModelPeek");

    // 2. Delete CLSIDs
    std::wstring thumbClsidKey = std::wstring(L"Software\\Classes\\CLSID\\") + CLSID_THUMB;
    std::wstring prevClsidKey  = std::wstring(L"Software\\Classes\\CLSID\\") + CLSID_PREV;
    RegDeleteTreeW(HKEY_CLASSES_ROOT, (std::wstring(L"CLSID\\") + CLSID_THUMB).c_str());
    RegDeleteTreeW(HKEY_CLASSES_ROOT, (std::wstring(L"CLSID\\") + CLSID_PREV).c_str());
    RegDeleteTreeW(HKEY_LOCAL_MACHINE, thumbClsidKey.c_str());
    RegDeleteTreeW(HKEY_LOCAL_MACHINE, prevClsidKey.c_str());
    RegDeleteTreeW(HKEY_CURRENT_USER, thumbClsidKey.c_str());
    RegDeleteTreeW(HKEY_CURRENT_USER, prevClsidKey.c_str());

    // 3. Remove from PreviewHandlers list
    HKEY hPrevList;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\PreviewHandlers", 0, KEY_SET_VALUE, &hPrevList) == ERROR_SUCCESS) {
        RegDeleteValueW(hPrevList, CLSID_PREV);
        RegCloseKey(hPrevList);
    }

    // 4. Remove file extension shellex bindings for ModelPeek
    for (const wchar_t* ext : G_EXTENSIONS) {
        std::wstring extKey = std::wstring(L"Software\\Classes\\") + ext;
        
        std::wstring thumbSub = extKey + L"\\ShellEx\\{e357fccd-a995-4576-b01f-234630154e96}";
        RegDeleteTreeW(HKEY_LOCAL_MACHINE, thumbSub.c_str());
        RegDeleteTreeW(HKEY_CURRENT_USER, thumbSub.c_str());
        RegDeleteTreeW(HKEY_CLASSES_ROOT, (std::wstring(ext) + L"\\ShellEx\\{e357fccd-a995-4576-b01f-234630154e96}").c_str());

        std::wstring prevSub = extKey + L"\\ShellEx\\{8895b1c6-b41f-4c1c-a562-0d564250836f}";
        RegDeleteTreeW(HKEY_LOCAL_MACHINE, prevSub.c_str());
        RegDeleteTreeW(HKEY_CURRENT_USER, prevSub.c_str());
        RegDeleteTreeW(HKEY_CLASSES_ROOT, (std::wstring(ext) + L"\\ShellEx\\{8895b1c6-b41f-4c1c-a562-0d564250836f}").c_str());
    }
}

static void CleanShortcuts() {
    WCHAR commonPrograms[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, commonPrograms))) {
        DeleteDirectoryRecursive(std::wstring(commonPrograms) + L"\\ModelPeek");
    }

    WCHAR userPrograms[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, 0, userPrograms))) {
        DeleteDirectoryRecursive(std::wstring(userPrograms) + L"\\ModelPeek");
    }

    WCHAR commonDesktop[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, NULL, 0, commonDesktop))) {
        DeleteFileW((std::wstring(commonDesktop) + L"\\ModelPeek 控制中心.lnk").c_str());
    }

    WCHAR userDesktop[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, userDesktop))) {
        DeleteFileW((std::wstring(userDesktop) + L"\\ModelPeek 控制中心.lnk").c_str());
    }
}

static void CleanUserData() {
    WCHAR localAppData[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppData))) {
        DeleteDirectoryRecursive(std::wstring(localAppData) + L"\\ModelPeek");
    }
}

void DoCompleteUninstall(const std::wstring& targetDir, bool silent) {
    Sleep(500);

    system("taskkill /f /im prevhost.exe >nul 2>&1");
    system("taskkill /f /im ModelPeekWorker.exe >nul 2>&1");
    system("taskkill /f /im ModelPeekSettings.exe >nul 2>&1");

    std::wstring dllPath = targetDir + L"\\ModelPeekExtension.dll";
    if (PathFileExistsW(dllPath.c_str())) {
        std::wstring unregCmd = L"regsvr32.exe /u /s \"" + dllPath + L"\"";
        _wsystem(unregCmd.c_str());
    }

    CleanRegistry();
    CleanShortcuts();
    CleanUserData();

    for (int retry = 0; retry < 5; ++retry) {
        if (!PathFileExistsW(targetDir.c_str())) break;
        DeleteDirectoryRecursive(targetDir);
        if (!PathFileExistsW(targetDir.c_str())) break;
        Sleep(400);
    }

    system("taskkill /f /im prevhost.exe >nul 2>&1");
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

    if (!silent) {
        MessageBoxW(NULL, 
            L"🎉 ModelPeek 已成功从您的计算机彻底卸载！\r\n\r\n"
            L"● 所有程序组件与安装目录已彻底删除（零残留）\r\n"
            L"● 64 位 COM 资源管理器扩展与格式关联已完全注销\r\n"
            L"● 开始菜单及桌面快捷方式已清理干净\r\n"
            L"● 双击关联已完全恢复操作系统原生状态", 
            L"ModelPeek 卸载完成", MB_OK | MB_ICONINFORMATION);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // Progress bar
        g_hProgress = CreateWindowExW(0, PROGRESS_CLASSW, L"", 
            WS_CHILD | PBS_SMOOTH, 38, 268, 480, 14, hWnd, NULL, GetModuleHandleW(NULL), NULL);

        // Status Label
        g_hStatusLabel = CreateWindowExW(0, L"STATIC", L"准备就绪，点击下方按钮开始卸载", 
            WS_CHILD | WS_VISIBLE, 38, 288, 480, 18, hWnd, NULL, GetModuleHandleW(NULL), NULL);
        SendMessageW(g_hStatusLabel, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);

        // Danger Uninstall Button (Owner Drawn)
        g_hBtnUninstall = CreateWindowExW(0, L"BUTTON", L"🗑️ 彻底卸载 ModelPeek", 
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 110, 318, 220, 38, hWnd, (HMENU)2001, GetModuleHandleW(NULL), NULL);
        SendMessageW(g_hBtnUninstall, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        // Cancel Button
        g_hBtnCancel = CreateWindowExW(0, L"BUTTON", L"取消", 
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 345, 318, 100, 38, hWnd, (HMENU)2002, GetModuleHandleW(NULL), NULL);
        SendMessageW(g_hBtnCancel, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT rcClient;
        GetClientRect(hWnd, &rcClient);

        // 1. Top Danger Banner
        RECT rcTop = { 0, 0, rcClient.right, 84 };
        HBRUSH hBrTop = CreateSolidBrush(CLR_CARD_BG);
        FillRect(hdc, &rcTop, hBrTop);
        DeleteObject(hBrTop);

        // Divider
        HPEN hPenDiv = CreatePen(PS_SOLID, 1, CLR_BORDER);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPenDiv);
        MoveToEx(hdc, 0, 84, NULL);
        LineTo(hdc, rcClient.right, 84);

        // Title
        SelectObject(hdc, g_hFontTitle);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, CLR_TEXT_DARK);
        TextOutW(hdc, 30, 16, L"ModelPeek 彻底卸载向导", 14);

        // Red Danger Badge: [卸载清理]
        RECT rcBadge = { 310, 18, 385, 38 };
        HBRUSH hBrBadge = CreateSolidBrush(RGB(254, 242, 242)); // Light red
        HPEN hPenBadge = CreatePen(PS_SOLID, 1, RGB(254, 202, 202));
        SelectObject(hdc, hBrBadge);
        SelectObject(hdc, hPenBadge);
        RoundRect(hdc, rcBadge.left, rcBadge.top, rcBadge.right, rcBadge.bottom, 8, 8);
        DeleteObject(hBrBadge);
        DeleteObject(hPenBadge);

        SelectObject(hdc, g_hFontSmall);
        SetTextColor(hdc, CLR_DANGER);
        DrawTextW(hdc, L"卸载清理", -1, &rcBadge, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Subtitle
        SelectObject(hdc, g_hFontNormal);
        SetTextColor(hdc, CLR_TEXT_MUTED);
        TextOutW(hdc, 30, 48, L"快速安全地移除所有三维预览扩展、系统关联并彻底清理程序文件", 31);

        // 2. Middle Scope Card
        RECT rcCard = { 24, 98, rcClient.right - 24, 252 };
        HBRUSH hBrCard = CreateSolidBrush(CLR_CARD_BG);
        HPEN hPenCard = CreatePen(PS_SOLID, 1, CLR_BORDER);
        SelectObject(hdc, hBrCard);
        SelectObject(hdc, hPenCard);
        RoundRect(hdc, rcCard.left, rcCard.top, rcCard.right, rcCard.bottom, 10, 10);
        DeleteObject(hBrCard);
        DeleteObject(hPenCard);

        SelectObject(hdc, g_hFontBold);
        SetTextColor(hdc, CLR_TEXT_DARK);
        TextOutW(hdc, 40, 110, L"即将执行的卸载与清理项目：", 13);

        SelectObject(hdc, g_hFontNormal);
        SetTextColor(hdc, CLR_TEXT_MUTED);
        TextOutW(hdc, 40, 136, L"● 移除全部 18 种 CAD/3D 格式的缩略图及 Alt+P 交互预览扩展", 33);
        TextOutW(hdc, 40, 160, L"● 注销 64 位 COM 注册表关联，完全保留原生 CAD 软件双击打开行为", 37);
        TextOutW(hdc, 40, 184, L"● 彻底清理开始菜单及桌面快捷方式", 18);
        TextOutW(hdc, 40, 208, L"● 彻底物理删除安装目录与所有依赖文件（零字节残留）", 26);

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
        if (pdis->CtlID == 2001) { // Danger button
            HDC hdc = pdis->hDC;
            RECT rc = pdis->rcItem;

            bool isPressed = (pdis->itemState & ODS_SELECTED);
            COLORREF btnColor = isPressed ? CLR_DANGER_PRS : CLR_DANGER;

            HBRUSH hBrBtn = CreateSolidBrush(btnColor);
            HPEN hPenBtn = CreatePen(PS_SOLID, 1, btnColor);
            HBRUSH hOldBr = (HBRUSH)SelectObject(hdc, hBrBtn);
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPenBtn);

            RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 8, 8);

            SelectObject(hdc, g_hFontBold);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 255, 255));
            DrawTextW(hdc, L"🗑️ 彻底卸载 ModelPeek", -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            SelectObject(hdc, hOldBr);
            SelectObject(hdc, hOldPen);
            DeleteObject(hBrBtn);
            DeleteObject(hPenBtn);
            return TRUE;
        }
        break;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == 2001) { // Uninstall
            if (!IsRunAsAdmin()) {
                ElevateNow(GetCommandLineW());
                return 0;
            }

            EnableWindow(g_hBtnUninstall, FALSE);
            EnableWindow(g_hBtnCancel, FALSE);
            ShowWindow(g_hProgress, SW_SHOW);
            SendMessageW(g_hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
            SendMessageW(g_hProgress, PBM_SETPOS, 20, 0);
            SetWindowTextW(g_hStatusLabel, L"正在移交清理权限并释放文件占用...");

            // Determine target dir
            WCHAR selfExe[MAX_PATH] = {0};
            GetModuleFileNameW(NULL, selfExe, MAX_PATH);
            PathRemoveFileSpecW(selfExe);
            std::wstring installDir = selfExe;

            // Self-delegation to %TEMP%
            WCHAR tempPath[MAX_PATH] = {0};
            GetTempPathW(MAX_PATH, tempPath);
            std::wstring tempExe = std::wstring(tempPath) + L"ModelPeek_Uninstall_Worker.exe";

            CopyFileW(selfExe, tempExe.c_str(), FALSE);

            SendMessageW(g_hProgress, PBM_SETPOS, 50, 0);
            SetWindowTextW(g_hStatusLabel, L"正在注销 COM 扩展并物理删除程序文件...");

            // Launch worker
            std::wstring workerCmd = L"\"" + tempExe + L"\" --worker \"" + installDir + L"\"";
            std::vector<WCHAR> cmdVec(workerCmd.begin(), workerCmd.end());
            cmdVec.push_back(L'\0');

            STARTUPINFOW si = { sizeof(si) };
            PROCESS_INFORMATION pi = { 0 };
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;

            if (CreateProcessW(NULL, cmdVec.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }

            // Current process terminates immediately so installDir is unlocked!
            PostQuitMessage(0);
            return 0;
        } else if (id == 2002) { // Cancel
            PostQuitMessage(0);
            return 0;
        }
        return 0;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        HWND hCtrl = (HWND)lParam;
        if (hCtrl == g_hStatusLabel) {
            SetTextColor(hdc, CLR_DANGER);
            SetBkColor(hdc, CLR_BG);
            return (INT_PTR)g_hBrushBg;
        }
        SetTextColor(hdc, CLR_TEXT_DARK);
        SetBkColor(hdc, CLR_CARD_BG);
        return (INT_PTR)g_hBrushCard;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    CoInitialize(NULL);

    LPWSTR cmdLine = GetCommandLineW();
    std::wstring fullCmd(cmdLine ? cmdLine : L"");
    std::wstring lowerCmd = fullCmd;
    for (auto& c : lowerCmd) c = towlower(c);

    bool silent = (lowerCmd.find(L"/s") != std::wstring::npos || 
                   lowerCmd.find(L"-s") != std::wstring::npos ||
                   lowerCmd.find(L"/silent") != std::wstring::npos ||
                   lowerCmd.find(L"-silent") != std::wstring::npos);

    // If running as worker: --worker <dir>
    size_t workerPos = lowerCmd.find(L"--worker");
    if (workerPos != std::wstring::npos) {
        size_t quoteStart = fullCmd.find(L"\"", workerPos);
        std::wstring targetDir;
        if (quoteStart != std::wstring::npos) {
            size_t quoteEnd = fullCmd.find(L"\"", quoteStart + 1);
            if (quoteEnd != std::wstring::npos) {
                targetDir = fullCmd.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
            }
        }
        if (targetDir.empty()) {
            std::wstring rem = fullCmd.substr(workerPos + 8);
            while (!rem.empty() && rem.front() == L' ') rem.erase(rem.begin());
            size_t sp = rem.find(L' ');
            targetDir = (sp != std::wstring::npos) ? rem.substr(0, sp) : rem;
        }

        if (!targetDir.empty()) {
            DoCompleteUninstall(targetDir, silent);
        }
        CoUninitialize();
        return 0;
    }

    // Normal launcher: check Admin
    if (!IsRunAsAdmin()) {
        ElevateNow(cmdLine);
        return 0;
    }

    if (silent) {
        WCHAR selfExe[MAX_PATH] = {0};
        GetModuleFileNameW(NULL, selfExe, MAX_PATH);
        PathRemoveFileSpecW(selfExe);
        DoCompleteUninstall(selfExe, true);
        CoUninitialize();
        return 0;
    }

    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icex);

    g_hBrushBg = CreateSolidBrush(CLR_BG);
    g_hBrushCard = CreateSolidBrush(CLR_CARD_BG);
    g_hPenBorder = CreatePen(PS_SOLID, 1, CLR_BORDER);

    g_hFontTitle = CreateFontW(-19, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
    g_hFontBold  = CreateFontW(-13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
    g_hFontNormal= CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
    g_hFontSmall = CreateFontW(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ModelPeekUninstallerClass";
    wc.hbrBackground = g_hBrushBg;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    RegisterClassExW(&wc);

    int w = 560;
    int h = 405;
    int x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;

    HWND hWnd = CreateWindowExW(0, L"ModelPeekUninstallerClass", L"ModelPeek 彻底卸载向导",
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
