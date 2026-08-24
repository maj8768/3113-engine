bool getAsyncKeyStateWrapper(int key);

// Edge-detected: true only on the frame `key` transitions from up to down.
bool getKeyPressedOnce(int key);

// Gate all keyboard reads. When disabled, getAsyncKeyStateWrapper (and thus
// getKeyPressedOnce) report every key as up — used to ignore OS-global key state
// while the game window doesn't have focus.
void setInputEnabled(bool enabled);
