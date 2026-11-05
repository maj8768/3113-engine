#include "mouse.h"

#if defined(_WIN32) || defined(_WIN64) || defined(WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace {
HWND gWindow = nullptr;
WNDPROC gPreviousProc = nullptr;
float gDeltaX = 0.f;
float gDeltaY = 0.f;
bool gFocused = false;

void clearDelta() { gDeltaX = gDeltaY = 0.f; }

LRESULT CALLBACK rawMouseProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_KILLFOCUS || message == WM_SETFOCUS) {
        gFocused = message == WM_SETFOCUS;
        clearDelta(); // never apply motion accumulated while switching windows
    } else if (message == WM_INPUT) {
        // A RAWINPUT object provides alignment without a heap allocation or a
        // separate size query for each mouse packet.
        RAWINPUT input{};
        UINT size = sizeof(input);
        UINT read = GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT,
                                    &input, &size, sizeof(RAWINPUTHEADER));
        if (read != static_cast<UINT>(-1) && read >= sizeof(RAWINPUT) &&
            input.header.dwType == RIM_TYPEMOUSE && gFocused &&
            !(input.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)) {
            gDeltaX += static_cast<float>(input.data.mouse.lLastX);
            gDeltaY += static_cast<float>(input.data.mouse.lLastY);
        }
        // Forward to GLFW/raylib, including WM_INPUT cleanup and ordinary input.
    }
    return gPreviousProc ? CallWindowProc(gPreviousProc, window, message, wParam, lParam)
                         : DefWindowProc(window, message, wParam, lParam);
}
} // namespace

bool RawMouseInitFromHWND(void* windowHandle) {
    HWND window = static_cast<HWND>(windowHandle);
    if (!window || !IsWindow(window)) return false;
    if (gWindow == window && gPreviousProc) return true;
    RawMouseShutdown();

    WNDPROC previous = reinterpret_cast<WNDPROC>(
        SetWindowLongPtr(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(rawMouseProc)));
    if (!previous) return false;

    RAWINPUTDEVICE device{};
    device.usUsagePage = 0x01; // generic desktop
    device.usUsage = 0x02;     // mouse
    device.hwndTarget = window;
    // No INPUTSINK or NOLEGACY: foreground motion with normal mouse buttons.
    if (!RegisterRawInputDevices(&device, 1, sizeof(device))) {
        SetWindowLongPtr(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(previous));
        return false;
    }
    gWindow = window;
    gPreviousProc = previous;
    gFocused = GetForegroundWindow() == window;
    clearDelta();
    return true;
}

void RawMouseShutdown() {
    if (gWindow) {
        RAWINPUTDEVICE device{};
        device.usUsagePage = 0x01;
        device.usUsage = 0x02;
        device.dwFlags = RIDEV_REMOVE;
        device.hwndTarget = nullptr; // required when removing a registration
        RegisterRawInputDevices(&device, 1, sizeof(device));
        if (IsWindow(gWindow) && gPreviousProc)
            SetWindowLongPtr(gWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(gPreviousProc));
    }
    gPreviousProc = nullptr;
    gWindow = nullptr;
    gFocused = false;
    clearDelta();
}

void RawMouseGetDelta(float& dx, float& dy) {
    dx = gFocused ? gDeltaX : 0.f;
    dy = gFocused ? gDeltaY : 0.f;
    clearDelta();
}

void pumpMessages() {
    MSG message;
    int limit = 50;
    while (limit-- && PeekMessage(&message, nullptr, 0, 0, PM_REMOVE)) {
        if (message.message == WM_QUIT) {
            PostQuitMessage(static_cast<int>(message.wParam));
            break;
        }
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
}

#else

#include "raylib.h"

bool RawMouseInitFromHWND(void*) { return true; }
void RawMouseShutdown() {}
void RawMouseGetDelta(float& dx, float& dy) {
    Vector2 delta = GetMouseDelta();
    dx = delta.x;
    dy = delta.y;
}
void pumpMessages() {}

#endif
