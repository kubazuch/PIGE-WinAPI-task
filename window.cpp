#include "window.h"

#include "resource.h"

#pragma region window

RECT window::window_rect(int width, int height, DWORD style, BOOL menu)
{
    RECT rect{
        .left = 0,
        .top = 0,
        .right = width,
        .bottom = height
    };

    AdjustWindowRect(&rect, style, menu);
    return rect;
}


window::window(window_class* wClass, const std::wstring& title)
    : m_hWnd{ nullptr }, m_hInst{ wClass->getInstance() }
{
    int x = GetSystemMetrics(SM_CXSCREEN);
    int y = GetSystemMetrics(SM_CYSCREEN);

    CreateWindowExW(0, wClass->getName(), title.c_str(), WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        x / 4, y / 4, x / 2, y / 2,
        nullptr, nullptr, wClass->getInstance(), reinterpret_cast<LPVOID>(this));
}

window::~window()
{
    if (offOldBitmap != nullptr)
        SelectObject(offDC, offOldBitmap);
    if (offDC != nullptr)
        DeleteObject(offDC);
    if (offBitmap != nullptr)
        DeleteObject(offBitmap);
    if (arial != nullptr)
        DeleteObject(arial);
    if (bigger_arial != nullptr)
        DeleteObject(bigger_arial);
    if (m_hWnd)
        DestroyWindow(m_hWnd);
}

LRESULT window::window_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    window* w = nullptr;
    if (msg == WM_NCCREATE)
    {
        auto pcs = reinterpret_cast<LPCREATESTRUCTW>(lParam);
        w = static_cast<window*>(pcs->lpCreateParams);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(w));
        w->m_hWnd = hWnd;
    }
    else
        w = reinterpret_cast<window*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

    if (w)
    {
        auto r = w->window_proc(msg, wParam, lParam);
        if (msg == WM_NCDESTROY)
        {
            w->m_hWnd = nullptr;
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, 0);
        }
        return r;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT window::window_proc(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        HDC hdc = GetDC(m_hWnd);
        offDC = CreateCompatibleDC(hdc);
        ReleaseDC(m_hWnd, hdc);
        arial = CreateFont(-MulDiv(16, GetDeviceCaps(offDC, LOGPIXELSY), 72),
            0,
            0,
            0,
            FW_BOLD,
            false,
            FALSE,
            0,
            EASTEUROPE_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_SWISS,
            L"Arial");

        bigger_arial = CreateFont(-MulDiv(18, GetDeviceCaps(offDC, LOGPIXELSY), 72),
            0,
            0,
            0,
            FW_BOLD,
            false,
            FALSE,
            0,
            EASTEUROPE_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_SWISS,
            L"Arial");
        break;
    }
    case WM_SIZE:
    {
        const int clientWidth = LOWORD(lParam);
        const int clientHeight = HIWORD(lParam);
        const HDC hdc = GetDC(m_hWnd);
        if (offOldBitmap != nullptr)
            SelectObject(offDC, offOldBitmap);
        if (offBitmap != nullptr)
            DeleteObject(offBitmap);

        offBitmap = CreateCompatibleBitmap(hdc, clientWidth, clientHeight);
        offOldBitmap = static_cast<HBITMAP>(SelectObject(offDC, offBitmap));
        ReleaseDC(m_hWnd, hdc);
        break;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        const HDC hdc = BeginPaint(m_hWnd, &ps);
        RECT rc;
        GetClientRect(m_hWnd, &rc);

        const HFONT oldFont = static_cast<HFONT>(SelectObject(offDC, arial));
        draw(rc);
        SelectObject(offDC, oldFont);

        BitBlt(hdc, 0, 0, rc.right, rc.bottom, offDC, 0, 0, SRCCOPY);
        EndPaint(m_hWnd, &ps);
        break;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_CLOSE:
        DestroyWindow(m_hWnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(EXIT_SUCCESS);
        return 0;
    default:
        return DefWindowProcW(m_hWnd, msg, wParam, lParam);
    }

    return 0;
}

void window::draw(const RECT& clientRect)
{
    const HBRUSH brush = CreateSolidBrush(RGB(255,255,0));
    const HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(offDC, brush));
    Rectangle(offDC, 0, 0, clientRect.right, clientRect.bottom);
    SelectObject(offDC, GetStockObject(BLACK_BRUSH));
    constexpr int margin = 50;
    Rectangle(offDC, margin, margin, clientRect.right - margin, clientRect.bottom - margin);
    SelectObject(offDC, oldBrush);
    DeleteObject(brush);
}

// https://learn.microsoft.com/en-us/windows/win32/gdi/alpha-blending-a-bitmap
void window::overlay(RECT* rc, HBRUSH brush) const
{
    const HDC transparentDC = CreateCompatibleDC(offDC);
    const HBITMAP transparentMap = CreateCompatibleBitmap(offDC, rc->right, rc->bottom);
    SelectObject(transparentDC, transparentMap);
    FillRect(transparentDC, rc, brush);

    BLENDFUNCTION bf{
        .BlendOp = AC_SRC_OVER,
        .BlendFlags = 0,
        .SourceConstantAlpha = 0xbf,
        .AlphaFormat = 0
    };

    AlphaBlend(offDC, 0, 0,
        rc->right, rc->bottom,
        transparentDC, 0, 0, rc->right, rc->bottom, bf);

    DeleteObject(transparentMap);
    DeleteDC(transparentDC);
}


BOOL window::show(int nCmdShow) const
{
    return ShowWindow(m_hWnd, nCmdShow);
}

#pragma endregion


#pragma region window_class

window::window_class::window_class(WNDCLASSEXW wcx)
    : m_hInstance{ wcx.hInstance }, m_lpszClassName{ wcx.lpszClassName }
{
    RegisterClassExW(&wcx);
}

window::window_class::~window_class()
{
    UnregisterClassW(m_lpszClassName, m_hInstance);
}

#pragma endregion

#pragma region builder

window::window_class::builder::builder(HINSTANCE hInstance, LPCWSTR lpszClassName)
{
    WNDCLASSEXW wcx;
    GetClassInfoExW(hInstance, lpszClassName, &wcx);

    m_wcx.lpszClassName = lpszClassName;
    m_wcx.hInstance = hInstance;
}

window::window_class::builder& window::window_class::builder::withStyle(UINT style)
{
    m_wcx.style = style;
    return *this;
}

window::window_class::builder& window::window_class::builder::withWndProc(WNDPROC wndProc)
{
    m_wcx.lpfnWndProc = wndProc;
    return *this;
}

window::window_class::builder& window::window_class::builder::withClsExtra(int clsExtra)
{
    m_wcx.cbClsExtra = clsExtra;
    return *this;
}

window::window_class::builder& window::window_class::builder::withWndExtra(int wndExtra)
{
    m_wcx.cbWndExtra = wndExtra;
    return *this;
}

window::window_class::builder& window::window_class::builder::withIcon(HICON icon, HICON small)
{
    m_wcx.hIcon = icon;
    m_wcx.hIconSm = small;
    return *this;
}

window::window_class::builder& window::window_class::builder::withCursor(HCURSOR cursor)
{
    m_wcx.hCursor = cursor;
    return *this;
}

window::window_class::builder& window::window_class::builder::withBackground(HBRUSH hBrush)
{
    m_wcx.hbrBackground = hBrush;
    return *this;
}

window::window_class::builder& window::window_class::builder::withMenu(LPCWSTR menu)
{
    m_wcx.lpszMenuName = menu;
    return *this;
}

#pragma endregion





