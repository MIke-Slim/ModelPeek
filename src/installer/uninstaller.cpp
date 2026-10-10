#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "advapi32.lib")

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
        sei.nShow = SW_NORMAL;
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
        
        // Remove Thumbnail Provider shellex
        std::wstring thumbSub = extKey + L"\\ShellEx\\{e357fccd-a995-4576-b01f-234630154e96}";
        RegDeleteTreeW(HKEY_LOCAL_MACHINE, thumbSub.c_str());
        RegDeleteTreeW(HKEY_CURRENT_USER, thumbSub.c_str());
        RegDeleteTreeW(HKEY_CLASSES_ROOT, (std::wstring(ext) + L"\\ShellEx\\{e357fccd-a995-4576-b01f-234630154e96}").c_str());

        // Remove Preview Handler shellex
        std::wstring prevSub = extKey + L"\\ShellEx\\{8895b1c6-b41f-4c1c-a562-0d564250836f}";
        RegDeleteTreeW(HKEY_LOCAL_MACHINE, prevSub.c_str());
        RegDeleteTreeW(HKEY_CURRENT_USER, prevSub.c_str());
        RegDeleteTreeW(HKEY_CLASSES_ROOT, (std::wstring(ext) + L"\\ShellEx\\{8895b1c6-b41f-4c1c-a562-0d564250836f}").c_str());
    }
}

static void CleanShortcuts() {
    // 1. Common Start Menu
    WCHAR commonPrograms[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, commonPrograms))) {
        DeleteDirectoryRecursive(std::wstring(commonPrograms) + L"\\ModelPeek");
    }

    // 2. User Start Menu
    WCHAR userPrograms[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, 0, userPrograms))) {
        DeleteDirectoryRecursive(std::wstring(userPrograms) + L"\\ModelPeek");
    }

    // 3. Common Desktop
    WCHAR commonDesktop[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, NULL, 0, commonDesktop))) {
        DeleteFileW((std::wstring(commonDesktop) + L"\\ModelPeek 控制中心.lnk").c_str());
    }

    // 4. User Desktop
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
    // Step 1: Wait briefly for parent launcher to terminate
    Sleep(600);

    // Step 2: Terminate any related processes
    system("taskkill /f /im prevhost.exe >nul 2>&1");
    system("taskkill /f /im ModelPeekWorker.exe >nul 2>&1");
    system("taskkill /f /im ModelPeekSettings.exe >nul 2>&1");

    // Step 3: Unregister COM DLL if present
    std::wstring dllPath = targetDir + L"\\ModelPeekExtension.dll";
    if (PathFileExistsW(dllPath.c_str())) {
        std::wstring unregCmd = L"regsvr32.exe /u /s \"" + dllPath + L"\"";
        _wsystem(unregCmd.c_str());
    }

    // Step 4: Clean all Registry entries
    CleanRegistry();

    // Step 5: Clean all Shortcuts
    CleanShortcuts();

    // Step 6: Clean User Cache & Temp
    CleanUserData();

    // Step 7: Delete entire target install directory with retries
    for (int retry = 0; retry < 5; ++retry) {
        if (!PathFileExistsW(targetDir.c_str())) break;
        DeleteDirectoryRecursive(targetDir);
        if (!PathFileExistsW(targetDir.c_str())) break;
        Sleep(500);
    }

    // Step 8: Notify Explorer shell
    system("taskkill /f /im prevhost.exe >nul 2>&1");
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

    if (!silent) {
        MessageBoxW(NULL, 
            L"🎉 ModelPeek 已成功从您的计算机彻底卸载！\r\n\r\n"
            L"● 所有程序组件与安装目录已彻底删除\r\n"
            L"● 64 位 COM 资源管理器扩展与格式关联已完全注销\r\n"
            L"● 开始菜单及桌面快捷方式已清理干净\r\n"
            L"● 双击关联已完全恢复操作系统原生状态", 
            L"ModelPeek 卸载完成", MB_OK | MB_ICONINFORMATION);
    }
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

    // Check if running as temp worker: --worker <dir>
    size_t workerPos = lowerCmd.find(L"--worker");
    if (workerPos != std::wstring::npos) {
        // Extract targetDir from fullCmd
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

    // Normal launcher entry: Check Admin
    if (!IsRunAsAdmin()) {
        ElevateNow(cmdLine);
        return 0;
    }

    // Determine current directory as targetDir
    WCHAR selfExe[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, selfExe, MAX_PATH);
    PathRemoveFileSpecW(selfExe);
    std::wstring installDir = selfExe;

    // Confirm with user if not silent
    if (!silent) {
        int ret = MessageBoxW(NULL, 
            L"您确定要彻底卸载 ModelPeek 3D/CAD 资源管理器扩展吗？\r\n\r\n"
            L"这将移除全部 18 种 3D 格式的缩略图与 Alt+P 交互预览功能，并清理所有相关文件与快捷方式。", 
            L"ModelPeek 卸载确认", MB_YESNO | MB_ICONQUESTION);
        if (ret != IDYES) {
            CoUninitialize();
            return 0;
        }
    }

    // Self-delegation: Copy uninstaller to %TEMP% so the install directory is not locked!
    WCHAR tempPath[MAX_PATH] = {0};
    GetTempPathW(MAX_PATH, tempPath);
    std::wstring tempExe = std::wstring(tempPath) + L"ModelPeek_Uninstall_Worker.exe";

    WCHAR curExe[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, curExe, MAX_PATH);
    CopyFileW(curExe, tempExe.c_str(), FALSE);

    // Launch temp worker
    std::wstring workerCmd = L"\"" + tempExe + L"\" --worker \"" + installDir + L"\"" + (silent ? L" /S" : L"");
    std::vector<WCHAR> cmdVec(workerCmd.begin(), workerCmd.end());
    cmdVec.push_back(L'\0');

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = silent ? SW_HIDE : SW_NORMAL;

    if (CreateProcessW(NULL, cmdVec.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    CoUninitialize();
    return 0; // Current process terminates immediately, releasing locks!
}
