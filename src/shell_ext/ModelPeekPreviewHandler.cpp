#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include "ModelPeekPreviewHandler.h"
#include "Guids.h"
#include "WebView2Callbacks.h"
#include "Logger.h"
#include <shlwapi.h>
#include <shlobj.h>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cstdint>

extern HINSTANCE g_hInst;
extern long g_serverLocks;

static const WCHAR* PREVIEW_WND_CLASS = L"ModelPeekPreviewHostClass";

static std::wstring UrlEncode(const std::wstring& value) {
    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), (int)value.length(), NULL, 0, NULL, NULL);
    if (utf8Len <= 0) return value;
    std::string utf8Str(utf8Len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), (int)value.length(), &utf8Str[0], utf8Len, NULL, NULL);

    std::wostringstream escaped;
    escaped.fill(L'0');
    for (unsigned char c : utf8Str) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '~' || c == '/' || c == ':') {
            escaped << (WCHAR)c;
        } else if (c == '\\') {
            escaped << L'/';
        } else {
            escaped << L'%' << std::uppercase << std::hex << std::setw(2) << (int)c;
        }
    }
    return escaped.str();
}

static std::wstring DumpStreamToTemp(IStream* pStream, LPCWSTR origName) {
    std::wstring ext = L".stl";
    if (origName) {
        LPCWSTR dot = wcsrchr(origName, L'.');
        if (dot) ext = dot;
    }
    WCHAR tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    WCHAR tempFile[MAX_PATH];
    GetTempFileNameW(tempPath, L"mpk", 0, tempFile);
    std::wstring finalTemp = std::wstring(tempFile) + ext;
    MoveFileW(tempFile, finalTemp.c_str());

    FILE* f = _wfopen(finalTemp.c_str(), L"wb");
    if (!f) return L"";

    LARGE_INTEGER liZero = {0};
    pStream->Seek(liZero, STREAM_SEEK_SET, NULL);

    char buf[16384];
    ULONG bytesRead = 0;
    while (SUCCEEDED(pStream->Read(buf, sizeof(buf), &bytesRead)) && bytesRead > 0) {
        fwrite(buf, 1, bytesRead, f);
    }
    fclose(f);
    return finalTemp;
}

ModelPeekPreviewHandler::ModelPeekPreviewHandler() 
    : m_refCount(1), m_hwndParent(NULL), m_hwndPreview(NULL),
      m_punkSite(nullptr), m_controller(nullptr), m_webview(nullptr) {
    ZeroMemory(&m_rcParent, sizeof(m_rcParent));
    LogTrace(L"ModelPeekPreviewHandler created");
    InterlockedIncrement(&g_serverLocks);
}

ModelPeekPreviewHandler::~ModelPeekPreviewHandler() {
    LogTrace(L"ModelPeekPreviewHandler destroyed");
    Unload();
    if (m_punkSite) {
        m_punkSite->Release();
        m_punkSite = nullptr;
    }
    InterlockedDecrement(&g_serverLocks);
}

