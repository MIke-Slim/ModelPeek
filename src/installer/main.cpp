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
#include <sstream>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "uxtheme.lib")

#define IDR_PAYLOAD_ZIP 101
#define MODELPEEK_VERSION_STR L"2.1.3"

// UI Theme Colors
static const COLORREF CLR_BG         = RGB(248, 250, 252); // #F8FAFC
static const COLORREF CLR_CARD_BG    = RGB(255, 255, 255); // #FFFFFF
static const COLORREF CLR_BORDER     = RGB(226, 232, 240); // #E2E8F0
static const COLORREF CLR_TEXT_DARK  = RGB(15, 23, 42);    // #0F172A
static const COLORREF CLR_TEXT_MUTED = RGB(100, 116, 139); // #64748B
static const COLORREF CLR_PRIMARY    = RGB(37, 99, 235);   // #2563EB
static const COLORREF CLR_PRIMARY_HOV= RGB(29, 78, 216);   // #1D4ED8
static const COLORREF CLR_PRIMARY_PRS= RGB(30, 64, 175);   // #1E40AF

static HBRUSH g_hBrushBg = NULL;
static HBRUSH g_hBrushCard = NULL;
static HPEN g_hPenBorder = NULL;

static HWND g_hPathEdit = NULL;
static HWND g_hChkShell = NULL;
static HWND g_hChkStartMenu = NULL;
static HWND g_hChkDesktop = NULL;
static HWND g_hBtnInstall = NULL;
static HWND g_hBtnBrowse = NULL;
static HWND g_hProgress = NULL;
static HWND g_hStatusLabel = NULL;

static HFONT g_hFontTitle = NULL;
static HFONT g_hFontNormal = NULL;
static HFONT g_hFontBold = NULL;
static HFONT g_hFontSmall = NULL;

static bool g_bSilent = false;
static bool g_bBtnHover = false;
static bool g_bBtnPressed = false;

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

void ElevateNow(HWND hWnd, LPCWSTR lpParameters = NULL) {
    WCHAR szPath[MAX_PATH];
    if (GetModuleFileNameW(NULL, szPath, MAX_PATH)) {
        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.lpVerb = L"runas";
        sei.lpFile = szPath;
        sei.lpParameters = lpParameters;
        sei.hwnd = hWnd;
        sei.nShow = g_bSilent ? SW_HIDE : SW_NORMAL;
        if (ShellExecuteExW(&sei)) {
            ExitProcess(0);
        }
    }
}

static void UnblockDirectoryRecursive(const std::wstring& dir, int maxDepth = 4, int currentDepth = 0) {
    if (currentDepth > maxDepth || dir.empty()) return;
    std::wstring s = dir + L"\\*.*";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(s.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;
    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
        std::wstring fp = dir + L"\\" + fd.cFileName;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (_wcsicmp(fd.cFileName, L"AppData") == 0 || 
                _wcsicmp(fd.cFileName, L"$Recycle.Bin") == 0 ||
                _wcsicmp(fd.cFileName, L"Windows") == 0 ||
                _wcsicmp(fd.cFileName, L".git") == 0) {
                continue;
            }
            UnblockDirectoryRecursive(fp, maxDepth, currentDepth + 1);
        } else {
            std::wstring zs = fp + L":Zone.Identifier";
            DeleteFileW(zs.c_str());
        }
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);
}

static void UnblockCommonModelLocations(const std::wstring& targetDir) {
    // 1. Unblock installation target directory
    UnblockDirectoryRecursive(targetDir, 5, 0);

    // 2. Unblock installer's own directory
    WCHAR exePath[MAX_PATH] = {0};
    if (GetModuleFileNameW(NULL, exePath, MAX_PATH)) {
        PathRemoveFileSpecW(exePath);
        if (wcslen(exePath) > 0) {
            UnblockDirectoryRecursive(exePath, 4, 0);
        }
    }

    // 3. Unblock User Desktop and Common Desktop
    WCHAR desktopPath[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktopPath))) {
        UnblockDirectoryRecursive(desktopPath, 3, 0);
    }
    WCHAR commonDesktopPath[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, NULL, 0, commonDesktopPath))) {
        UnblockDirectoryRecursive(commonDesktopPath, 3, 0);
    }

    // 4. Unblock User Downloads
    WCHAR userProfile[MAX_PATH] = {0};
    if (GetEnvironmentVariableW(L"USERPROFILE", userProfile, MAX_PATH)) {
        std::wstring downloads = std::wstring(userProfile) + L"\\Downloads";
        UnblockDirectoryRecursive(downloads, 3, 0);
    }

    // 5. Unblock User Documents
    WCHAR personalPath[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, 0, personalPath))) {
        UnblockDirectoryRecursive(personalPath, 3, 0);
    }
}

