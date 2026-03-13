#ifdef _WIN32

bool RawMouseInitFromHWND(void* windowHandle);
void RawMouseShutdown();
void RawMouseGetDelta(float& dx, float& dy);
void pumpMessages();

#elif defined(__APPLE__) || defined(__linux__)

bool RawMouseInitFromHWND(void* windowHandle);
void RawMouseShutdown();
void RawMouseGetDelta(float& dx, float& dy);
void pumpMessages();

#endif