#include "keyboard.h"
#include <iostream>
#include <unordered_map>

bool getKeyPressedOnce(int key) {
    static std::unordered_map<int, bool> prev;
    bool down = getAsyncKeyStateWrapper(key);
    bool wasDown = prev[key];
    prev[key] = down;
    return down && !wasDown;
}

int rlToMacKey(int key) {
    switch (key) {
        case 87: return 13;
        case 65: return 0;
        case 83: return 1;
        case 68: return 2;
        case 32: return 49;
        case 82: return 15;
        case 69: return 14;
        case 81: return 12;
        case 70: return 3;
        case 71: return 5;
        case 78: return 45;
        case 49: return 18;
        case 50: return 19;
        case 51: return 20;
        case 80: return 35;
        case 340: return 56;
        case 341: return 59;
        case 263: return 123;
        case 265: return 126;
        case 262: return 124;
        case 264: return 125;
        case 257: return 36;
        default: return key;
    }
}

int rlToWinKey(int key) {
    switch (key) {
        case 87: return 87;
        case 65: return 65;
        case 83: return 83;
        case 68: return 68;
        case 32: return 32;
        case 82: return 82;
        case 69: return 69;
        case 81: return 81;
        case 70: return 70;
        case 71: return 71;
        case 78: return 78;
        case 49: return 49;
        case 50: return 50;
        case 51: return 51;
        case 80: return 80;
        case 340: return 16;
        case 341: return 17;
        case 263: return 37;
        case 265: return 38;
        case 262: return 39;
        case 264: return 40;
        case 257: return 13;
        default: return key;
    }
}

#ifdef _WIN32
#include <windows.h>

bool getAsyncKeyStateWrapper(int key) {

    SHORT state = GetAsyncKeyState(rlToWinKey(key));

    return (state & 0x8000) != 0;
}

#elif defined(__APPLE__)
#include <ApplicationServices/ApplicationServices.h>

bool getAsyncKeyStateWrapper(int key) {
    int macKey = rlToMacKey(key);

    return CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, macKey);
}

#elif defined(__linux__)
#if defined(USE_X11)
#include <X11/Xlib.h>
#include <X11/keysym.h>

bool getAsyncKeyStateWrapper(KeySym keysym) {
    static Display* display = XOpenDisplay(nullptr);
    if (!display) return false;

    char keys[32];
    XQueryKeymap(display, keys);

    KeyCode keycode = XKeysymToKeycode(display, keysym);
    if (keycode == 0) return false;

    return (keys[keycode / 8] & (1 << (keycode % 8))) != 0;
}

#elif defined(USE_WAYLAND)
#include <unordered_map>

static std::unordered_map<int, bool> g_keyStates;

inline void onWaylandKeyEvent(int key, bool pressed) {
    g_keyStates[key] = pressed;
}

bool getAsyncKeyStateWrapper(int key) {
    auto it = g_keyStates.find(key);
    if (it == g_keyStates.end()) return false;
    return it->second;
}
#endif
#endif
