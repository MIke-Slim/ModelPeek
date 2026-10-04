#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include "Guids.h"
#include "ClassFactory.h"
#include "ModelPeekThumbnailProvider.h"
#include "ModelPeekPreviewHandler.h"
#include "Logger.h"

HINSTANCE g_hInst = NULL;
long g_serverLocks = 0;

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/) {
    if (dwReason == DLL_PROCESS_ATTACH) {
        g_hInst = hInstance;
        DisableThreadLibraryCalls(hInstance);
    }
    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv) {
    if (!ppv) return E_POINTER;
    *ppv = nullptr;

    if (IsEqualCLSID(rclsid, CLSID_ModelPeekThumbnailProvider)) {
        LogTrace(L"DllGetClassObject: requested ModelPeekThumbnailProvider");
        ClassFactory<ModelPeekThumbnailProvider> *pFactory = new (std::nothrow) ClassFactory<ModelPeekThumbnailProvider>();
        if (!pFactory) return E_OUTOFMEMORY;
        HRESULT hr = pFactory->QueryInterface(riid, ppv);
        pFactory->Release();
        return hr;
    } else if (IsEqualCLSID(rclsid, CLSID_ModelPeekPreviewHandler)) {
        LogTrace(L"DllGetClassObject: requested ModelPeekPreviewHandler");
        ClassFactory<ModelPeekPreviewHandler> *pFactory = new (std::nothrow) ClassFactory<ModelPeekPreviewHandler>();
        if (!pFactory) return E_OUTOFMEMORY;
        HRESULT hr = pFactory->QueryInterface(riid, ppv);
        pFactory->Release();
        return hr;
    }

    LogTrace(L"DllGetClassObject: unknown CLSID");
    return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllCanUnloadNow() {
    return (g_serverLocks == 0) ? S_OK : S_FALSE;
}

static HRESULT SetRegString(HKEY root, LPCWSTR subKey, LPCWSTR valueName, LPCWSTR value) {
    HKEY hKey;
    LONG res = RegCreateKeyExW(root, subKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    if (res != ERROR_SUCCESS) return HRESULT_FROM_WIN32(res);
    res = RegSetValueExW(hKey, valueName, 0, REG_SZ, (const BYTE*)value, (DWORD)((wcslen(value) + 1) * sizeof(WCHAR)));
    RegCloseKey(hKey);
    return HRESULT_FROM_WIN32(res);
}

static HRESULT SetRegDword(HKEY root, LPCWSTR subKey, LPCWSTR valueName, DWORD value) {
    HKEY hKey;
    LONG res = RegCreateKeyExW(root, subKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    if (res != ERROR_SUCCESS) return HRESULT_FROM_WIN32(res);
    res = RegSetValueExW(hKey, valueName, 0, REG_DWORD, (const BYTE*)&value, sizeof(DWORD));
    RegCloseKey(hKey);
    return HRESULT_FROM_WIN32(res);
}

static void DeleteRegTreeIfExists(HKEY root, LPCWSTR subKey) {
    RegDeleteTreeW(root, subKey);
}

// Supported file extensions
static const LPCWSTR SUPPORTED_EXTS[] = {
    // Standard CAD & Mesh
    L".step", L".stp", L".stl", L".obj", L".fbx", L".glb", L".gltf", L".3mf",
    // Tier 1 (Industrial CAD & Scanning):
    L".iges", L".igs", L".brep", L".brp", L".ply",
    // Tier 2 (Toolpath & CG):
    L".gcode", L".dae", L".3ds"
};

static HKEY GetRegistryRoot() {
    HKEY testKey = NULL;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Classes", 0, KEY_WRITE, &testKey) == ERROR_SUCCESS) {
        RegCloseKey(testKey);
        return HKEY_LOCAL_MACHINE;
    }
    return HKEY_CURRENT_USER;
}

STDAPI DllRegisterServer() {
    WCHAR dllPath[MAX_PATH];
    GetModuleFileNameW(g_hInst, dllPath, MAX_PATH);
    LogTrace(std::wstring(L"DllRegisterServer started for: ") + dllPath);

    HKEY root = GetRegistryRoot();

    // 1. Register Thumbnail Provider CLSID
    std::wstring thumbClsidKey = L"Software\\Classes\\CLSID\\" + std::wstring(CLSID_THUMBNAIL_STRING);
    SetRegString(root, thumbClsidKey.c_str(), NULL, L"ModelPeek Thumbnail Provider");
    SetRegString(root, (thumbClsidKey + L"\\InprocServer32").c_str(), NULL, dllPath);
    SetRegString(root, (thumbClsidKey + L"\\InprocServer32").c_str(), L"ThreadingModel", L"Apartment");
    SetRegDword(root, thumbClsidKey.c_str(), L"DisableProcessIsolation", 1);

    // 2. Register Preview Handler CLSID
    std::wstring prevClsidKey = L"Software\\Classes\\CLSID\\" + std::wstring(CLSID_PREVIEW_STRING);
    SetRegString(root, prevClsidKey.c_str(), NULL, L"ModelPeek Preview Handler");
    SetRegString(root, prevClsidKey.c_str(), L"DisplayName", L"ModelPeek 3D Previewer");
    SetRegString(root, prevClsidKey.c_str(), L"AppID", PREVHOST_APPID_STRING);
    SetRegString(root, (prevClsidKey + L"\\InprocServer32").c_str(), NULL, dllPath);
    SetRegString(root, (prevClsidKey + L"\\InprocServer32").c_str(), L"ThreadingModel", L"Apartment");
    SetRegDword(root, prevClsidKey.c_str(), L"DisableProcessIsolation", 1);

    // 3. Register to Windows PreviewHandlers list
    SetRegString(root,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\PreviewHandlers",
        CLSID_PREVIEW_STRING,
        L"ModelPeek 3D Previewer");

    // 4. Associate extensions
    for (LPCWSTR ext : SUPPORTED_EXTS) {
        std::wstring shellExThumb = std::wstring(L"Software\\Classes\\") + ext + L"\\ShellEx\\{e357fccd-a995-4576-b01f-234630154e96}";
        SetRegString(root, shellExThumb.c_str(), NULL, CLSID_THUMBNAIL_STRING);

        std::wstring shellExPrev = std::wstring(L"Software\\Classes\\") + ext + L"\\ShellEx\\{8895b1c6-b41f-4c1c-a562-0d564250836f}";
        SetRegString(root, shellExPrev.c_str(), NULL, CLSID_PREVIEW_STRING);
    }

    // Notify Explorer of shell changes
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
    LogTrace(L"DllRegisterServer completed successfully.");
    return S_OK;
}

STDAPI DllUnregisterServer() {
    HKEY root = GetRegistryRoot();

    std::wstring thumbClsidKey = L"Software\\Classes\\CLSID\\" + std::wstring(CLSID_THUMBNAIL_STRING);
    DeleteRegTreeIfExists(root, thumbClsidKey.c_str());

    std::wstring prevClsidKey = L"Software\\Classes\\CLSID\\" + std::wstring(CLSID_PREVIEW_STRING);
    DeleteRegTreeIfExists(root, prevClsidKey.c_str());

    HKEY hKey;
    if (RegOpenKeyExW(root, L"Software\\Microsoft\\Windows\\CurrentVersion\\PreviewHandlers", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        RegDeleteValueW(hKey, CLSID_PREVIEW_STRING);
        RegCloseKey(hKey);
    }

    for (LPCWSTR ext : SUPPORTED_EXTS) {
        std::wstring shellExThumb = std::wstring(L"Software\\Classes\\") + ext + L"\\ShellEx\\{e357fccd-a995-4576-b01f-234630154e96}";
        DeleteRegTreeIfExists(root, shellExThumb.c_str());

        std::wstring shellExPrev = std::wstring(L"Software\\Classes\\") + ext + L"\\ShellEx\\{8895b1c6-b41f-4c1c-a562-0d564250836f}";
        DeleteRegTreeIfExists(root, shellExPrev.c_str());
    }

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
    return S_OK;
}
