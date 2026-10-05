#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <exdisp.h>
#include <shldisp.h>
#include <dwmapi.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstdint>

#include "../shell_ext/webview2/WebView2.h"
#include "../shell_ext/WebView2Callbacks.h"

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "dwmapi.lib")

#define TIMER_SELECTION_CHECK 1001
#define WM_QUICKLOOK_SHOW (WM_USER + 101)

static const LPCWSTR WND_CLASS_NAME = L"ModelPeekQuickLookWindow";
static const LPCWSTR MUTEX_NAME = L"ModelPeekPeek_Daemon_Mutex_v2";

static const std::vector<std::wstring> SUPPORTED_EXTS = {
    L".step", L".stp", L".stl", L".obj", L".fbx", L".glb", L".gltf", L".3mf",
    L".iges", L".igs", L".brep", L".brp", L".ply", L".pcd", L".dxf", L".gcode",
    L".dae", L".3ds"
};

// Global daemon state
static HINSTANCE g_hInstance = NULL;
static HWND g_hWnd = NULL;
static HHOOK g_hKeyboardHook = NULL;
static HWND g_hLastExplorer = NULL;
static std::wstring g_currentPath = L"";
static bool g_isNavigating = false;

// WebView2 state
static ICoreWebView2Controller* g_pController = nullptr;
static ICoreWebView2* g_pWebView = nullptr;
static bool g_bWebViewReady = false;
static std::wstring g_pendingLoadUrl = L"";

std::wstring GetAppDir() {
    WCHAR selfPath[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, selfPath, MAX_PATH);
    PathRemoveFileSpecW(selfPath);
    return selfPath;
}

std::wstring GetLocalLowDir() {
    WCHAR appData[MAX_PATH] = {0};
    if (SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appData) == S_OK) {
        std::wstring p = std::wstring(appData) + L"\\..\\LocalLow\\ModelPeek";
        CreateDirectoryW(p.c_str(), NULL);
        return p;
    }
    return L"C:\\Temp";
}

std::wstring UrlEncode(const std::wstring& value) {
    std::wostringstream escaped;
    escaped.fill(L'0');
    escaped << std::hex;
    for (WCHAR c : value) {
        if (isalnum((int)c) || c == L'-' || c == L'_' || c == L'.' || c == L'~' || c == L'/' || c == L':') {
            escaped << c;
        } else if (c == L'\\') {
            escaped << L'/';
        } else {
            escaped << L'%' << std::setw(2) << ((int)(unsigned char)c);
        }
    }
    return escaped.str();
}

bool IsSupported3DFile(const std::wstring& path) {
    if (path.empty()) return false;
    size_t dot = path.find_last_of(L'.');
    if (dot == std::wstring::npos) return false;
    std::wstring ext = path.substr(dot);
    for (auto& c : ext) c = towlower(c);
    for (const auto& sup : SUPPORTED_EXTS) {
        if (ext == sup) return true;
    }
    return false;
}

