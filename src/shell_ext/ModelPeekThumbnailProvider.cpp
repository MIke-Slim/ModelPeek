#include "ModelPeekThumbnailProvider.h"
#include "Guids.h"
#include "Logger.h"
#include <shlwapi.h>
#include <shlobj.h>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <cmath>
#include <algorithm>

extern HINSTANCE g_hInst;
extern long g_serverLocks;

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

ModelPeekThumbnailProvider::ModelPeekThumbnailProvider() : m_refCount(1) {
    LogTrace(L"ModelPeekThumbnailProvider created");
    InterlockedIncrement(&g_serverLocks);
}

ModelPeekThumbnailProvider::~ModelPeekThumbnailProvider() {
    LogTrace(L"ModelPeekThumbnailProvider destroyed");
    InterlockedDecrement(&g_serverLocks);
}

STDMETHODIMP ModelPeekThumbnailProvider::QueryInterface(REFIID riid, void **ppv) {
    if (!ppv) return E_POINTER;
    if (riid == IID_IUnknown || riid == __uuidof(IThumbnailProvider)) {
        *ppv = static_cast<IThumbnailProvider*>(this);
    } else if (riid == __uuidof(IInitializeWithFile)) {
        *ppv = static_cast<IInitializeWithFile*>(this);
    } else if (riid == __uuidof(IInitializeWithItem)) {
        *ppv = static_cast<IInitializeWithItem*>(this);
    } else if (riid == __uuidof(IInitializeWithStream)) {
        *ppv = static_cast<IInitializeWithStream*>(this);
    } else {
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) ModelPeekThumbnailProvider::AddRef() {
    return InterlockedIncrement(&m_refCount);
}

STDMETHODIMP_(ULONG) ModelPeekThumbnailProvider::Release() {
    ULONG count = InterlockedDecrement(&m_refCount);
    if (count == 0) delete this;
    return count;
}

STDMETHODIMP ModelPeekThumbnailProvider::Initialize(LPCWSTR pszFilePath, DWORD /*grfMode*/) {
    LogTrace(std::wstring(L"ThumbnailProvider::Initialize(File): ") + (pszFilePath ? pszFilePath : L"null"));
    if (!pszFilePath) return E_INVALIDARG;
    m_filePath = pszFilePath;
    return S_OK;
}

STDMETHODIMP ModelPeekThumbnailProvider::Initialize(IShellItem *psi, DWORD /*grfMode*/) {
    LogTrace(L"ThumbnailProvider::Initialize(IShellItem*)");
    if (!psi) return E_INVALIDARG;
    LPWSTR pszPath = nullptr;
    HRESULT hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
    if (SUCCEEDED(hr) && pszPath) {
        m_filePath = pszPath;
        LogTrace(std::wstring(L"ThumbnailProvider::Initialize(IShellItem) resolved path: ") + m_filePath);
        CoTaskMemFree(pszPath);
        return S_OK;
    }
    return hr;
}

STDMETHODIMP ModelPeekThumbnailProvider::Initialize(IStream *pStream, DWORD /*grfMode*/) {
    LogTrace(L"ThumbnailProvider::Initialize(IStream*)");
    if (!pStream) return E_INVALIDARG;
    STATSTG stat = {0};
    if (SUCCEEDED(pStream->Stat(&stat, STATFLAG_DEFAULT)) && stat.pwcsName) {
        if (PathFileExistsW(stat.pwcsName)) {
            m_filePath = stat.pwcsName;
            LogTrace(std::wstring(L"ThumbnailProvider: from stream Stat exists: ") + m_filePath);
            CoTaskMemFree(stat.pwcsName);
            return S_OK;
        } else {
            m_filePath = DumpStreamToTemp(pStream, stat.pwcsName);
            LogTrace(std::wstring(L"ThumbnailProvider: stream dumped to temp: ") + m_filePath);
            CoTaskMemFree(stat.pwcsName);
            return m_filePath.empty() ? E_FAIL : S_OK;
        }
    }
    m_filePath = DumpStreamToTemp(pStream, L"model.stl");
    return m_filePath.empty() ? E_FAIL : S_OK;
}

std::wstring ModelPeekThumbnailProvider::GetCacheDirectory() {
    WCHAR appData[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appData))) {
        std::wstring dir = std::wstring(appData) + L"\\ModelPeek\\cache\\thumbnails";
        CreateDirectoryW((std::wstring(appData) + L"\\ModelPeek").c_str(), NULL);
        CreateDirectoryW((std::wstring(appData) + L"\\ModelPeek\\cache").c_str(), NULL);
        CreateDirectoryW(dir.c_str(), NULL);
        return dir;
    }
    return L"";
}

