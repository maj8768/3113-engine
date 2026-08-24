
#pragma once
#include "../server_util.h"


void pong(int id);


// --- loopback test harness (key 9) -------------------------------------------
// Spawns a thread that JOINs the server on 127.0.0.1:NET_PORT, then PINGs once a
// second and prints the PONGs that come back.

void startClientThread();

void stopClientThread();

bool isClientThreadRunning();
