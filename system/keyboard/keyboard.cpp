#include "keyboard.h"
// #include "raylib.h"

#ifdef _WIN32
    #include <windows.h>

    bool getAsyncKeyStateWrapper(int key) {
        SHORT state = GetAsyncKeyState(key);
        return (state & 0x8000) != 0;
    }

#elif defined(__APPLE__)
    #include <ApplicationServices/ApplicationServices.h>

    static CGKeyCode toNativeKey(int key) {
        switch (key) {
            case KEY_A:     return 0;
            case KEY_S:     return 1;
            case KEY_D:     return 2;
            case KEY_W:     return 13;
            case KEY_SPACE: return 49;
            case KEY_LEFT:  return 123;
            case KEY_RIGHT: return 124;
            case KEY_DOWN:  return 125;
            case KEY_UP:    return 126;
            default:        return UINT16_MAX;
        }
    }

    bool getAsyncKeyStateWrapper(int key) {
        CGKeyCode code = toNativeKey(key);
        if (code == UINT16_MAX) return false;
        return CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, code);
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
