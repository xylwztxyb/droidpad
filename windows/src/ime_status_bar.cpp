#include "common.h"
#include "ime_status_bar.h"
#include "util.h"
#include "mime.h"

using namespace DroidPad;

#define SHOW_FUNC_ERROR(FUNC)                                                           \
    do                                                                                  \
    {                                                                                   \
        const WCHAR *format = L"Function:%s failed, with error code(%d) in line(%d)\n"; \
        WCHAR info[1024] = {0};                                                         \
        _snwprintf_s(info, 1023, format, FUNC, GetLastError(), __LINE__);               \
        if (m_pCallback)                                                                \
            m_pCallback->OnIMEStatusBarError(std::wstring(info));                       \
    } while (0);

#define WINDOW_CLASS_NAME L"IMEStatusBar"

static LRESULT __stdcall WndProc(HWND hwd, UINT message, WPARAM wParam, LPARAM lParam);

static ATOM _regCls(HINSTANCE hInst)
{
    WNDCLASSEX cls;
    cls.cbSize = sizeof(WNDCLASSEX);
    cls.style = CS_HREDRAW | CS_VREDRAW;
    cls.lpfnWndProc = WndProc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = hInst;
    cls.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    cls.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    cls.hCursor = LoadCursor(NULL, IDC_ARROW);
    cls.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
    cls.lpszMenuName = NULL;
    cls.lpszClassName = WINDOW_CLASS_NAME;

    return RegisterClassEx(&cls);
}

static bool _createWindow(HINSTANCE hInst, HWND *hwd)
{
    DWORD dwExStyle = WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_NOACTIVATE;
    DWORD dwStyle = WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME; //Disable changing size

    SIZE desktop_size = getDesktopSize();
    POINT WPos = {desktop_size.cx - IME_STATUS_BAR_WIDTH - 5,
                  desktop_size.cy - IME_STATUS_BAR_HEIGHT - 5};

    *hwd = CreateWindowEx(dwExStyle, WINDOW_CLASS_NAME, NULL, dwStyle,
                          WPos.x, WPos.y, IME_STATUS_BAR_WIDTH, IME_STATUS_BAR_HEIGHT,
                          NULL, NULL, hInst, NULL);
    if (!*hwd)
        return false;

    LONG style = GetWindowLong(*hwd, GWL_STYLE);
    if (style & WS_CAPTION)
        style ^= WS_CAPTION;
    SetWindowLong(*hwd, GWL_STYLE, style & ~WS_SYSMENU); //Remove close button
    return true;
}

static void _updateWindow(HWND hwd, LPPOINT pos, HDC src)
{
    BLENDFUNCTION bf;
    bf.AlphaFormat = AC_SRC_ALPHA;
    bf.BlendFlags = 0;
    bf.BlendOp = AC_SRC_OVER;
    bf.SourceConstantAlpha = 255;

    POINT ptSrc = {0, 0};
    LPPOINT pptSrc = (src == NULL ? NULL : &ptSrc);
    SIZE size = {IME_STATUS_BAR_WIDTH, IME_STATUS_BAR_HEIGHT};
    UpdateLayeredWindow(hwd, GetDC(NULL),
                        pos, &size,
                        src, pptSrc, 0,
                        &bf, ULW_ALPHA);
}

static LRESULT __stdcall WndProc(HWND hwd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwd, &ps);
        EndPaint(hwd, &ps);
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE; //Disable focus
    case WM_USER:
    {
        switch (wParam)
        {
        case STATE_CONNECTED_BY_USB:
            _updateWindow(hwd, NULL, CImageCache::getCachedDC(STATE_CONNECTED_BY_USB));
            break;
        case STATE_CONNECTED_BY_WIFI:
            _updateWindow(hwd, NULL, CImageCache::getCachedDC(STATE_CONNECTED_BY_WIFI));
            break;
        case STATE_CONNECTING:
            _updateWindow(hwd, NULL, CImageCache::getCachedDC(STATE_CONNECTING));
            break;
        case STATE_NOT_CONNECTED:
            _updateWindow(hwd, NULL, CImageCache::getCachedDC(STATE_NOT_CONNECTED));
            break;
        case STATE_CONNECTION_FAIL:
            _updateWindow(hwd, NULL, CImageCache::getCachedDC(STATE_CONNECTION_FAIL));
            break;
        }
        break;
    }
    default:
        return DefWindowProc(hwd, message, wParam, lParam);
    }
    return S_OK;
}

/////////////////////////////////////////////////////////////////////

