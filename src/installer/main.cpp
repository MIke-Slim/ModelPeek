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

#define IDR_PAYLOAD_ZIP 101

static HWND g_hPathEdit = NULL;
static HWND g_hChkShell = NULL;
static HWND g_hChkStartMenu = NULL;
static HWND g_hChkDesktop = NULL;
static HWND g_hBtnInstall = NULL;
static HWND g_hProgress = NULL;
static HWND g_hStatusLabel = NULL;
static HFONT g_hFontNormal = NULL;
static HFONT g_hFontBold = NULL;
static HFONT g_hFontHeader = NULL;

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

void ElevateNow(HWND hWnd) {
    WCHAR szPath[MAX_PATH];
    if (GetModuleFileNameW(NULL, szPath, MAX_PATH)) {
        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.lpVerb = L"runas";
        sei.lpFile = szPath;
        sei.hwnd = hWnd;
        sei.nShow = SW_NORMAL;
        if (ShellExecuteExW(&sei)) {
            ExitProcess(0);
        }
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

    // Create target dir
    CreateDirectoryW(targetDir.c_str(), NULL);

    // Expand archive via PowerShell
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
        LPCWSTR ver = L"2.0.0";
        LPCWSTR pub = L"ModelPeek Team";
        std::wstring uninst = targetDir + L"\\uninstall.bat";
        std::wstring icon = targetDir + L"\\ModelPeekSettings.exe,0";

        RegSetValueExW(hKey, L"DisplayName", 0, REG_SZ, (const BYTE*)name, (DWORD)((wcslen(name)+1)*sizeof(WCHAR)));
        RegSetValueExW(hKey, L"DisplayVersion", 0, REG_SZ, (const BYTE*)ver, (DWORD)((wcslen(ver)+1)*sizeof(WCHAR)));
        RegSetValueExW(hKey, L"Publisher", 0, REG_SZ, (const BYTE*)pub, (DWORD)((wcslen(pub)+1)*sizeof(WCHAR)));
        RegSetValueExW(hKey, L"InstallLocation", 0, REG_SZ, (const BYTE*)targetDir.c_str(), (DWORD)((targetDir.length()+1)*sizeof(WCHAR)));
        RegSetValueExW(hKey, L"UninstallString", 0, REG_SZ, (const BYTE*)uninst.c_str(), (DWORD)((uninst.length()+1)*sizeof(WCHAR)));
        RegSetValueExW(hKey, L"DisplayIcon", 0, REG_SZ, (const BYTE*)icon.c_str(), (DWORD)((icon.length()+1)*sizeof(WCHAR)));
        RegCloseKey(hKey);
    }
}

