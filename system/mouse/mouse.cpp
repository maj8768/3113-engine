#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "mouse.h"
#include <iostream>

static HWND g_hwnd = nullptr;
static WNDPROC g_oldWndProc = nullptr;
static LONG g_mouseDx = 0;
static LONG g_mouseDy = 0;

static LRESULT CALLBACK RawMouseWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INPUT:
        {
            UINT size = 0;
            if (GetRawInputData(
                    reinterpret_cast<HRAWINPUT>(lParam),
                    RID_INPUT,
                    nullptr,
                    &size,
                    sizeof(RAWINPUTHEADER)) != 0)
            {
                break;
            }

            BYTE buffer[sizeof(RAWINPUT)] = {};
            size = sizeof(buffer);

            UINT read = GetRawInputData(
                reinterpret_cast<HRAWINPUT>(lParam),
                RID_INPUT,
                buffer,
                &size,
                sizeof(RAWINPUTHEADER));

            if (read == size)
            {
                RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(buffer);

                if (raw->header.dwType == RIM_TYPEMOUSE) {
                    g_mouseDx += raw->data.mouse.lLastX;
                    g_mouseDy += raw->data.mouse.lLastY;
                }
                return DefWindowProc(hwnd, msg, wParam, lParam);
            }
            break;
        }
    }

    return CallWindowProc(g_oldWndProc, hwnd, msg, wParam, lParam);
}

bool RawMouseInitFromHWND(void* windowHandle)
{
    if (!windowHandle) return false;

    g_hwnd = static_cast<HWND>(windowHandle);

    g_oldWndProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtr(g_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(RawMouseWndProc))
    );

    if (!g_oldWndProc) return false;

    RAWINPUTDEVICE rid = {};
    rid.usUsagePage = 0x01; // generic desktop controls
    rid.usUsage     = 0x02; // mouse
    rid.dwFlags     = 0;
    rid.hwndTarget  = g_hwnd;

    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
    {
        SetWindowLongPtr(g_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_oldWndProc));
        g_oldWndProc = nullptr;
        g_hwnd = nullptr;
        return false;
    }

    return true;
}

void RawMouseShutdown()
{
    if (g_hwnd && g_oldWndProc)
    {
        SetWindowLongPtr(g_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_oldWndProc));
    }

    g_oldWndProc = nullptr;
    g_hwnd = nullptr;
    g_mouseDx = 0;
    g_mouseDy = 0;
}

void RawMouseGetDelta(float& dx, float& dy)
{
    dx = static_cast<float>(g_mouseDx);
    dy = static_cast<float>(g_mouseDy);
    g_mouseDx = 0;
    g_mouseDy = 0;
}

void pumpMessages() {
    MSG msg;
    int limit = 50; // process at most 10 messages per frame
    while (limit-- && PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

#endif

#if defined(__APPLE__) || defined(__linux__)
#include "mouse.h"
#include "raylib.h"

bool RawMouseInitFromHWND(void* windowHandle) { return true; }
void RawMouseShutdown() {}

void RawMouseGetDelta(float& dx, float& dy)
{
    Vector2 delta = GetMouseDelta();
    dx = delta.x;
    dy = delta.y;
}

void pumpMessages() {}

#endif