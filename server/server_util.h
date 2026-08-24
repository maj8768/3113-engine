#pragma once

#include <cstdint>

#ifdef STEAMWORKS_AVAILABLE
    // Pulled in for the send-mode constants only. Safe next to winsock2.h: no
    // steam header includes windows.h. Defined by the build when
    // vendor/steamworks exists — see setup-steamworks.sh.
    #include <steam/steamnetworkingtypes.h>
#endif


struct playerPacket {
    int playerId;
    float x, y, z;
};

struct GameProcessPacket {
    playerPacket* player;
    int playerPacketCount;
};


// --- networking --------------------------------------------------------------
// Key 8 spawns the server thread, key 9 spawns a client thread that joins it.
// No socket headers here on purpose — see net_platform.h.

const unsigned short NET_PORT = 5555;

// The SERVER owns the tick rate. It hands its value to every client in the
// JOIN_ACK and the client adopts whatever it is told, so the rate is mandated in
// exactly one place and a client can never run at a different cadence.
const uint32_t NET_TPS = 24;

const uint32_t NET_MAX_PLAYERS = 8;

// Liveness. The client pings on this interval and the server pongs back; either
// side that hears nothing at all for the timeout tears down peacefully.
// Any received packet counts as "heard", and state/snapshot traffic runs at
// NET_TPS, so these only fire on real loss or a peer that quit.
const int NET_PING_INTERVAL_SECONDS = 5;
const int NET_TIMEOUT_SECONDS = 5;

enum netMsgType {
    NET_JOIN     = 1,
    NET_JOIN_ACK = 2,
    NET_PING     = 3,
    NET_PONG     = 4,
    NET_STATE    = 5, // client -> server, once per tick: my position
    NET_SNAPSHOT = 6  // server -> client, once per tick: every position that tick
};

// Mirrors Steam's send flags so moving this onto
// ISteamNetworkingSockets::SendMessageToConnection later is a transport swap
// rather than a protocol change. State (positions) goes UNRELIABLE and leans on
// the sequence number below; discrete events (damage, pickups) go RELIABLE.
enum netSendMode {
    NET_SEND_UNRELIABLE = 0,
    NET_SEND_RELIABLE   = 8
};

#ifdef STEAMWORKS_AVAILABLE
static_assert(NET_SEND_UNRELIABLE == k_nSteamNetworkingSend_Unreliable,
              "steam send flag drift: NET_SEND_UNRELIABLE");
static_assert(NET_SEND_RELIABLE == k_nSteamNetworkingSend_Reliable,
              "steam send flag drift: NET_SEND_RELIABLE");
#endif

// Control/handshake packet. Fixed-size POD sent as one datagram; UDP preserves
// message boundaries so there is nothing to delimit. Fixed-width types because
// this struct's layout IS the wire format. seq is per-sender and monotonic.
struct netPacket {
    uint32_t seq;
    uint32_t type;
    uint32_t sender; // instance number of whoever sent it
    uint32_t value;  // JOIN_ACK: the server's mandated tick rate
    char id[40];
    char text[64];
};

// One player's position on the wire. Coordinates are stored as IEEE-754 BITS
// rather than floats so a byte-swapped value never lands in a float register,
// where an arbitrary bit pattern can be a signalling NaN. Use netSetPos/netGetPos.
struct netPlayerState {
    uint32_t playerId;
    uint32_t x, y, z;
};

// Carries NET_STATE (count == 1, the sender's own position) and NET_SNAPSHOT
// (count == every state the server collected for that tick, plus its own).
struct netStatePacket {
    uint32_t seq;
    uint32_t type;
    uint32_t sender;
    uint32_t tps;   // echoed by the server so clients can see the mandated rate
    uint32_t count;
    netPlayerState players[NET_MAX_PLAYERS];
};

// Layout guards: every member is 4-byte or a char array, so there should be no
// padding. If that changes the wire size shifts and the recvfrom size checks
// start rejecting everything — fail the build instead of debugging it live.
static_assert(sizeof(netPacket) == 4 + 4 + 4 + 4 + 40 + 64,
              "netPacket has padding; its layout is the wire format");
static_assert(sizeof(netPlayerState) == 4 + 4 + 4 + 4,
              "netPlayerState has padding; its layout is the wire format");
static_assert(sizeof(netStatePacket) == 20 + sizeof(netPlayerState) * NET_MAX_PLAYERS,
              "netStatePacket has padding; its layout is the wire format");

// float <-> the uint32 bit fields above. Host order on both sides; the
// ToWire/ToHost pass below handles endianness like it does for every other field.
void netSetPos(netPlayerState& s, float x, float y, float z);
void netGetPos(const netPlayerState& s, float& x, float& y, float& z);

// Endian normalisation. Packets travel in network byte order (big endian);
// htonl/ntohl compile to nothing on a big-endian host and to a byte swap on x86.
// Call ToWire immediately before sendto and ToHost immediately after recvfrom.
// The char arrays are byte sequences and need no conversion.
void netPacketToWire(netPacket& p);
void netPacketToHost(netPacket& p);
void netStatePacketToWire(netStatePacket& p);
void netStatePacketToHost(netStatePacket& p);

// SEQUENCED, not ordered: accepts a packet only if it is newer than the last one
// seen, and drops stale or duplicate datagrams outright. Never buffers and never
// waits, so a lost packet costs nothing — the next one supersedes it. Ordered
// delivery would hold newer packets until the gap filled, which is head-of-line
// blocking and the wrong trade for state that is already latest-wins.
// Updates lastSeen/haveLastSeen when it returns true.
bool netAcceptSeq(uint32_t incoming, uint32_t& lastSeen, bool& haveLastSeen);

// --- cross-thread state ------------------------------------------------------
// The network threads are not the game thread, so both directions go through a
// lock. Neither side ever blocks on the other for more than a copy.

// Local player position: written by the game thread every frame, read by
// whichever network thread is sending this instance's state on its tick.
void netSetLocalPlayer(float x, float y, float z);
void netGetLocalPlayer(float& x, float& y, float& z);

// Remote players: written by the network thread when a server snapshot lands,
// read by the game thread when it places the spawned player objects. This is the
// ONLY thing that moves them — no physics, no interpolation.
//
// Both roles write into the same table (the host applies its own broadcast
// locally), so a reader cannot tell whether this instance is hosting or joined,
// and does not need to.
void netApplySnapshot(const netStatePacket& snap);
int  netGetPlayerPackets(playerPacket* out, int maxOut);
void netClearPlayerPackets();

// Instance identity, generated once at startup by initInstanceId().
// getInstanceNumber() is the random int; getInstanceId() is a printable form of
// it for logs. Never 0 — that value is left free to mean "unset".
void initInstanceId();
const char* getInstanceId();
uint32_t getInstanceNumber();

// WSAStartup / WSACleanup on Windows, no-ops elsewhere. Refcounted.
bool netInit();
void netShutdown();
