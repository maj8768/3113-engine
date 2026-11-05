#pragma once

// Minimal Win32 API surface for testing the mouse handler without a Windows host.
#include <stdint.h>

#define CALLBACK
using UINT = unsigned int;
using DWORD = uint32_t;
using LONG = int32_t;
using USHORT = uint16_t;
using HWND = void*;
using HRAWINPUT = void*;
using WPARAM = uintptr_t;
using LPARAM = intptr_t;
using LRESULT = intptr_t;
using LONG_PTR = intptr_t;
using WNDPROC = LRESULT (*)(HWND, UINT, WPARAM, LPARAM);

constexpr UINT WM_INPUT = 0xff, WM_SETFOCUS = 7, WM_KILLFOCUS = 8, WM_QUIT = 0x12;
constexpr UINT RID_INPUT = 0x10000003, RIM_TYPEMOUSE = 0, RIM_TYPEKEYBOARD = 1;
constexpr DWORD RIDEV_REMOVE = 1;
constexpr USHORT MOUSE_MOVE_ABSOLUTE = 1;
constexpr int GWLP_WNDPROC = -4, PM_REMOVE = 1;

struct RAWINPUTHEADER { DWORD dwType, dwSize; void* hDevice; WPARAM wParam; };
struct RAWMOUSE {
    USHORT usFlags;
    DWORD ulButtons, ulRawButtons;
    LONG lLastX, lLastY;
    DWORD ulExtraInformation;
};
struct RAWINPUT { RAWINPUTHEADER header; union { RAWMOUSE mouse; } data; };
struct RAWINPUTDEVICE { USHORT usUsagePage, usUsage; DWORD dwFlags; HWND hwndTarget; };
struct MSG { HWND hwnd; UINT message; WPARAM wParam; LPARAM lParam; };

bool IsWindow(HWND);
HWND GetForegroundWindow();
LONG_PTR SetWindowLongPtr(HWND, int, LONG_PTR);
bool RegisterRawInputDevices(const RAWINPUTDEVICE*, UINT, UINT);
UINT GetRawInputData(HRAWINPUT, UINT, void*, UINT*, UINT);
LRESULT CallWindowProc(WNDPROC, HWND, UINT, WPARAM, LPARAM);
LRESULT DefWindowProc(HWND, UINT, WPARAM, LPARAM);
bool PeekMessage(MSG*, HWND, UINT, UINT, UINT);
bool TranslateMessage(const MSG*);
LRESULT DispatchMessage(const MSG*);
void PostQuitMessage(int);