void DoInstallation(HWND hWnd) {
    WCHAR pathBuf[MAX_PATH] = {0};
    GetWindowTextW(g_hPathEdit, pathBuf, MAX_PATH);
    std::wstring targetDir = pathBuf;
    if (targetDir.empty()) targetDir = GetDefaultInstallDir();

    EnableWindow(g_hBtnInstall, FALSE);
    ShowWindow(g_hProgress, SW_SHOW);
    SendMessageW(g_hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

    SetWindowTextW(g_hStatusLabel, L"正在解压核心组件与 WebGL 视口引擎...");
    SendMessageW(g_hProgress, PBM_SETPOS, 30, 0);

    if (!ExtractPayloadToDirectory(targetDir)) {
        MessageBoxW(hWnd, L"解压安装组件失败，请检查目标路径权限或磁盘空间！", L"安装失败", MB_OK | MB_ICONERROR);
        EnableWindow(g_hBtnInstall, TRUE);
        return;
    }

    SendMessageW(g_hProgress, PBM_SETPOS, 60, 0);
    SetWindowTextW(g_hStatusLabel, L"正在注册 64 位 COM 资源管理器扩展与 3D 格式关联...");

    if (SendMessageW(g_hChkShell, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        RegisterShellExtension(targetDir);
    }

    SendMessageW(g_hProgress, PBM_SETPOS, 80, 0);
    SetWindowTextW(g_hStatusLabel, L"正在创建快捷方式与注册控制面板...");

    std::wstring settingsExe = targetDir + L"\\ModelPeekSettings.exe";

    // Start Menu shortcut
    if (SendMessageW(g_hChkStartMenu, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        WCHAR startMenuDir[MAX_PATH] = {0};
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, startMenuDir))) {
            std::wstring mpDir = std::wstring(startMenuDir) + L"\\ModelPeek";
            CreateDirectoryW(mpDir.c_str(), NULL);
            std::wstring lnk = mpDir + L"\\ModelPeek 控制中心.lnk";
            CreateShortcut(settingsExe.c_str(), lnk.c_str(), L"ModelPeek 3D/CAD 预览设置中心", settingsExe.c_str());
        }
    }

    // Desktop shortcut
    if (SendMessageW(g_hChkDesktop, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        WCHAR desktopDir[MAX_PATH] = {0};
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, NULL, 0, desktopDir))) {
            std::wstring lnk = std::wstring(desktopDir) + L"\\ModelPeek 控制中心.lnk";
            CreateShortcut(settingsExe.c_str(), lnk.c_str(), L"ModelPeek 3D/CAD 预览设置中心", settingsExe.c_str());
        }
    }

    RegisterUninstallEntry(targetDir);

    SendMessageW(g_hProgress, PBM_SETPOS, 100, 0);
    SetWindowTextW(g_hStatusLabel, L"安装完成！");

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

    int ret = MessageBoxW(hWnd, 
        L"🎉 ModelPeek v2.0 已成功安装并激活！\r\n\r\n"
        L"● 16 种 3D/CAD 格式缩略图与 Alt+P 视口已生效\r\n"
        L"● 双击文件保持原有专业软件关联，绝不破坏现有工作流\r\n\r\n"
        L"是否立即打开 ModelPeek 控制中心进行个性化配置？", 
        L"安装完成", MB_YESNO | MB_ICONINFORMATION);

    if (ret == IDYES) {
        ShellExecuteW(NULL, L"open", settingsExe.c_str(), NULL, targetDir.c_str(), SW_SHOWNORMAL);
    }

    PostQuitMessage(0);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // Banner header
        HWND hBanner = CreateWindowExW(0, L"STATIC", L"ModelPeek 3D/CAD 资源管理器扩展 v2.0", 
            WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 18, 560, 26, hWnd, NULL, GetModuleHandleW(NULL), NULL);
        SendMessageW(hBanner, WM_SETFONT, (WPARAM)g_hFontHeader, TRUE);

        HWND hSubBanner = CreateWindowExW(0, L"STATIC", L"让 Windows 文件夹秒变专业级工业 3D 看图工作台，16 种格式极速预览", 
            WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 48, 560, 20, hWnd, NULL, GetModuleHandleW(NULL), NULL);
        SendMessageW(hSubBanner, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

        // Path group
        HWND hLblPath = CreateWindowExW(0, L"STATIC", L"安装目标路径：", 
            WS_CHILD | WS_VISIBLE, 20, 85, 560, 18, hWnd, NULL, GetModuleHandleW(NULL), NULL);
        SendMessageW(hLblPath, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        g_hPathEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", GetDefaultInstallDir().c_str(), 
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 20, 108, 460, 26, hWnd, NULL, GetModuleHandleW(NULL), NULL);
        SendMessageW(g_hPathEdit, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

        HWND btnBrowse = CreateWindowExW(0, L"BUTTON", L"浏览...", 
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 490, 107, 85, 28, hWnd, (HMENU)1001, GetModuleHandleW(NULL), NULL);
        SendMessageW(btnBrowse, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

        // Options
        g_hChkShell = CreateWindowExW(0, L"BUTTON", L"激活 Windows 资源管理器 3D 缩略图与 Alt+P 交互预览窗格 (推荐)", 
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 20, 150, 560, 22, hWnd, NULL, GetModuleHandleW(NULL), NULL);
        SendMessageW(g_hChkShell, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        SendMessageW(g_hChkShell, BM_SETCHECK, BST_CHECKED, 0);

        g_hChkStartMenu = CreateWindowExW(0, L"BUTTON", L"在开始菜单创建快捷方式 (ModelPeek 控制中心)", 
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 20, 178, 560, 22, hWnd, NULL, GetModuleHandleW(NULL), NULL);
        SendMessageW(g_hChkStartMenu, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        SendMessageW(g_hChkStartMenu, BM_SETCHECK, BST_CHECKED, 0);

        g_hChkDesktop = CreateWindowExW(0, L"BUTTON", L"创建桌面快捷方式", 
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 20, 206, 560, 22, hWnd, NULL, GetModuleHandleW(NULL), NULL);
        SendMessageW(g_hChkDesktop, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        SendMessageW(g_hChkDesktop, BM_SETCHECK, BST_CHECKED, 0);

        // Progress bar
        g_hProgress = CreateWindowExW(0, PROGRESS_CLASSW, L"", 
            WS_CHILD | PBS_SMOOTH, 20, 240, 555, 18, hWnd, NULL, GetModuleHandleW(NULL), NULL);

        g_hStatusLabel = CreateWindowExW(0, L"STATIC", L"准备就绪，点击下方按钮开始安装", 
            WS_CHILD | WS_VISIBLE, 20, 265, 555, 18, hWnd, NULL, GetModuleHandleW(NULL), NULL);
        SendMessageW(g_hStatusLabel, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

        // Install button
        g_hBtnInstall = CreateWindowExW(0, L"BUTTON", L"🚀 立即安装 ModelPeek v2.0", 
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 180, 295, 230, 38, hWnd, (HMENU)1000, GetModuleHandleW(NULL), NULL);
        SendMessageW(g_hBtnInstall, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);
        return 0;
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
        SetTextColor(hdc, RGB(30, 41, 59));
        SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
        return (INT_PTR)GetSysColorBrush(COLOR_BTNFACE);
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    CoInitialize(NULL);
    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icex);

    g_hFontNormal = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
    g_hFontBold = CreateFontW(-13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
    g_hFontHeader = CreateFontW(-18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ModelPeekInstallerClass";
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    RegisterClassExW(&wc);

    HWND hWnd = CreateWindowExW(0, L"ModelPeekInstallerClass", L"ModelPeek 3D/CAD 预览插件 v2.0 安装程序",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 615, 390,
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
    if (g_hFontHeader) DeleteObject(g_hFontHeader);

    CoUninitialize();
    return (int)msg.wParam;
}