HRESULT CreateShortcut(LPCWSTR lpszPathObj, LPCWSTR lpszPathLink, LPCWSTR lpszDesc, LPCWSTR lpszIconPath) {
    IShellLinkW* psl = NULL;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID*)&psl);
    if (SUCCEEDED(hr)) {
        psl->SetPath(lpszPathObj);
        psl->SetDescription(lpszDesc);
        if (lpszIconPath) psl->SetIconLocation(lpszIconPath, 0);
        IPersistFile* ppf = NULL;
        hr = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
        if (SUCCEEDED(hr)) {
            hr = ppf->Save(lpszPathLink, TRUE);
            ppf->Release();
        }
        psl->Release();
    }
    return hr;
}

std::wstring GetDefaultInstallDir() {
    WCHAR programFiles[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAM_FILES, NULL, 0, programFiles))) {
        return std::wstring(programFiles) + L"\\ModelPeek";
    }
    return L"C:\\Program Files\\ModelPeek";
}

bool ExtractPayloadToDirectory(const std::wstring& targetDir) {
    HRSRC hRes = FindResourceW(NULL, MAKEINTRESOURCEW(IDR_PAYLOAD_ZIP), RT_RCDATA);
    if (!hRes) return false;

    HGLOBAL hMem = LoadResource(NULL, hRes);
    if (!hMem) return false;

    DWORD resSize = SizeofResource(NULL, hRes);
    LPVOID pData = LockResource(hMem);
    if (!pData || resSize == 0) return false;

    WCHAR tempPath[MAX_PATH] = {0};
    GetTempPathW(MAX_PATH, tempPath);
    std::wstring tempZip = std::wstring(tempPath) + L"modelpeek_setup_temp.zip";

    HANDLE hFile = CreateFileW(tempZip.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD written = 0;
    WriteFile(hFile, pData, resSize, &written, NULL);
    CloseHandle(hFile);

    CreateDirectoryW(targetDir.c_str(), NULL);

    std::wstringstream psCmd;
    psCmd << L"powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "
          << L"\"Expand-Archive -LiteralPath '" << tempZip << L"' -DestinationPath '" << targetDir << L"' -Force\"";

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    std::wstring cmdStr = psCmd.str();
    std::vector<WCHAR> cmdLine(cmdStr.begin(), cmdStr.end());
    cmdLine.push_back(L'\0');

    if (CreateProcessW(NULL, cmdLine.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 30000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    DeleteFileW(tempZip.c_str());

    UnblockDirectoryRecursive(targetDir, 5, 0);
    return true;
}

void RegisterShellExtension(const std::wstring& targetDir) {
    std::wstring dllPath = targetDir + L"\\ModelPeekExtension.dll";
    std::wstringstream cmd;
    cmd << L"regsvr32.exe /s \"" << dllPath << L"\"";

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    std::wstring cmdStr = cmd.str();
    std::vector<WCHAR> cmdLine(cmdStr.begin(), cmdStr.end());
    cmdLine.push_back(L'\0');

    if (CreateProcessW(NULL, cmdLine.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 10000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

void RegisterUninstallEntry(const std::wstring& targetDir) {
    HKEY hKey;
    LPCWSTR subKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ModelPeek";
    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        LPCWSTR name = L"ModelPeek 3D/CAD 资源管理器预览扩展";
        LPCWSTR ver = MODELPEEK_VERSION_STR;
        LPCWSTR pub = L"ModelPeek Team";
        std::wstring uninst = L"\"" + targetDir + L"\\ModelPeekUninstall.exe\"";
        std::wstring quietUninst = L"\"" + targetDir + L"\\ModelPeekUninstall.exe\" /S";
        std::wstring icon = targetDir + L"\\ModelPeekSettings.exe,0";

        RegSetValueExW(hKey, L"DisplayName", 0, REG_SZ, (const BYTE*)name, (DWORD)((wcslen(name)+1)*sizeof(WCHAR)));
        RegSetValueExW(hKey, L"DisplayVersion", 0, REG_SZ, (const BYTE*)ver, (DWORD)((wcslen(ver)+1)*sizeof(WCHAR)));
        RegSetValueExW(hKey, L"Publisher", 0, REG_SZ, (const BYTE*)pub, (DWORD)((wcslen(pub)+1)*sizeof(WCHAR)));
        RegSetValueExW(hKey, L"InstallLocation", 0, REG_SZ, (const BYTE*)targetDir.c_str(), (DWORD)((targetDir.length()+1)*sizeof(WCHAR)));
        RegSetValueExW(hKey, L"UninstallString", 0, REG_SZ, (const BYTE*)uninst.c_str(), (DWORD)((uninst.length()+1)*sizeof(WCHAR)));
        RegSetValueExW(hKey, L"QuietUninstallString", 0, REG_SZ, (const BYTE*)quietUninst.c_str(), (DWORD)((quietUninst.length()+1)*sizeof(WCHAR)));
        RegSetValueExW(hKey, L"DisplayIcon", 0, REG_SZ, (const BYTE*)icon.c_str(), (DWORD)((icon.length()+1)*sizeof(WCHAR)));
        RegCloseKey(hKey);
    }
}

void DoInstallation(HWND hWnd) {
    std::wstring targetDir;
    if (g_hPathEdit) {
        WCHAR pathBuf[MAX_PATH] = {0};
        GetWindowTextW(g_hPathEdit, pathBuf, MAX_PATH);
        targetDir = pathBuf;
    }
    if (targetDir.empty()) targetDir = GetDefaultInstallDir();

    if (g_hBtnInstall) EnableWindow(g_hBtnInstall, FALSE);
    if (g_hProgress) {
        ShowWindow(g_hProgress, SW_SHOW);
        SendMessageW(g_hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        SendMessageW(g_hProgress, PBM_SETPOS, 15, 0);
    }
    if (g_hStatusLabel) SetWindowTextW(g_hStatusLabel, L"正在解锁文件并清理旧版本...");

    // 1. Terminate running host and worker processes
    system("taskkill /f /im prevhost.exe >nul 2>&1");
    system("taskkill /f /im ModelPeekWorker.exe >nul 2>&1");
    system("taskkill /f /im ModelPeekSettings.exe >nul 2>&1");

    // 2. Unregister previous COM DLL if existing
    std::wstring oldDll = targetDir + L"\\ModelPeekExtension.dll";
    if (PathFileExistsW(oldDll.c_str())) {
        std::wstring unregCmd = L"regsvr32.exe /u /s \"" + oldDll + L"\"";
        _wsystem(unregCmd.c_str());
    }

    // 3. Remove obsolete legacy binaries
    std::wstring oldPeek = targetDir + L"\\ModelPeekPeek.exe";
    if (PathFileExistsW(oldPeek.c_str())) {
        DeleteFileW(oldPeek.c_str());
    }

    if (g_hStatusLabel) SetWindowTextW(g_hStatusLabel, L"正在解压核心组件与 WebGL 视口引擎...");
    if (g_hProgress) SendMessageW(g_hProgress, PBM_SETPOS, 40, 0);

    if (!ExtractPayloadToDirectory(targetDir)) {
        if (!g_bSilent && hWnd) {
            MessageBoxW(hWnd, L"解压安装组件失败，请检查目标路径权限或磁盘空间！", L"安装失败", MB_OK | MB_ICONERROR);
            if (g_hBtnInstall) EnableWindow(g_hBtnInstall, TRUE);
        }
        return;
    }

    if (g_hProgress) SendMessageW(g_hProgress, PBM_SETPOS, 65, 0);
    if (g_hStatusLabel) SetWindowTextW(g_hStatusLabel, L"正在注册 64 位 COM 资源管理器扩展与 3D 格式关联...");

    RegisterShellExtension(targetDir);

    if (g_hProgress) SendMessageW(g_hProgress, PBM_SETPOS, 80, 0);
    if (g_hStatusLabel) SetWindowTextW(g_hStatusLabel, L"正在创建快捷方式与配置系统环境...");

    std::wstring settingsExe = targetDir + L"\\ModelPeekSettings.exe";

    // Start Menu shortcut
    WCHAR startMenuDir[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, startMenuDir))) {
        std::wstring mpDir = std::wstring(startMenuDir) + L"\\ModelPeek";
        CreateDirectoryW(mpDir.c_str(), NULL);
        std::wstring lnk = mpDir + L"\\ModelPeek 控制中心.lnk";
        CreateShortcut(settingsExe.c_str(), lnk.c_str(), L"ModelPeek 3D/CAD 预览设置中心", settingsExe.c_str());
    }

    // Desktop shortcut
    WCHAR desktopDir[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, NULL, 0, desktopDir))) {
        std::wstring lnk = std::wstring(desktopDir) + L"\\ModelPeek 控制中心.lnk";
        CreateShortcut(settingsExe.c_str(), lnk.c_str(), L"ModelPeek 3D/CAD 预览设置中心", settingsExe.c_str());
    }

    RegisterUninstallEntry(targetDir);

    // Auto-unblock MOTW
    if (g_hProgress) SendMessageW(g_hProgress, PBM_SETPOS, 92, 0);
    if (g_hStatusLabel) SetWindowTextW(g_hStatusLabel, L"正在批量解除模型网络安全锁定 (Mark of the Web)...");
    UnblockCommonModelLocations(targetDir);

    if (g_hProgress) SendMessageW(g_hProgress, PBM_SETPOS, 100, 0);
    if (g_hStatusLabel) SetWindowTextW(g_hStatusLabel, L"🎉 安装成功！18 种格式立体缩略图已生效。");

    // Refresh Explorer shell
    system("taskkill /f /im prevhost.exe >nul 2>&1");
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

    if (!g_bSilent && hWnd) {
        int ret = MessageBoxW(hWnd, 
            L"🎉 ModelPeek v2.1.3 已成功安装并激活！\r\n\r\n"
            L"● 18 种 3D/CAD 格式立体缩略图已全面生效\r\n"
            L"● 已自动解除模型文件的网络安全锁定 (Mark of the Web 自愈)\r\n"
            L"● 内置轻量绿色 Python 运行时，纯净系统全格式通杀\r\n"
            L"● 支持 Windows 11 多标签页自愈与三维工程尺寸标注\r\n"
            L"● 双击文件保持原有专业软件关联，绝不破坏现有工作流\r\n\r\n"
            L"是否立即打开 ModelPeek 控制中心进行个性化配置？", 
            L"ModelPeek v2.1.3 安装完成", MB_YESNO | MB_ICONINFORMATION);

        if (ret == IDYES) {
            ShellExecuteW(NULL, L"open", settingsExe.c_str(), NULL, targetDir.c_str(), SW_SHOWNORMAL);
        }
        PostQuitMessage(0);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // Path input
        g_hPathEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", GetDefaultInstallDir().c_str(), 
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 42, 142, 430, 28, hWnd, NULL, GetModuleHandleW(NULL), NULL);
        SendMessageW(g_hPathEdit, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

        g_hBtnBrowse = CreateWindowExW(0, L"BUTTON", L"浏览...", 
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 482, 141, 95, 30, hWnd, (HMENU)1001, GetModuleHandleW(NULL), NULL);
        SendMessageW(g_hBtnBrowse, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

        // Checkboxes
        g_hChkShell = CreateWindowExW(0, L"BUTTON", L"激活 Windows 资源管理器 3D 缩略图与 Alt+P 交互预览窗格 (推荐)", 
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 42, 192, 530, 22, hWnd, NULL, GetModuleHandleW(NULL), NULL);
        SendMessageW(g_hChkShell, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        SendMessageW(g_hChkShell, BM_SETCHECK, BST_CHECKED, 0);

        g_hChkStartMenu = CreateWindowExW(0, L"BUTTON", L"在开始菜单创建快捷方式 (ModelPeek 控制中心)", 
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 42, 222, 530, 22, hWnd, NULL, GetModuleHandleW(NULL), NULL);
        SendMessageW(g_hChkStartMenu, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        SendMessageW(g_hChkStartMenu, BM_SETCHECK, BST_CHECKED, 0);

        g_hChkDesktop = CreateWindowExW(0, L"BUTTON", L"创建桌面快捷方式", 
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 42, 252, 530, 22, hWnd, NULL, GetModuleHandleW(NULL), NULL);
        SendMessageW(g_hChkDesktop, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        SendMessageW(g_hChkDesktop, BM_SETCHECK, BST_CHECKED, 0);

        // Progress bar
        g_hProgress = CreateWindowExW(0, PROGRESS_CLASSW, L"", 
            WS_CHILD | PBS_SMOOTH, 42, 315, 535, 14, hWnd, NULL, GetModuleHandleW(NULL), NULL);

        g_hStatusLabel = CreateWindowExW(0, L"STATIC", L"准备就绪，点击下方按钮开始安装", 
            WS_CHILD | WS_VISIBLE, 42, 335, 535, 18, hWnd, NULL, GetModuleHandleW(NULL), NULL);
        SendMessageW(g_hStatusLabel, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);

        // Owner-drawn Install Button
        g_hBtnInstall = CreateWindowExW(0, L"BUTTON", L"🚀 立即安装 ModelPeek v2.1.3", 
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 160, 368, 300, 42, hWnd, (HMENU)1000, GetModuleHandleW(NULL), NULL);
        SendMessageW(g_hBtnInstall, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT rcClient;
        GetClientRect(hWnd, &rcClient);

        // 1. Draw top hero banner card
        RECT rcTop = { 0, 0, rcClient.right, 92 };
        HBRUSH hBrTop = CreateSolidBrush(CLR_CARD_BG);
        FillRect(hdc, &rcTop, hBrTop);
        DeleteObject(hBrTop);

        // Bottom divider for top banner
        HPEN hPenDiv = CreatePen(PS_SOLID, 1, CLR_BORDER);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPenDiv);
        MoveToEx(hdc, 0, 92, NULL);
        LineTo(hdc, rcClient.right, 92);

        // Top banner title
        SelectObject(hdc, g_hFontTitle);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, CLR_TEXT_DARK);
        TextOutW(hdc, 32, 18, L"ModelPeek 3D/CAD 资源管理器扩展", 25);

        // Version badge pill: [v2.1.3]
        RECT rcBadge = { 395, 20, 460, 42 };
        HBRUSH hBrBadge = CreateSolidBrush(RGB(239, 246, 255)); // Light blue
        HPEN hPenBadge = CreatePen(PS_SOLID, 1, RGB(191, 219, 254));
        SelectObject(hdc, hBrBadge);
        SelectObject(hdc, hPenBadge);
        RoundRect(hdc, rcBadge.left, rcBadge.top, rcBadge.right, rcBadge.bottom, 10, 10);
        DeleteObject(hBrBadge);
        DeleteObject(hPenBadge);

        SelectObject(hdc, g_hFontSmall);
        SetTextColor(hdc, CLR_PRIMARY);
        DrawTextW(hdc, L"v2.1.3", -1, &rcBadge, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Subtitle
        SelectObject(hdc, g_hFontNormal);
        SetTextColor(hdc, CLR_TEXT_MUTED);
        TextOutW(hdc, 32, 54, L"赋予 Windows 文件夹工业 3D 缩略图与 Alt+P 全交互式视口预览能力", 41);

        // 2. Middle Content Card
        RECT rcCard = { 24, 108, rcClient.right - 24, 295 };
        HBRUSH hBrCard = CreateSolidBrush(CLR_CARD_BG);
        HPEN hPenCard = CreatePen(PS_SOLID, 1, CLR_BORDER);
        SelectObject(hdc, hBrCard);
        SelectObject(hdc, hPenCard);
        RoundRect(hdc, rcCard.left, rcCard.top, rcCard.right, rcCard.bottom, 12, 12);
        DeleteObject(hBrCard);
        DeleteObject(hPenCard);

        // Path Label
        SelectObject(hdc, g_hFontBold);
        SetTextColor(hdc, CLR_TEXT_DARK);
        TextOutW(hdc, 42, 120, L"📁 安装目标路径：", 9);

        // Feature / Security footer note in card
        SelectObject(hdc, g_hFontSmall);
        SetTextColor(hdc, CLR_TEXT_MUTED);
        TextOutW(hdc, 42, 275, L"🛡️ 零常驻守护进程 · 双击保留专业 CAD 原生关联 · 自动解除网络锁定", 43);

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
        if (pdis->CtlID == 1000) { // Install Button
            HDC hdc = pdis->hDC;
            RECT rc = pdis->rcItem;

            bool isPressed = (pdis->itemState & ODS_SELECTED) || g_bBtnPressed;
            COLORREF btnColor = isPressed ? CLR_PRIMARY_PRS : (g_bBtnHover ? CLR_PRIMARY_HOV : CLR_PRIMARY);

            HBRUSH hBrBtn = CreateSolidBrush(btnColor);
            HPEN hPenBtn = CreatePen(PS_SOLID, 1, btnColor);
            HBRUSH hOldBr = (HBRUSH)SelectObject(hdc, hBrBtn);
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPenBtn);

            RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 10, 10);

            SelectObject(hdc, g_hFontBold);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 255, 255));
            DrawTextW(hdc, L"🚀 立即安装 ModelPeek v2.1.3", -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

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
        if (id == 1000) { // Install button
            if (!IsRunAsAdmin()) {
                ElevateNow(hWnd);
                return 0;
            }
            DoInstallation(hWnd);
        } else if (id == 1001) { // Browse button
            BROWSEINFOW bi = {0};
            bi.hwndOwner = hWnd;
            bi.lpszTitle = L"选择 ModelPeek 安装目录";
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
            PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
            if (pidl) {
                WCHAR chosen[MAX_PATH];
                if (SHGetPathFromIDListW(pidl, chosen)) {
                    std::wstring fullPath = chosen;
                    if (fullPath.find(L"ModelPeek") == std::wstring::npos) {
                        fullPath += L"\\ModelPeek";
                    }
                    SetWindowTextW(g_hPathEdit, fullPath.c_str());
                }
                CoTaskMemFree(pidl);
            }
        }
        return 0;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        HWND hCtrl = (HWND)lParam;
        if (hCtrl == g_hStatusLabel) {
            SetTextColor(hdc, CLR_PRIMARY);
            SetBkColor(hdc, CLR_BG);
            return (INT_PTR)g_hBrushBg;
        }
        // Controls inside card (checkboxes etc)
        SetTextColor(hdc, CLR_TEXT_DARK);
        SetBkColor(hdc, CLR_CARD_BG);
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
    CoInitialize(NULL);

    LPWSTR cmdLine = GetCommandLineW();
    if (cmdLine) {
        std::wstring cmd(cmdLine);
        for (auto& c : cmd) c = towlower(c);
        if (cmd.find(L"/s") != std::wstring::npos || cmd.find(L"-s") != std::wstring::npos ||
            cmd.find(L"/silent") != std::wstring::npos || cmd.find(L"-silent") != std::wstring::npos) {
            g_bSilent = true;
        }
    }

    if (g_bSilent) {
        if (!IsRunAsAdmin()) {
            ElevateNow(NULL, L"/S");
            return 0;
        }
        DoInstallation(NULL);
        CoUninitialize();
        return 0;
    }

    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icex);

    g_hBrushBg = CreateSolidBrush(CLR_BG);
    g_hBrushCard = CreateSolidBrush(CLR_CARD_BG);
    g_hPenBorder = CreatePen(PS_SOLID, 1, CLR_BORDER);

    g_hFontTitle = CreateFontW(-20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
    g_hFontBold  = CreateFontW(-14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
    g_hFontNormal= CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
    g_hFontSmall = CreateFontW(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ModelPeekInstallerClass";
    wc.hbrBackground = g_hBrushBg;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    RegisterClassExW(&wc);

    int w = 620;
    int h = 460;
    int x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;

    HWND hWnd = CreateWindowExW(0, L"ModelPeekInstallerClass", L"ModelPeek 3D/CAD 预览扩展 v2.1.3 安装向导",
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
