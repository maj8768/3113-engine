#include "server_util.h"
#include "net_platform.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <random>

#ifdef _WIN32
    #include <process.h>
    #define netGetPid _getpid
#else
    #define netGetPid getpid
#endif

static char gInstanceId[40] = {0};
static uint32_t gInstanceNumber = 0;

void initInstanceId() {
    // Every launch must produce a different number, so no single source is
    // trusted on its own:
    //   - a time seed alone repeats when two instances start in the same tick,
    //     which is exactly the two-window test;
    //   - std::random_device is a fixed-sequence PRNG on some MinGW/libstdc++
    //     builds, so on those it returns the same value every run;
    //   - the pid is unique among live processes but can be recycled;
    //   - the stack address varies per run under ASLR.
    // Mixed together, all four have to fail simultaneously to collide.
    unsigned long long t =
        (unsigned long long)std::chrono::high_resolution_clock::now().time_since_epoch().count();

    std::random_device rd;
    uint32_t r = (uint32_t)rd() ^ ((uint32_t)rd() << 1);

    uint32_t mixed = (uint32_t)t;
    mixed ^= (uint32_t)(t >> 32);
    mixed ^= r;
    mixed ^= (uint32_t)netGetPid() * 2654435761u;              // Knuth's multiplicative hash
    mixed ^= (uint32_t)(uintptr_t)(void*)&t;                   // stack address (ASLR)

    if (mixed == 0) mixed = 1;                                 // 0 stays free to mean "unset"
    gInstanceNumber = mixed;

    std::snprintf(gInstanceId, sizeof(gInstanceId), "%08x-%04x-%08x",
                  (uint32_t)(t >> 32),
                  (uint32_t)(t & 0xffff),
                  gInstanceNumber);
}

const char* getInstanceId() {
    return gInstanceId;
}

uint32_t getInstanceNumber() {
    return gInstanceNumber;
}

void netSetPos(netPlayerState& s, float x, float y, float z) {
    std::memcpy(&s.x, &x, 4);
    std::memcpy(&s.y, &y, 4);
    std::memcpy(&s.z, &z, 4);
}

void netGetPos(const netPlayerState& s, float& x, float& y, float& z) {
    std::memcpy(&x, &s.x, 4);
    std::memcpy(&y, &s.y, 4);
    std::memcpy(&z, &s.z, 4);
}

void netPacketToWire(netPacket& p) {
    p.seq    = htonl(p.seq);
    p.type   = htonl(p.type);
    p.sender = htonl(p.sender);
    p.value  = htonl(p.value);
}

void netPacketToHost(netPacket& p) {
    p.seq    = ntohl(p.seq);
    p.type   = ntohl(p.type);
    p.sender = ntohl(p.sender);
    p.value  = ntohl(p.value);
}

void netStatePacketToWire(netStatePacket& p) {
    uint32_t count = p.count; // read before the header is swapped
    if (count > NET_MAX_PLAYERS) count = NET_MAX_PLAYERS;

    p.seq    = htonl(p.seq);
    p.type   = htonl(p.type);
    p.sender = htonl(p.sender);
    p.tps    = htonl(p.tps);
    p.count  = htonl(p.count);

    for (uint32_t i = 0; i < count; i++) {
        p.players[i].playerId = htonl(p.players[i].playerId);
        p.players[i].x = htonl(p.players[i].x);
        p.players[i].y = htonl(p.players[i].y);
        p.players[i].z = htonl(p.players[i].z);
    }
}

void netStatePacketToHost(netStatePacket& p) {
    p.seq    = ntohl(p.seq);
    p.type   = ntohl(p.type);
    p.sender = ntohl(p.sender);
    p.tps    = ntohl(p.tps);
    p.count  = ntohl(p.count);

    // Clamp before it indexes anything: count arrives from the network and a
    // corrupt or hostile value would otherwise walk off the end of players[].
    if (p.count > NET_MAX_PLAYERS) p.count = NET_MAX_PLAYERS;

    for (uint32_t i = 0; i < p.count; i++) {
        p.players[i].playerId = ntohl(p.players[i].playerId);
        p.players[i].x = ntohl(p.players[i].x);
        p.players[i].y = ntohl(p.players[i].y);
        p.players[i].z = ntohl(p.players[i].z);
    }
}

bool netAcceptSeq(uint32_t incoming, uint32_t& lastSeen, bool& haveLastSeen) {
    if (haveLastSeen && incoming <= lastSeen) return false; // stale or duplicate
    lastSeen = incoming;
    haveLastSeen = true;
    return true;
}


// --- cross-thread state ------------------------------------------------------

static std::mutex gLocalPlayerMutex;
static float gLocalX = 0.f, gLocalY = 0.f, gLocalZ = 0.f;

void netSetLocalPlayer(float x, float y, float z) {
    std::lock_guard<std::mutex> lock(gLocalPlayerMutex);
    gLocalX = x;
    gLocalY = y;
    gLocalZ = z;
}

void netGetLocalPlayer(float& x, float& y, float& z) {
    std::lock_guard<std::mutex> lock(gLocalPlayerMutex);
    x = gLocalX;
    y = gLocalY;
    z = gLocalZ;
}

static std::mutex gRemoteMutex;
static playerPacket gRemote[NET_MAX_PLAYERS];
static int gRemoteCount = 0;

void netApplySnapshot(const netStatePacket& snap) {
    std::lock_guard<std::mutex> lock(gRemoteMutex);

    gRemoteCount = 0;
    uint32_t count = snap.count;
    if (count > NET_MAX_PLAYERS) count = NET_MAX_PLAYERS;

    for (uint32_t i = 0; i < count; i++) {
        // Our own state comes back in the snapshot; skip it or we would spawn a
        // body standing inside the local camera.
        if (snap.players[i].playerId == getInstanceNumber()) continue;

        playerPacket& out = gRemote[gRemoteCount];
        out.playerId = (int)snap.players[i].playerId;
        netGetPos(snap.players[i], out.x, out.y, out.z);
        gRemoteCount++;
    }
}

int netGetPlayerPackets(playerPacket* out, int maxOut) {
    std::lock_guard<std::mutex> lock(gRemoteMutex);

    int n = gRemoteCount;
    if (n > maxOut) n = maxOut;
    for (int i = 0; i < n; i++) out[i] = gRemote[i];
    return n;
}

void netClearPlayerPackets() {
    std::lock_guard<std::mutex> lock(gRemoteMutex);
    gRemoteCount = 0;
}


#ifdef _WIN32
static int gNetInitCount = 0;
#endif

bool netInit() {
#ifdef _WIN32
    if (gNetInitCount == 0) {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            std::printf("[net] WSAStartup failed\n");
            return false;
        }
    }
    gNetInitCount++;
#endif
    return true;
}

void netShutdown() {
#ifdef _WIN32
    if (gNetInitCount > 0) {
        gNetInitCount--;
        if (gNetInitCount == 0) WSACleanup();
    }
#endif
}
