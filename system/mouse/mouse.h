#pragma once

// The declarations are available regardless of platform defines. Only the
// implementation selects Windows raw input or raylib's mouse delta.
bool RawMouseInitFromHWND(void* windowHandle);
void RawMouseShutdown();
void RawMouseGetDelta(float& dx, float& dy);
void pumpMessages();