std::wstring ModelPeekThumbnailProvider::GetThumbnailCachePath(const std::wstring& filePath, UINT cx) {
    std::wstring dir = GetCacheDirectory();
    if (dir.empty()) return L"";

    WIN32_FILE_ATTRIBUTE_DATA attr;
    uint64_t lastWrite = 0;
    uint64_t fileSize = 0;
    if (GetFileAttributesExW(filePath.c_str(), GetFileExInfoStandard, &attr)) {
        lastWrite = ((uint64_t)attr.ftLastWriteTime.dwHighDateTime << 32) | attr.ftLastWriteTime.dwLowDateTime;
        fileSize = ((uint64_t)attr.nFileSizeHigh << 32) | attr.nFileSizeLow;
    }

    uint64_t hash = 14695981039346656037ULL;
    for (WCHAR c : filePath) {
        hash ^= (uint64_t)(towlower(c));
        hash *= 1099511628211ULL;
    }
    hash ^= lastWrite;
    hash *= 1099511628211ULL;
    hash ^= fileSize;
    hash *= 1099511628211ULL;

    std::wstringstream ss;
    ss << dir << L"\\" << std::hex << hash << L"_" << cx << L".bmp";
    return ss.str();
}

bool ModelPeekThumbnailProvider::GenerateThumbnail(const std::wstring& filePath, const std::wstring& outBmpPath, UINT cx) {
    // 1. Try Named Pipe IPC if daemon worker is running (fast path, <1ms)
    LPCWSTR pipeName = L"\\\\.\\pipe\\ModelPeekWorkerPipe";
    if (WaitNamedPipeW(pipeName, 20)) {
        HANDLE hPipe = CreateFileW(pipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hPipe != INVALID_HANDLE_VALUE) {
            std::wstringstream req;
            req << L"thumbnail\t" << filePath << L"\t" << outBmpPath << L"\t" << cx;
            std::wstring reqStr = req.str();
            DWORD written = 0;
            if (WriteFile(hPipe, reqStr.c_str(), (DWORD)(reqStr.length() * sizeof(WCHAR)), &written, NULL)) {
                WCHAR resp[128] = {0};
                DWORD read = 0;
                ReadFile(hPipe, resp, sizeof(resp) - sizeof(WCHAR), &read, NULL);
            }
            CloseHandle(hPipe);
            if (PathFileExistsW(outBmpPath.c_str())) {
                LogTrace(L"ThumbnailProvider: Generated via Daemon Named Pipe IPC successfully!");
                return true;
            }
        }
    }

    // 2. Direct Process spawn fallback
    WCHAR modulePath[MAX_PATH];
    GetModuleFileNameW(g_hInst, modulePath, MAX_PATH);
    PathRemoveFileSpecW(modulePath);

    std::wstring workerExe = std::wstring(modulePath) + L"\\ModelPeekWorker.exe";

    std::wstringstream cmd;
    cmd << L"\"" << workerExe << L"\" thumbnail \"" << filePath << L"\" \"" << outBmpPath << L"\" " << cx;

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    std::wstring cmdStr = cmd.str();
    LogTrace(std::wstring(L"ThumbnailProvider executing worker: ") + cmdStr);
    std::vector<WCHAR> cmdLine(cmdStr.begin(), cmdStr.end());
    cmdLine.push_back(L'\0');

    if (CreateProcessW(NULL, cmdLine.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 20000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    bool exists = (PathFileExistsW(outBmpPath.c_str()) == TRUE);
    LogTrace(std::wstring(L"Thumbnail generated result: ") + (exists ? L"SUCCESS" : L"FAILED"));
    return exists;
}

STDMETHODIMP ModelPeekThumbnailProvider::GetThumbnail(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha) {
    LogTrace(L"ThumbnailProvider::GetThumbnail requested size=" + std::to_wstring(cx));
    if (!phbmp || !pdwAlpha) return E_POINTER;
    if (m_filePath.empty()) {
        LogTrace(L"ThumbnailProvider: m_filePath is EMPTY!");
        return E_UNEXPECTED;
    }

    std::wstring cachePath = GetThumbnailCachePath(m_filePath, cx);
    if (cachePath.empty()) return E_FAIL;

    if (!PathFileExistsW(cachePath.c_str())) {
        if (!GenerateThumbnail(m_filePath, cachePath, cx)) {
            return E_FAIL;
        }
    }

    HBITMAP hBmp = (HBITMAP)LoadImageW(NULL, cachePath.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    if (!hBmp) {
        LogTrace(L"ThumbnailProvider: LoadImageW failed for " + cachePath);
        return E_FAIL;
    }

    *phbmp = hBmp;
    *pdwAlpha = WTSAT_RGB;
    LogTrace(L"ThumbnailProvider: Returning valid HBITMAP successfully!");
    return S_OK;
}
