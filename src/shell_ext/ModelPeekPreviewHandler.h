#pragma once
#include <windows.h>
#include <shobjidl.h>
#include <ocidl.h>
#include <string>
#include "webview2/WebView2.h"

class ModelPeekPreviewHandler : public IPreviewHandler,
                                public IInitializeWithFile,
                                public IInitializeWithItem,
                                public IInitializeWithStream,
                                public IObjectWithSite,
                                public IOleWindow,
                                public IPreviewHandlerVisuals {
public:
    ModelPeekPreviewHandler();
    virtual ~ModelPeekPreviewHandler();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // IInitializeWithFile
    STDMETHODIMP Initialize(LPCWSTR pszFilePath, DWORD grfMode) override;

    // IInitializeWithItem
    STDMETHODIMP Initialize(IShellItem *psi, DWORD grfMode) override;

    // IInitializeWithStream
    STDMETHODIMP Initialize(IStream *pstream, DWORD grfMode) override;

    // IObjectWithSite
    STDMETHODIMP SetSite(IUnknown *pUnkSite) override;
    STDMETHODIMP GetSite(REFIID riid, void **ppvSite) override;

    // IPreviewHandler
    STDMETHODIMP SetWindow(HWND hwnd, const RECT *prc) override;
    STDMETHODIMP SetRect(const RECT *prc) override;
    STDMETHODIMP DoPreview() override;
    STDMETHODIMP Unload() override;
    STDMETHODIMP SetFocus() override;
    STDMETHODIMP QueryFocus(HWND *phwnd) override;
    STDMETHODIMP TranslateAccelerator(MSG *pmsg) override;

    // IOleWindow
    STDMETHODIMP GetWindow(HWND *phwnd) override;
    STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) override;

    // IPreviewHandlerVisuals
    STDMETHODIMP SetBackgroundColor(COLORREF color) override;
    STDMETHODIMP SetFont(const LOGFONTW *plf) override;
    STDMETHODIMP SetTextColor(COLORREF color) override;

private:
    long m_refCount;
    std::wstring m_filePath;
    HWND m_hwndParent;
    RECT m_rcParent;
    HWND m_hwndPreview;
    IUnknown* m_punkSite;

    ICoreWebView2Controller* m_controller;
    ICoreWebView2* m_webview;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    bool CreateChildWindow();
    bool InitWebView2();
    std::wstring GetViewerHtmlPath();
    std::wstring PrepareModelForPreview(const std::wstring& filePath);
    HWND ResolveValidParent(HWND candidateHwnd, IUnknown* pSite, RECT* outRc);
};