namespace DroidPad
{

CIMEStatusBar::CIMEStatusBar() : m_hWnd(NULL),
                                 m_hInst(NULL),
                                 /*  m_hWinCreated(NULL), */
                                 m_eCurState(STATE_NOT_CONNECTED),
                                 m_pCallback(NULL)
{
    // m_hWinCreated = CreateEvent(NULL, true, false, NULL);
}

CIMEStatusBar::~CIMEStatusBar()
{
    if (m_hWnd != NULL)
        DestroyWindow(m_hWnd);
    m_hWnd = NULL;
    if (m_hWinCreated != NULL)
        CloseHandle(m_hWinCreated);
    m_hWinCreated = NULL;
}

void CIMEStatusBar::OnExitThread()
{
    SendMessage(m_hWnd, WM_DESTROY, NULL, NULL);
}

void CIMEStatusBar::showBar(bool bShow)
{
    bShow ? ShowWindow(m_hWnd, SW_SHOWNOACTIVATE) : ShowWindow(m_hWnd, SW_HIDE);
}

void CIMEStatusBar::showMessage(LPCTSTR msg)
{
    MessageBox(m_hWnd, msg, NULL, MB_OK);
}

void CIMEStatusBar::IMEOnDeviceConnected(eConnType type)
{
    if (type == CNN_TYPE_USB && m_eCurState != STATE_CONNECTED_BY_USB)
    {
        m_eCurState = STATE_CONNECTED_BY_USB;
        SendMessage(m_hWnd, WM_USER, STATE_CONNECTED_BY_USB, 0);
    }
    else if (type == CNN_TYPE_WIFI && m_eCurState != STATE_CONNECTED_BY_WIFI)
    {
        m_eCurState = STATE_CONNECTED_BY_WIFI;
        SendMessage(m_hWnd, WM_USER, STATE_CONNECTED_BY_WIFI, 0);
    }
}

void CIMEStatusBar::IMEOnDeviceConnecting()
{
    if (m_eCurState != STATE_CONNECTING)
    {
        m_eCurState = STATE_CONNECTING;
        SendMessage(m_hWnd, WM_USER, STATE_CONNECTING, 0);
    }
}

void CIMEStatusBar::IMEOnDeviceNone()
{
    if (m_eCurState != STATE_NOT_CONNECTED)
    {
        m_eCurState = STATE_NOT_CONNECTED;
        SendMessage(m_hWnd, WM_USER, STATE_NOT_CONNECTED, 0);
    }
}

void CIMEStatusBar::IMEOnDeviceConnectionFail()
{
    if (m_eCurState != STATE_CONNECTION_FAIL)
    {
        m_eCurState = STATE_CONNECTION_FAIL;
        SendMessage(m_hWnd, WM_USER, STATE_CONNECTION_FAIL, 0);
    }
}

bool CIMEStatusBar::threadLoop()
{
    Gdiplus::GdiplusStartupInput _gdi_input;
    ULONG_PTR _gdi_token;
    Gdiplus::GdiplusStartup(&_gdi_token, &_gdi_input, NULL);

    if ((m_hInst = GetModuleHandle(NULL)) == NULL)
    {
        SHOW_FUNC_ERROR(L"IMEStatusBar::threadLoop");
        return false;
    }

    _regCls(m_hInst);
    if (!_createWindow(m_hInst, &m_hWnd))
    {
        SHOW_FUNC_ERROR(L"IMEStatusBar::threadLoop:_createWindow");
        return false;
    }

    //Load resources
    HDC hdc;
    if ((hdc = GetDC(m_hWnd)) == NULL)
    {
        SHOW_FUNC_ERROR("IMEStatusBar::threadLoop:GetDC");
        return false;
    }
    if (!CImageCache::cacheDC(hdc, IME_STATUS_BAR_WIDTH, IME_STATUS_BAR_HEIGHT))
    {
        SHOW_FUNC_ERROR("IMEStatusBar::threadLoop:cacheDC");
        return false;
    }
    ShowWindow(m_hWnd, SW_SHOWNOACTIVATE);
    _updateWindow(m_hWnd, NULL, CImageCache::getCachedDC(m_eCurState));
    if (m_pCallback)
        m_pCallback->OnIMEStatusBarReady(this);

    MSG msg;
    BOOL bRet;

    while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
    {
        if (bRet == -1)
            break;
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    CImageCache::clean();
    Gdiplus::GdiplusShutdown(_gdi_token);
    return false;
}

} // namespace DroidPad