STDMETHODIMP ModelPeekPreviewHandler::QueryInterface(REFIID riid, void **ppv) {
    if (!ppv) return E_POINTER;
    if (riid == IID_IUnknown || riid == __uuidof(IPreviewHandler)) {
        *ppv = static_cast<IPreviewHandler*>(this);
    } else if (riid == __uuidof(IInitializeWithFile)) {
        *ppv = static_cast<IInitializeWithFile*>(this);
    } else if (riid == __uuidof(IInitializeWithItem)) {
        *ppv = static_cast<IInitializeWithItem*>(this);
    } else if (riid == __uuidof(IInitializeWithStream)) {
        *ppv = static_cast<IInitializeWithStream*>(this);
    } else if (riid == __uuidof(IObjectWithSite)) {
        *ppv = static_cast<IObjectWithSite*>(this);
    } else if (riid == __uuidof(IOleWindow)) {
        *ppv = static_cast<IOleWindow*>(this);
    } else if (riid == __uuidof(IPreviewHandlerVisuals)) {
        *ppv = static_cast<IPreviewHandlerVisuals*>(this);
    } else {
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) ModelPeekPreviewHandler::AddRef() {
    return InterlockedIncrement(&m_refCount);
}

STDMETHODIMP_(ULONG) ModelPeekPreviewHandler::Release() {
    ULONG count = InterlockedDecrement(&m_refCount);
    if (count == 0) delete this;
    return count;
}

STDMETHODIMP ModelPeekPreviewHandler::Initialize(LPCWSTR pszFilePath, DWORD /*grfMode*/) {
    LogTrace(std::wstring(L"PreviewHandler::Initialize(File): ") + (pszFilePath ? pszFilePath : L"null"));
    if (!pszFilePath) return E_INVALIDARG;
    m_filePath = pszFilePath;
    return S_OK;
}

STDMETHODIMP ModelPeekPreviewHandler::Initialize(IShellItem *psi, DWORD /*grfMode*/) {
    LogTrace(L"PreviewHandler::Initialize(IShellItem*)");
    if (!psi) return E_INVALIDARG;
    LPWSTR pszPath = nullptr;
    HRESULT hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
    if (SUCCEEDED(hr) && pszPath) {
        m_filePath = pszPath;
        LogTrace(std::wstring(L"PreviewHandler::Initialize(IShellItem) got path: ") + m_filePath);
        CoTaskMemFree(pszPath);
        return S_OK;
    }
    return hr;
}

STDMETHODIMP ModelPeekPreviewHandler::Initialize(IStream *pStream, DWORD /*grfMode*/) {
    LogTrace(L"PreviewHandler::Initialize(IStream*)");
    if (!pStream) return E_INVALIDARG;
    STATSTG stat = {0};
    if (SUCCEEDED(pStream->Stat(&stat, STATFLAG_DEFAULT)) && stat.pwcsName) {
        if (PathFileExistsW(stat.pwcsName)) {
            m_filePath = stat.pwcsName;
            LogTrace(std::wstring(L"PreviewHandler: from stream Stat exists: ") + m_filePath);
            CoTaskMemFree(stat.pwcsName);
            return S_OK;
        } else {
            m_filePath = DumpStreamToTemp(pStream, stat.pwcsName);
            LogTrace(std::wstring(L"PreviewHandler: stream dumped to temp: ") + m_filePath);
            CoTaskMemFree(stat.pwcsName);
            return m_filePath.empty() ? E_FAIL : S_OK;
        }
    }
    m_filePath = DumpStreamToTemp(pStream, L"model.stl");
    return m_filePath.empty() ? E_FAIL : S_OK;
}

STDMETHODIMP ModelPeekPreviewHandler::SetSite(IUnknown *pUnkSite) {
    LogTrace(L"PreviewHandler::SetSite called");
    if (m_punkSite) {
        m_punkSite->Release();
        m_punkSite = nullptr;
    }
    m_punkSite = pUnkSite;
    if (m_punkSite) {
        m_punkSite->AddRef();
    }
    return S_OK;
}

STDMETHODIMP ModelPeekPreviewHandler::GetSite(REFIID riid, void **ppvSite) {
    LogTrace(L"PreviewHandler::GetSite called");
    if (!ppvSite) return E_POINTER;
    *ppvSite = nullptr;
    if (!m_punkSite) return E_FAIL;
    return m_punkSite->QueryInterface(riid, ppvSite);
}

STDMETHODIMP ModelPeekPreviewHandler::SetWindow(HWND hwnd, const RECT *prc) {
    if (!hwnd || !prc) return E_INVALIDARG;
    m_hwndParent = hwnd;
    m_rcParent = *prc;
    std::wstringstream ss;
    ss << L"PreviewHandler::SetWindow: hwnd=" << (UINT_PTR)hwnd 
       << L" rc={" << prc->left << L"," << prc->top << L"," << prc->right << L"," << prc->bottom << L"}";
    LogTrace(ss.str());
    if (m_hwndPreview) {
        SetParent(m_hwndPreview, m_hwndParent);
        SetRect(prc);
    }
    return S_OK;
}

STDMETHODIMP ModelPeekPreviewHandler::SetRect(const RECT *prc) {
    if (!prc) return E_INVALIDARG;
    m_rcParent = *prc;
    int w = m_rcParent.right - m_rcParent.left;
    int h = m_rcParent.bottom - m_rcParent.top;
    if (w <= 0) w = 200;
    if (h <= 0) h = 200;

    std::wstringstream ss;
    ss << L"PreviewHandler::SetRect: w=" << w << L" h=" << h;
    LogTrace(ss.str());

    if (m_hwndPreview) {
        SetWindowPos(m_hwndPreview, NULL, 0, 0, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (m_controller) {
        RECT bounds = { 0, 0, w, h };
        m_controller->put_Bounds(bounds);
    }
    return S_OK;
}

LRESULT CALLBACK ModelPeekPreviewHandler::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_SIZE: {
            ModelPeekPreviewHandler* pThis = (ModelPeekPreviewHandler*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
            if (pThis && pThis->m_controller) {
                RECT rc;
                GetClientRect(hwnd, &rc);
                pThis->m_controller->put_Bounds(rc);
            }
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

bool ModelPeekPreviewHandler::CreateChildWindow() {
    LogTrace(L"PreviewHandler::CreateChildWindow starting...");
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = g_hInst;
    wc.lpszClassName = PREVIEW_WND_CLASS;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    if (!RegisterClassExW(&wc)) {
        DWORD regErr = GetLastError();
        if (regErr != ERROR_CLASS_ALREADY_EXISTS) {
            LogTrace(L"RegisterClassExW err=" + std::to_wstring(regErr));
        }
    }

    int w = m_rcParent.right - m_rcParent.left;
    int h = m_rcParent.bottom - m_rcParent.top;
    if (w <= 0) w = 300;
    if (h <= 0) h = 300;

    std::wstringstream ss;
    ss << L"CreateWindowExW: parent=" << (UINT_PTR)m_hwndParent
       << L" IsWindow=" << IsWindow(m_hwndParent)
       << L" w=" << w << L" h=" << h
       << L" g_hInst=" << (UINT_PTR)g_hInst;
    LogTrace(ss.str());

    // 1. Create preview window without parent first to avoid cross-process CreateWindowEx error 1400
    m_hwndPreview = CreateWindowExW(
        WS_EX_NOPARENTNOTIFY, PREVIEW_WND_CLASS, L"ModelPeekPreview",
        WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        0, 0, w, h,
        NULL, NULL, g_hInst, NULL
    );

    if (m_hwndPreview) {
        SetWindowLongPtrW(m_hwndPreview, GWLP_USERDATA, (LONG_PTR)this);
        // 2. Attach to host parent window across processes using SetParent
        if (m_hwndParent) {
            SetWindowLongPtrW(m_hwndPreview, GWL_STYLE, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
            SetParent(m_hwndPreview, m_hwndParent);
            SetWindowPos(m_hwndPreview, NULL, 0, 0, w, h, SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        }
        ShowWindow(m_hwndPreview, SW_SHOW);
        LogTrace(L"PreviewHandler::CreateChildWindow created HWND successfully");
        return true;
    }
    DWORD err = GetLastError();
    LogTrace(L"PreviewHandler::CreateChildWindow CreateWindowExW FAILED err=" + std::to_wstring(err));
    return false;
}

std::wstring ModelPeekPreviewHandler::GetViewerHtmlPath() {
    WCHAR modulePath[MAX_PATH];
    GetModuleFileNameW(g_hInst, modulePath, MAX_PATH);
    PathRemoveFileSpecW(modulePath);

    std::wstring viewerPath = std::wstring(modulePath) + L"\\viewer\\index.html";
    if (!PathFileExistsW(viewerPath.c_str())) {
        viewerPath = std::wstring(modulePath) + L"\\..\\src\\viewer\\index.html";
    }
    LogTrace(L"Viewer HTML path: " + viewerPath + L" exists=" + std::to_wstring(PathFileExistsW(viewerPath.c_str())));
    return viewerPath;
}

std::wstring ModelPeekPreviewHandler::PrepareModelForPreview(const std::wstring& filePath) {
    std::wstring ext;
    size_t dot = filePath.find_last_of(L'.');
    if (dot != std::wstring::npos) {
        ext = filePath.substr(dot);
        for (auto& c : ext) c = towlower(c);
    }

    bool isCadBrep = (ext == L".step" || ext == L".stp" || ext == L".iges" || ext == L".igs" || ext == L".brep" || ext == L".brp");
    if (isCadBrep) {
        std::wstring lowDir = GetLocalLowDir();
        if (!lowDir.empty()) {
            std::wstring cacheDir = lowDir + L"\\cache\\preview_mesh";
            CreateDirectoryW((lowDir + L"\\cache").c_str(), NULL);
            CreateDirectoryW(cacheDir.c_str(), NULL);

            uint64_t hash = 14695981039346656037ULL;
            for (WCHAR c : filePath) {
                hash ^= (uint64_t)(towlower(c));
                hash *= 1099511628211ULL;
            }
            std::wstringstream ss;
            ss << cacheDir << L"\\" << std::hex << hash << L".stl";
            std::wstring cachedStl = ss.str();

            if (!PathFileExistsW(cachedStl.c_str())) {
                bool pipeConverted = false;
                LPCWSTR pipeName = L"\\\\.\\pipe\\ModelPeekWorkerPipe";
                if (WaitNamedPipeW(pipeName, 20)) {
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
                            pipeConverted = true;
                            LogTrace(L"PreviewHandler: CAD converted via Daemon Named Pipe successfully!");
                        }
                    }
                }

                if (!pipeConverted) {
                    WCHAR modulePath[MAX_PATH];
                    GetModuleFileNameW(g_hInst, modulePath, MAX_PATH);
                    PathRemoveFileSpecW(modulePath);
                    std::wstring workerExe = std::wstring(modulePath) + L"\\ModelPeekWorker.exe";

                    std::wstringstream cmd;
                    cmd << L"\"" << workerExe << L"\" convert \"" << filePath << L"\" \"" << cachedStl << L"\"";

                    STARTUPINFOW si = { sizeof(si) };
                    PROCESS_INFORMATION pi = { 0 };
                    si.dwFlags = STARTF_USESHOWWINDOW;
                    si.wShowWindow = SW_HIDE;
                    std::wstring cmdStr = cmd.str();
                    LogTrace(std::wstring(L"PreviewHandler executing worker conversion: ") + cmdStr);
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
                LogTrace(L"PreviewHandler: CAD converted successfully to: " + cachedStl);
                return cachedStl;
            } else {
                LogTrace(L"PreviewHandler: CAD conversion FAILED to produce: " + cachedStl);
            }
        }
    }
    return filePath;
}

bool ModelPeekPreviewHandler::InitWebView2() {
    LogTrace(L"PreviewHandler::InitWebView2 starting...");
    std::wstring lowDir = GetLocalLowDir();
    std::wstring userDataFolder = lowDir + L"\\webview_data";
    CreateDirectoryW(userDataFolder.c_str(), NULL);
    LogTrace(L"PreviewHandler: using userDataFolder=" + userDataFolder);

    SetEnvironmentVariableW(L"WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS", L"--allow-file-access-from-files --disable-web-security");

    auto onEnvCreated = [this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
        LogTrace(L"onEnvCreated callback hr=" + std::to_wstring(result));
        if (FAILED(result) || !env) {
            LogTrace(L"onEnvCreated FAILED!");
            return result;
        }

        auto onControllerCreated = [this](HRESULT res, ICoreWebView2Controller* controller) -> HRESULT {
            LogTrace(L"onControllerCreated callback res=" + std::to_wstring(res));
            if (FAILED(res) || !controller) return res;

            m_controller = controller;
            m_controller->AddRef();
            m_controller->get_CoreWebView2(&m_webview);

            RECT clientRc = { 0 };
            GetClientRect(m_hwndPreview, &clientRc);
            if (clientRc.right <= 0 || clientRc.bottom <= 0) {
                clientRc.right = m_rcParent.right - m_rcParent.left;
                clientRc.bottom = m_rcParent.bottom - m_rcParent.top;
            }
            LogTrace(L"Setting WebView2 bounds: " + std::to_wstring(clientRc.right) + L"x" + std::to_wstring(clientRc.bottom));
            m_controller->put_Bounds(clientRc);
            m_controller->put_IsVisible(TRUE);

            ICoreWebView2Controller2* ctrl2 = nullptr;
            if (SUCCEEDED(m_controller->QueryInterface(IID_ICoreWebView2Controller2, (void**)&ctrl2)) && ctrl2) {
                COREWEBVIEW2_COLOR darkBg = { 255, 30, 34, 43 };
                ctrl2->put_DefaultBackgroundColor(darkBg);
                ctrl2->Release();
            }

            ICoreWebView2Settings* settings = nullptr;
            if (m_webview && SUCCEEDED(m_webview->get_Settings(&settings)) && settings) {
                settings->put_IsStatusBarEnabled(FALSE);
                settings->put_AreDefaultContextMenusEnabled(FALSE);
                settings->put_AreDevToolsEnabled(FALSE);
                settings->Release();
            }

            std::wstring targetModel = PrepareModelForPreview(m_filePath);
            std::wstring viewerPath = GetViewerHtmlPath();

            std::wstring url = L"file:///" + UrlEncode(viewerPath) + L"?file=" + UrlEncode(targetModel);

            LogTrace(L"Navigating to URL: " + url);
            if (m_webview) {
                m_webview->Navigate(url.c_str());
            }
            return S_OK;
        };

        return env->CreateCoreWebView2Controller(m_hwndPreview, MakeControllerHandler(onControllerCreated));
    };

    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
        nullptr, userDataFolder.c_str(), nullptr,
        MakeEnvHandler(onEnvCreated)
    );
    LogTrace(L"CreateCoreWebView2EnvironmentWithOptions returned hr=" + std::to_wstring(hr));

    return SUCCEEDED(hr);
}

STDMETHODIMP ModelPeekPreviewHandler::DoPreview() {
    LogTrace(L"PreviewHandler::DoPreview called. m_filePath=" + m_filePath);
    if (!m_hwndParent) {
        LogTrace(L"DoPreview: m_hwndParent is NULL");
        return E_UNEXPECTED;
    }
    if (m_filePath.empty()) {
        LogTrace(L"DoPreview: m_filePath is EMPTY");
        return E_FAIL;
    }

    if (!m_hwndPreview) {
        if (!CreateChildWindow()) {
            LogTrace(L"DoPreview: CreateChildWindow FAILED");
            return E_FAIL;
        }
    }

    if (!InitWebView2()) {
        LogTrace(L"DoPreview: InitWebView2 FAILED");
        return E_FAIL;
    }
    LogTrace(L"DoPreview: SUCCEEDED");
    return S_OK;
}

STDMETHODIMP ModelPeekPreviewHandler::Unload() {
    LogTrace(L"PreviewHandler::Unload called");
    if (m_controller) {
        m_controller->Close();
        m_controller->Release();
        m_controller = nullptr;
    }
    if (m_webview) {
        m_webview->Release();
        m_webview = nullptr;
    }

    if (m_hwndPreview) {
        DestroyWindow(m_hwndPreview);
        m_hwndPreview = NULL;
    }
    return S_OK;
}

STDMETHODIMP ModelPeekPreviewHandler::SetFocus() {
    if (m_controller) {
        m_controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
    } else if (m_hwndPreview) {
        ::SetFocus(m_hwndPreview);
    }
    return S_OK;
}

STDMETHODIMP ModelPeekPreviewHandler::QueryFocus(HWND *phwnd) {
    if (!phwnd) return E_POINTER;
    *phwnd = ::GetFocus();
    return S_OK;
}

STDMETHODIMP ModelPeekPreviewHandler::TranslateAccelerator(MSG *pmsg) {
    if (!pmsg) return E_POINTER;
    return S_FALSE;
}

STDMETHODIMP ModelPeekPreviewHandler::GetWindow(HWND *phwnd) {
    if (!phwnd) return E_POINTER;
    *phwnd = m_hwndPreview;
    return S_OK;
}

STDMETHODIMP ModelPeekPreviewHandler::ContextSensitiveHelp(BOOL /*fEnterMode*/) {
    return E_NOTIMPL;
}

STDMETHODIMP ModelPeekPreviewHandler::SetBackgroundColor(COLORREF /*color*/) {
    return S_OK;
}

STDMETHODIMP ModelPeekPreviewHandler::SetFont(const LOGFONTW* /*plf*/) {
    return S_OK;
}

STDMETHODIMP ModelPeekPreviewHandler::SetTextColor(COLORREF /*color*/) {
    return S_OK;
}
