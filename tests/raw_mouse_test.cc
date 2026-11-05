// Compile the Windows backend against tests/fakes/windows.h, without a GUI:
// c++ -std=c++17 -D_WIN32 -Itests/fakes -c system/mouse/mouse.cpp -o /tmp/raw-mouse.o
// c++ -std=c++17 tests/raw_mouse_test.cc /tmp/raw-mouse.o -o /tmp/raw-mouse-test
#include "../system/mouse/mouse.h"
#include "fakes/windows.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <deque>

namespace {
int windowToken;
HWND window = &windowToken;
bool failHook = false, failRegister = false, failRead = false;
int registrations = 0, removals = 0, forwarded = 0, cleanups = 0, quits = 0;
std::deque<MSG> messages;
LRESULT previous(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    ++forwarded;
    return DefWindowProc(h, msg, wp, lp);
}
WNDPROC current = previous;
void packet(LONG x, LONG y, USHORT flags = 0, DWORD type = RIM_TYPEMOUSE) {
    RAWINPUT input{};
    input.header.dwType = type;
    input.data.mouse.usFlags = flags;
    input.data.mouse.lLastX = x; input.data.mouse.lLastY = y;
    current(window, WM_INPUT, 0, reinterpret_cast<LPARAM>(&input));
}
void expectDelta(float x, float y) {
    float dx = -1, dy = -1;
    RawMouseGetDelta(dx, dy);
    assert(dx == x && dy == y);
}
} // namespace

bool IsWindow(HWND h) { return h == window; }
HWND GetForegroundWindow() { return window; }
LONG_PTR SetWindowLongPtr(HWND h, int index, LONG_PTR proc) {
    assert(h == window && index == GWLP_WNDPROC);
    if (failHook) return 0;
    WNDPROC old = current;
    current = reinterpret_cast<WNDPROC>(proc);
    return reinterpret_cast<LONG_PTR>(old);
}
bool RegisterRawInputDevices(const RAWINPUTDEVICE* devices, UINT count, UINT size) {
    assert(count == 1 && size == sizeof(RAWINPUTDEVICE));
    assert(devices->usUsagePage == 1 && devices->usUsage == 2);
    if (devices->dwFlags == RIDEV_REMOVE) {
        assert(devices->hwndTarget == nullptr); ++removals;
    } else {
        assert(devices->dwFlags == 0 && devices->hwndTarget == window); ++registrations;
    }
    return !failRegister;
}
UINT GetRawInputData(HRAWINPUT h, UINT command, void* data, UINT* size, UINT headerSize) {
    assert(command == RID_INPUT && headerSize == sizeof(RAWINPUTHEADER));
    assert(data && *size >= sizeof(RAWINPUT)); // one aligned read, no size query
    assert(reinterpret_cast<uintptr_t>(data)%alignof(RAWINPUT) == 0);
    if (failRead) return static_cast<UINT>(-1);
    std::memcpy(data, h, sizeof(RAWINPUT)); *size = sizeof(RAWINPUT);
    return sizeof(RAWINPUT);
}
LRESULT CallWindowProc(WNDPROC p, HWND h, UINT msg, WPARAM wp, LPARAM lp) { return p(h,msg,wp,lp); }
LRESULT DefWindowProc(HWND, UINT msg, WPARAM, LPARAM) { if (msg == WM_INPUT) ++cleanups; return 0; }
bool PeekMessage(MSG* out, HWND, UINT, UINT, UINT) {
    if (messages.empty()) return false;
    *out = messages.front(); messages.pop_front(); return true;
}
bool TranslateMessage(const MSG*) { return true; }
LRESULT DispatchMessage(const MSG* m) { return current(m->hwnd,m->message,m->wParam,m->lParam); }
void PostQuitMessage(int code) { ++quits; messages.push_back({window,WM_QUIT,static_cast<WPARAM>(code),0}); }

int main() {
    assert(!RawMouseInitFromHWND(nullptr));
    int invalidWindow;
    assert(!RawMouseInitFromHWND(&invalidWindow));
    failHook = true;
    assert(!RawMouseInitFromHWND(window) && current == previous);
    failHook = false; failRegister = true;
    assert(!RawMouseInitFromHWND(window) && current == previous);
    failRegister = false;
    assert(RawMouseInitFromHWND(window));
    int registered = registrations;
    assert(RawMouseInitFromHWND(window) && registrations == registered); // idempotent init
    packet(10,-4); packet(-2,7);
    expectDelta(8,3); expectDelta(0,0); // consume exactly once
    assert(cleanups == 2 && forwarded == 2);
    packet(65535,65535,MOUSE_MOVE_ABSOLUTE);
    packet(20,30,0,RIM_TYPEKEYBOARD);
    failRead = true; packet(100,100); failRead = false;
    expectDelta(0,0);
    packet(4,5);
    current(window,WM_KILLFOCUS,0,0); expectDelta(0,0);
    packet(50,50); expectDelta(0,0);
    current(window,WM_SETFOCUS,0,0); packet(3,-9); expectDelta(3,-9);
    messages.push_back({window,WM_SETFOCUS,0,0});
    pumpMessages(); assert(messages.empty());
    messages.push_back({window,WM_QUIT,17,0});
    pumpMessages(); assert(quits == 1 && messages.size() == 1 && messages.front().wParam == 17);
    messages.clear();
    packet(1,2); RawMouseShutdown();
    assert(current == previous && removals == 1); expectDelta(0,0);
    RawMouseShutdown(); assert(removals == 1);
    assert(RawMouseInitFromHWND(window)); packet(8,9); expectDelta(8,9);
    RawMouseShutdown();
    std::puts("Windows mouse: declarations, init/rollback, packet accumulation, focus, forwarding, and shutdown passed");
}
