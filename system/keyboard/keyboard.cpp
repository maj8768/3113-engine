#include "keyboard.h"
#include <iostream>
// #include "raylib.h"

int rlToMacKey(int key) {
    switch (key) {
        case 87: return 13;
        case 65: return 0;
        case 83: return 1;
        case 68: return 2;
        case 32: return 49;
        case 82: return 15;
            
        case 49: return 18;
        case 50: return 19;
        case 51: return 20;

        case 37: return 123;
        case 38: return 126;
        case 39: return 124;
        case 40: return 125;
            
        case 257: return 36;
            
        default: return key;
    }
}

int rlToWinKey(int key) {
    switch (key) {
    case 87: return 87;   // W
    case 65: return 65;   // A
    case 83: return 83;   // S
    case 68: return 68;   // D
    case 32: return 32;   // Space

    case 37: return 37;   // Left arrow
    case 38: return 38;   // Up arrow
    case 39: return 39;   // Right arrow
    case 40: return 40;   // Down arrow

    case 257: return 13;  // Enter (VK_RETURN)

    default: return key;
}
}

#ifdef _WIN32
    #include <windows.h>

    bool getAsyncKeyStateWrapper(int key) {
        // std::cout << key << std::endl;
        SHORT state = GetAsyncKeyState(rlToWinKey(key));
        // if (state != 0) std::cout << state << std::endl;
        return (state & 0x8000) != 0;
    }

#elif defined(__APPLE__)
    #include <ApplicationServices/ApplicationServices.h>

    bool getAsyncKeyStateWrapper(int key) {
        int macKey = rlToMacKey(key);
//        std::cout << macKey << std::endl;
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