std::wstring PrepareModelForViewer(const std::wstring& filePath) {
    size_t dot = filePath.find_last_of(L'.');
    if (dot != std::wstring::npos) {
        std::wstring ext = filePath.substr(dot);
        for (auto& c : ext) c = towlower(c);

        bool isCadBrep = (ext == L".step" || ext == L".stp" || ext == L".iges" || ext == L".igs" || ext == L".brep" || ext == L".brp");
        if (isCadBrep) {
            std::wstring lowDir = GetLocalLowDir();
            std::wstring cacheDir = lowDir + L"\\cache\\cad_converted";
            CreateDirectoryW((lowDir + L"\\cache").c_str(), NULL);
            CreateDirectoryW(cacheDir.c_str(), NULL);

            uint64_t hash = 14695981039346656037ULL;
            for (WCHAR c : filePath) {
                hash ^= (uint64_t)towlower(c);
                hash *= 1099511628211ULL;
            }
            std::wstringstream ss;
            ss << cacheDir << L"\\" << std::hex << hash << L".stl";
            std::wstring cachedStl = ss.str();

            if (!PathFileExistsW(cachedStl.c_str())) {
                bool pipeDone = false;
                LPCWSTR pipeName = L"\\\\.\\pipe\\ModelPeekWorkerPipe";
                if (WaitNamedPipeW(pipeName, 25)) {
                    HANDLE hPipe = CreateFileW(pipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
                    if (hPipe != INVALID_HANDLE_VALUE) {
                        std::wstringstream req;
                        req << L"convert\t" << filePath << L"\t" << cachedStl;
                        std::wstring reqStr = req.str();
                        DWORD written = 0;
                        if (WriteFile(hPipe, reqStr.c_str(), (DWORD)(reqStr.length() * sizeof(WCHAR)), &written, NULL)) {
                            WCHAR resp[128] = {0};
                            DWORD read = 0;
                            ReadFile(hPipe, resp, sizeof(resp) - sizeof(WCHAR), &read, NULL);
                        }
                        CloseHandle(hPipe);
                        if (PathFileExistsW(cachedStl.c_str())) {
                            pipeDone = true;
                        }
                    }
                }

                if (!pipeDone) {
                    std::wstring workerExe = GetAppDir() + L"\\ModelPeekWorker.exe";
                    std::wstringstream cmd;
                    cmd << L"\"" << workerExe << L"\" convert \"" << filePath << L"\" \"" << cachedStl << L"\"";
                    STARTUPINFOW si = { sizeof(si) };
                    PROCESS_INFORMATION pi = { 0 };
                    si.dwFlags = STARTF_USESHOWWINDOW;
                    si.wShowWindow = SW_HIDE;
                    std::wstring cmdStr = cmd.str();
                    std::vector<WCHAR> cmdLine(cmdStr.begin(), cmdStr.end());
                    cmdLine.push_back(L'\0');

                    if (CreateProcessW(NULL, cmdLine.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                        WaitForSingleObject(pi.hProcess, 8000);
                        CloseHandle(pi.hProcess);
                        CloseHandle(pi.hThread);
                    }
                }
            }

            if (PathFileExistsW(cachedStl.c_str())) {
                return cachedStl;
            }
        }
    }
    return filePath;
}

bool GetExplorerSelectedItem(HWND hActive, std::wstring& outPath, HWND& outExplorerWnd) {
    outPath.clear();
    outExplorerWnd = NULL;
    if (!hActive) return false;

    WCHAR className[128] = {0};
    GetClassNameW(hActive, className, 128);

    HWND hTarget = hActive;
    while (hTarget) {
        GetClassNameW(hTarget, className, 128);
        if (_wcsicmp(className, L"CabinetWClass") == 0 ||
            _wcsicmp(className, L"Progman") == 0 ||
            _wcsicmp(className, L"WorkerW") == 0) {
            break;
        }
        hTarget = GetParent(hTarget);
    }

    if (!hTarget) return false;
    outExplorerWnd = hTarget;

    bool isDesktop = (_wcsicmp(className, L"Progman") == 0 || _wcsicmp(className, L"WorkerW") == 0);

    IShellWindows* psw = NULL;
    HRESULT hr = CoCreateInstance(CLSID_ShellWindows, NULL, CLSCTX_ALL, IID_IShellWindows, (void**)&psw);
    if (FAILED(hr) || !psw) return false;

    long count = 0;
    psw->get_Count(&count);
    bool found = false;

    for (long i = 0; i < count; ++i) {
        VARIANT vi;
        VariantInit(&vi);
        vi.vt = VT_I4;
        vi.lVal = i;

        IDispatch* pdisp = NULL;
        if (SUCCEEDED(psw->Item(vi, &pdisp)) && pdisp) {
            IWebBrowserApp* pwba = NULL;
            if (SUCCEEDED(pdisp->QueryInterface(IID_IWebBrowserApp, (void**)&pwba)) && pwba) {
                HWND hwndBrowser = NULL;
                pwba->get_HWND((LONG_PTR*)&hwndBrowser);
                if (hwndBrowser == hTarget || IsChild(hTarget, hwndBrowser) || IsChild(hwndBrowser, hTarget)) {
                    IDispatch* pdoc = NULL;
                    if (SUCCEEDED(pwba->get_Document(&pdoc)) && pdoc) {
                        IShellFolderViewDual* pFolderView = NULL;
                        if (SUCCEEDED(pdoc->QueryInterface(IID_IShellFolderViewDual, (void**)&pFolderView)) && pFolderView) {
                            FolderItems* pItems = NULL;
                            if (SUCCEEDED(pFolderView->SelectedItems(&pItems)) && pItems) {
                                long selCount = 0;
                                pItems->get_Count(&selCount);
                                if (selCount > 0) {
                                    VARIANT vIdx;
                                    VariantInit(&vIdx);
                                    vIdx.vt = VT_I4;
                                    vIdx.lVal = 0;
                                    FolderItem* pItem = NULL;
                                    if (SUCCEEDED(pItems->Item(vIdx, &pItem)) && pItem) {
                                        BSTR bstr = NULL;
                                        if (SUCCEEDED(pItem->get_Path(&bstr)) && bstr) {
                                            outPath = bstr;
                                            SysFreeString(bstr);
                                            found = true;
                                        }
                                        pItem->Release();
                                    }
                                }
                                pItems->Release();
                            }
                            pFolderView->Release();
                        }
                        pdoc->Release();
                    }
                }
                pwba->Release();
            }
            pdisp->Release();
        }
        if (found) break;
    }

    if (!found && isDesktop) {
        VARIANT vEmpty;
        VariantInit(&vEmpty);
        LONG hwndDesk = 0;
        IDispatch* pdispDesk = NULL;
        if (SUCCEEDED(psw->FindWindowSW(&vEmpty, &vEmpty, SWC_DESKTOP, &hwndDesk, SWFO_NEEDDISPATCH, &pdispDesk)) && pdispDesk) {
            IWebBrowserApp* pwba = NULL;
            if (SUCCEEDED(pdispDesk->QueryInterface(IID_IWebBrowserApp, (void**)&pwba)) && pwba) {
                IDispatch* pdoc = NULL;
                if (SUCCEEDED(pwba->get_Document(&pdoc)) && pdoc) {
                    IShellFolderViewDual* pFolderView = NULL;
                    if (SUCCEEDED(pdoc->QueryInterface(IID_IShellFolderViewDual, (void**)&pFolderView)) && pFolderView) {
                        FolderItems* pItems = NULL;
                        if (SUCCEEDED(pFolderView->SelectedItems(&pItems)) && pItems) {
                            long selCount = 0;
                            pItems->get_Count(&selCount);
                            if (selCount > 0) {
                                VARIANT vIdx;
                                VariantInit(&vIdx);
                                vIdx.vt = VT_I4;
                                vIdx.lVal = 0;
                                FolderItem* pItem = NULL;
                                if (SUCCEEDED(pItems->Item(vIdx, &pItem)) && pItem) {
                                    BSTR bstr = NULL;
                                    if (SUCCEEDED(pItem->get_Path(&bstr)) && bstr) {
                                        outPath = bstr;
                                        SysFreeString(bstr);
                                        found = true;
                                    }
                                    pItem->Release();
                                }
                            }
                            pItems->Release();
                        }
                        pFolderView->Release();
                    }
                    pdoc->Release();
                }
                pwba->Release();
            }
            pdispDesk->Release();
        }
    }

    psw->Release();
    return found;
}

void LoadModelInWebView(const std::wstring& filePath) {
    if (filePath.empty()) return;
    std::wstring displayModel = PrepareModelForViewer(filePath);

    std::wstring appDir = GetAppDir();
    std::wstring htmlPath = appDir + L"\\viewer\\index.html";
    if (!PathFileExistsW(htmlPath.c_str())) {
        htmlPath = appDir + L"\\..\\src\\viewer\\index.html";
    }

    std::wstring encodedModel = UrlEncode(displayModel);
    std::wstring fileUrl = L"file:///" + UrlEncode(htmlPath) + L"?file=" + encodedModel + L"&quicklook=1";

    if (g_bWebViewReady && g_pWebView) {
        g_pWebView->Navigate(fileUrl.c_str());
    } else {
        g_pendingLoadUrl = fileUrl;
    }

    // Update window title
    std::wstring fileName = filePath;
    size_t slash = fileName.find_last_of(L"\\/");
    if (slash != std::wstring::npos) fileName = fileName.substr(slash + 1);

    std::wstring title = L"ModelPeek QuickLook - " + fileName;
    SetWindowTextW(g_hWnd, title.c_str());
}

void ShowQuickLookWindow(const std::wstring& filePath, HWND hExplorer) {
    g_currentPath = filePath;
    g_hLastExplorer = hExplorer;

    // Center window over current monitor
    HMONITOR hMon = MonitorFromWindow(hExplorer ? hExplorer : GetDesktopWindow(), MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfoW(hMon, &mi);

    int w = 1040;
    int h = 720;
    int x = mi.rcWork.left + (mi.rcWork.right - mi.rcWork.left - w) / 2;
    int y = mi.rcWork.top + (mi.rcWork.bottom - mi.rcWork.top - h) / 2;

    SetWindowPos(g_hWnd, HWND_TOP, x, y, w, h, SWP_SHOWWINDOW);
    ShowWindow(g_hWnd, SW_SHOW);
    SetForegroundWindow(g_hWnd);

    LoadModelInWebView(filePath);
}

void InitWebView2() {
    std::wstring lowDir = GetLocalLowDir();
    std::wstring userDataFolder = lowDir + L"\\quicklook_webview_data";
    CreateDirectoryW(userDataFolder.c_str(), NULL);

    SetEnvironmentVariableW(L"WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS", L"--allow-file-access-from-files --disable-web-security");

    auto onEnvCreated = [](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
        if (FAILED(result) || !env) return result;

        auto onControllerCreated = [](HRESULT res, ICoreWebView2Controller* controller) -> HRESULT {
            if (FAILED(res) || !controller) return res;

            g_pController = controller;
            g_pController->AddRef();

            RECT rc;
            GetClientRect(g_hWnd, &rc);
            g_pController->put_Bounds(rc);
            g_pController->put_IsVisible(TRUE);

            g_pController->get_CoreWebView2(&g_pWebView);
            if (g_pWebView) {
                g_pWebView->AddRef();
                ICoreWebView2Settings* settings = nullptr;
                if (SUCCEEDED(g_pWebView->get_Settings(&settings)) && settings) {
                    settings->put_IsScriptEnabled(TRUE);
                    settings->put_AreDefaultContextMenusEnabled(FALSE);
                    settings->put_IsStatusBarEnabled(FALSE);
                    settings->put_AreDevToolsEnabled(FALSE);
                    settings->Release();
                }

                g_bWebViewReady = true;
                if (!g_pendingLoadUrl.empty()) {
                    g_pWebView->Navigate(g_pendingLoadUrl.c_str());
                    g_pendingLoadUrl.clear();
                }
            }
            return S_OK;
        };

        return env->CreateCoreWebView2Controller(
            g_hWnd,
            MakeControllerHandler(onControllerCreated)
        );
    };

    CreateCoreWebView2EnvironmentWithOptions(
        nullptr,
        userDataFolder.c_str(),
        nullptr,
        MakeEnvHandler(onEnvCreated)
    );
}

LRESULT CALLBACK QuickLookWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_SIZE: {
        if (g_pController) {
            RECT rc;
            GetClientRect(hWnd, &rc);
            g_pController->put_Bounds(rc);
        }
        return 0;
    }
    case WM_TIMER: {
        if (wParam == TIMER_SELECTION_CHECK) {
            KillTimer(hWnd, TIMER_SELECTION_CHECK);
            g_isNavigating = false;
            std::wstring newPath;
            HWND hExp = NULL;
            if (GetExplorerSelectedItem(g_hLastExplorer, newPath, hExp)) {
                if (IsSupported3DFile(newPath) && newPath != g_currentPath) {
                    g_currentPath = newPath;
                    LoadModelInWebView(newPath);
                }
            }
            return 0;
        }
        break;
    }
    case WM_CLOSE: {
        ShowWindow(hWnd, SW_HIDE);
        return 0;
    }
    case WM_DESTROY: {
        if (g_pController) {
            g_pController->Close();
            g_pController->Release();
            g_pController = nullptr;
        }
        if (g_pWebView) {
            g_pWebView->Release();
            g_pWebView = nullptr;
        }
        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* pKbd = (KBDLLHOOKSTRUCT*)lParam;
        bool isKeyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);

        if (isKeyDown) {
            // 1. Check Spacebar
            if (pKbd->vkCode == VK_SPACE) {
                if (IsWindowVisible(g_hWnd)) {
                    ShowWindow(g_hWnd, SW_HIDE);
                    return 1; // Handled, close QuickLook
                } else {
                    HWND hFg = GetForegroundWindow();
                    if (hFg) {
                        // Check if focused in an edit control (search box or file rename)
                        GUITHREADINFO gti = { sizeof(gti) };
                        if (GetGUIThreadInfo(0, &gti) && gti.hwndFocus) {
                            WCHAR focusClass[128] = {0};
                            GetClassNameW(gti.hwndFocus, focusClass, 128);
                            if (_wcsicmp(focusClass, L"Edit") == 0 ||
                                _wcsicmp(focusClass, L"SearchEditBox") == 0 ||
                                wcsstr(focusClass, L"Edit") != NULL) {
                                return CallNextHookEx(NULL, nCode, wParam, lParam);
                            }
                        }

                        std::wstring selPath;
                        HWND hExplorer = NULL;
                        if (GetExplorerSelectedItem(hFg, selPath, hExplorer)) {
                            if (IsSupported3DFile(selPath)) {
                                ShowQuickLookWindow(selPath, hExplorer);
                                return 1; // Suppress space in Explorer
                            }
                        }
                    }
                }
            }
            // 2. Check ESC key
            else if (pKbd->vkCode == VK_ESCAPE) {
                if (IsWindowVisible(g_hWnd)) {
                    ShowWindow(g_hWnd, SW_HIDE);
                    return 1;
                }
            }
            // 3. Arrow Keys navigation when QuickLook is open
            else if (pKbd->vkCode == VK_UP || pKbd->vkCode == VK_DOWN ||
                     pKbd->vkCode == VK_LEFT || pKbd->vkCode == VK_RIGHT) {
                if (IsWindowVisible(g_hWnd) && g_hLastExplorer && IsWindow(g_hLastExplorer)) {
                    if (!g_isNavigating) {
                        g_isNavigating = true;
                        PostMessageW(g_hLastExplorer, WM_KEYDOWN, pKbd->vkCode, pKbd->scanCode << 16);
                        PostMessageW(g_hLastExplorer, WM_KEYUP, pKbd->vkCode, (pKbd->scanCode << 16) | 0xC0000000);
                        SetTimer(g_hWnd, TIMER_SELECTION_CHECK, 70, NULL);
                        return 1;
                    }
                }
            }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPWSTR lpCmdLine, int /*nCmdShow*/) {
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    // Command-line options
    if (lpCmdLine && (wcsstr(lpCmdLine, L"--quit") || wcsstr(lpCmdLine, L"--stop"))) {
        HWND hExist = FindWindowW(WND_CLASS_NAME, NULL);
        if (hExist) {
            PostMessageW(hExist, WM_CLOSE, 0, 0);
            PostMessageW(hExist, WM_DESTROY, 0, 0);
        }
        return 0;
    }

    if (lpCmdLine && wcsstr(lpCmdLine, L"--status")) {
        HWND hExist = FindWindowW(WND_CLASS_NAME, NULL);
        return hExist ? 0 : 1;
    }

    HANDLE hMutex = CreateMutexW(NULL, TRUE, MUTEX_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (hMutex) CloseHandle(hMutex);
        return 0;
    }

    g_hInstance = hInstance;

    // Register Window Class
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = QuickLookWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = WND_CLASS_NAME;
    RegisterClassExW(&wc);

    // Create Hidden QuickLook Window initially
    g_hWnd = CreateWindowExW(
        WS_EX_TOPMOST,
        WND_CLASS_NAME,
        L"ModelPeek QuickLook",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 1040, 720,
        NULL, NULL, hInstance, NULL
    );

    // Enable Modern Windows Immersive Dark Mode for Title Bar
    BOOL dark = TRUE;
    DwmSetWindowAttribute(g_hWnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &dark, sizeof(dark));

    // Initialize WebView2
    InitWebView2();

    // Install Low-Level Keyboard Hook
    g_hKeyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);

    // Message Loop
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (g_hKeyboardHook) {
        UnhookWindowsHookEx(g_hKeyboardHook);
        g_hKeyboardHook = NULL;
    }

    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }

    CoUninitialize();
    return (int)msg.wParam;
}
