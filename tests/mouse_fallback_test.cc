// c++ -std=c++17 -Ivendor/raylib/include tests/mouse_fallback_test.cc system/mouse/mouse.cpp -o /tmp/mouse-fallback-test
#include "../system/mouse/mouse.h"
#include "raylib.h"

#include <cassert>

Vector2 GetMouseDelta() { return {3.5f,-2.f}; }
int main() {
    assert(RawMouseInitFromHWND(nullptr));
    float x=0, y=0;
    RawMouseGetDelta(x,y);
    assert(x == 3.5f && y == -2.f);
    pumpMessages();
    RawMouseShutdown();
}
