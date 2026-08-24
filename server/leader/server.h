
#pragma once
#include "../server_util.h"


void createServer();

void shutdownServer();

void ping(int id);

void serverTick(float deltaTime);

void recieveClientPacket(GameProcessPacket packet);

void sendServerPacket(GameProcessPacket* packets, int packetCount);


// --- loopback test harness (key 8) -------------------------------------------
// Spawns a thread holding a UDP socket bound to 127.0.0.1:NET_PORT, answering
// JOIN with JOIN_ACK and PING with PONG.

void startServerThread();

void stopServerThread();

bool isServerThreadRunning();
