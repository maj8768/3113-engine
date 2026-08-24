#include "../server_util.h"
#include "server.h"
#include "../net_platform.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>

GameProcessPacket* currentPackets = nullptr;
int PacketCount = 0;

void createServer() {

}

void shutdownServer() {

}

void serverTick(float deltaTime) {

}

void recieveClientPacket(GameProcessPacket packet) {
    GameProcessPacket* newPackets = new GameProcessPacket[PacketCount + 1];
    for (int i = 0; i < PacketCount; i++) {
        newPackets[i] = currentPackets[i];
    }
    newPackets[PacketCount] = packet;
    delete[] currentPackets;
    currentPackets = newPackets;
    PacketCount++;
}

void sendServerPacket(GameProcessPacket* packets, int packetCount) {

}


// --- networked server --------------------------------------------------------

static std::thread gServerThread;
static std::atomic<bool> gServerRunning(false);

// One connected client. Sequence tracking is PER CLIENT: a single shared counter
// would make two clients' independent sequences interleave, and each would keep
// discarding the other's packets as stale.
struct serverClient {
    bool active;
    sockaddr_in addr;
    uint32_t sender;        // the client's instance number
    uint32_t lastSeq;
    bool haveSeq;

    bool stateThisTick;     // did this client send a position during the tick?
    netPlayerState state;   // its most recent one, host order

    std::chrono::steady_clock::time_point lastHeard;
};

// Set once a client has ever joined, so the "everyone left" shutdown below can't
// fire on a server that is simply still waiting for its first player.
static bool gHadClients = false;

static serverClient gClients[NET_MAX_PLAYERS];

static bool sameAddr(const sockaddr_in& a, const sockaddr_in& b) {
    return a.sin_addr.s_addr == b.sin_addr.s_addr && a.sin_port == b.sin_port;
}

// Finds the client by address, or claims a free slot for it. Returns null when
// the server is full.
static serverClient* findOrAddClient(const sockaddr_in& from, uint32_t sender) {
    auto now = std::chrono::steady_clock::now();

    for (uint32_t i = 0; i < NET_MAX_PLAYERS; i++) {
        if (gClients[i].active && sameAddr(gClients[i].addr, from)) {
            gClients[i].lastHeard = now;
            return &gClients[i];
        }
    }

    for (uint32_t i = 0; i < NET_MAX_PLAYERS; i++) {
        if (!gClients[i].active) {
            gClients[i] = serverClient{};
            gClients[i].active = true;
            gClients[i].addr = from;
            gClients[i].sender = sender;
            gClients[i].lastHeard = now;
            gHadClients = true;
            return &gClients[i];
        }
    }
    return nullptr;
}

// Drop clients that have gone silent. UDP has no disconnect, so without this a
// client that quit would hold its slot forever and the server would report
// itself full after a few test restarts. Returns how many are still connected.
static int expireClients() {
    auto now = std::chrono::steady_clock::now();
    int alive = 0;

    for (uint32_t i = 0; i < NET_MAX_PLAYERS; i++) {
        if (!gClients[i].active) continue;

        auto quietFor = std::chrono::duration_cast<std::chrono::seconds>(now - gClients[i].lastHeard);
        if (quietFor.count() >= NET_TIMEOUT_SECONDS) {
            std::printf("[server] client %08x silent for %ds, disconnecting it\n",
                        gClients[i].sender, NET_TIMEOUT_SECONDS);
            gClients[i] = serverClient{};
        } else {
            alive++;
        }
    }
    return alive;
}

static void serverSend(netSocketT sock, const sockaddr_in& to, uint32_t seq,
                       uint32_t type, uint32_t value, const char* text) {
    netPacket out;
    std::memset(&out, 0, sizeof(out));
    out.seq = seq;
    out.type = type;
    out.sender = getInstanceNumber();
    out.value = value;
    std::strncpy(out.id, getInstanceId(), sizeof(out.id) - 1);
    std::strncpy(out.text, text, sizeof(out.text) - 1);

    netPacketToWire(out); // host -> network byte order, last thing before the send
    sendto(sock, (const char*)&out, sizeof(out), 0, (const sockaddr*)&to, sizeof(to));
}

// Builds this tick's snapshot — every client state received during the tick plus
// the server's own player — and sends the identical packet to every client. One
// sequence number for the whole tick, so all copies of a snapshot agree.
static void broadcastSnapshot(netSocketT sock, uint32_t seq) {
    netStatePacket snap;
    std::memset(&snap, 0, sizeof(snap));
    snap.seq = seq;
    snap.type = NET_SNAPSHOT;
    snap.sender = getInstanceNumber();
    snap.tps = NET_TPS;
    snap.count = 0;

    // The server is a player too: its position goes out every tick regardless of
    // whether any client reported in.
    float lx, ly, lz;
    netGetLocalPlayer(lx, ly, lz);
    snap.players[snap.count].playerId = getInstanceNumber();
    netSetPos(snap.players[snap.count], lx, ly, lz);
    snap.count++;

    for (uint32_t i = 0; i < NET_MAX_PLAYERS && snap.count < NET_MAX_PLAYERS; i++) {
        if (!gClients[i].active || !gClients[i].stateThisTick) continue;
        snap.players[snap.count] = gClients[i].state;
        snap.count++;
    }

    // The host is a player too and never receives its own broadcast, so apply the
    // snapshot locally as well — otherwise a server-only instance would render
    // nobody. netApplySnapshot filters out our own entry.
    netApplySnapshot(snap);

    netStatePacket wire = snap;
    netStatePacketToWire(wire);

    for (uint32_t i = 0; i < NET_MAX_PLAYERS; i++) {
        if (!gClients[i].active) continue;
        sendto(sock, (const char*)&wire, sizeof(wire), 0,
               (const sockaddr*)&gClients[i].addr, sizeof(gClients[i].addr));
    }

    // The tick's collection window closes here.
    for (uint32_t i = 0; i < NET_MAX_PLAYERS; i++) gClients[i].stateThisTick = false;
}

static void serverThreadMain() {
    for (uint32_t i = 0; i < NET_MAX_PLAYERS; i++) gClients[i] = serverClient{};
    gHadClients = false; // fresh session, so a previous run cannot close this one

    netSocketT sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == NET_BAD_SOCKET) {
        std::printf("[server] socket() failed\n");
        gServerRunning = false;
        return;
    }

    // Without SO_REUSEADDR the port stays unusable for a while after a restart
    // and the next bind fails for reasons that look like a bug in your code.
    int yes = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(NET_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(sock, (const sockaddr*)&addr, sizeof(addr)) != 0) {
        std::printf("[server] bind to 127.0.0.1:%u failed (already running?)\n",
                    (unsigned)NET_PORT);
        netCloseSocket(sock);
        gServerRunning = false;
        return;
    }

    std::printf("[server] listening on 127.0.0.1:%u as %s at %u tps\n",
                (unsigned)NET_PORT, getInstanceId(), (unsigned)NET_TPS);

    uint32_t outSeq = 0;

    const auto tickPeriod = std::chrono::microseconds(1000000 / NET_TPS);
    auto nextTick = std::chrono::steady_clock::now() + tickPeriod;

    while (gServerRunning) {
        // Wait only until the next tick is due, so sends stay on cadence instead
        // of drifting by however long the last receive happened to block.
        auto now = std::chrono::steady_clock::now();
        auto waitFor = std::chrono::duration_cast<std::chrono::microseconds>(nextTick - now);
        if (waitFor.count() < 0) waitFor = std::chrono::microseconds(0);

        fd_set rd;
        FD_ZERO(&rd);
        FD_SET(sock, &rd);

        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = (long)waitFor.count();

        int ready = select((int)(sock + 1), &rd, nullptr, nullptr, &tv);

        if (ready > 0) {
            // Peek at the header to tell the two packet shapes apart. Both start
            // with seq/type/sender, so type is readable before the full read.
            char buf[sizeof(netStatePacket)];
            sockaddr_in from;
            socklen_t fromLen = sizeof(from);
            int got = recvfrom(sock, buf, sizeof(buf), 0, (sockaddr*)&from, &fromLen);

            if (got == (int)sizeof(netStatePacket)) {
                netStatePacket in;
                std::memcpy(&in, buf, sizeof(in));
                netStatePacketToHost(in);

                if (in.type == NET_STATE && in.count >= 1) {
                    serverClient* c = findOrAddClient(from, in.sender);
                    if (c && netAcceptSeq(in.seq, c->lastSeq, c->haveSeq)) {
                        c->sender = in.sender;
                        c->state = in.players[0];
                        c->state.playerId = in.sender; // trust the address, not the field
                        c->stateThisTick = true;
                    }
                }
            } else if (got == (int)sizeof(netPacket)) {
                netPacket in;
                std::memcpy(&in, buf, sizeof(in));
                netPacketToHost(in);
                in.id[sizeof(in.id) - 1] = '\0';     // never trust a remote string
                in.text[sizeof(in.text) - 1] = '\0';

                serverClient* c = findOrAddClient(from, in.sender);
                if (!c) {
                    std::printf("[server] full, refusing %s\n", in.id);
                } else if (netAcceptSeq(in.seq, c->lastSeq, c->haveSeq)) {
                    c->sender = in.sender;

                    if (in.type == NET_JOIN) {
                        std::printf("[server] JOIN seq %u from %s (%08x)\n",
                                    in.seq, in.id, in.sender);
                        // The tick rate is mandated here: the client adopts this.
                        serverSend(sock, from, ++outSeq, NET_JOIN_ACK, NET_TPS, "welcome");
                    } else if (in.type == NET_PING) {
                        std::printf("[server] PING seq %u from %s: %s\n",
                                    in.seq, in.id, in.text);
                        serverSend(sock, from, ++outSeq, NET_PONG, NET_TPS, "pong");
                    }
                }
            }
        }

        now = std::chrono::steady_clock::now();
        if (now >= nextTick) {
            int alive = expireClients();

            // Everyone who joined has since timed out: close the server rather
            // than leave it broadcasting to nobody. A server that has never had
            // a client keeps waiting.
            if (gHadClients && alive == 0) {
                std::printf("[server] all clients disconnected, closing\n");
                break;
            }

            broadcastSnapshot(sock, ++outSeq);

            nextTick += tickPeriod;
            // After a stall, snap forward rather than burning through a backlog
            // of missed ticks all at once.
            if (nextTick < now) nextTick = now + tickPeriod;
        }
    }

    netCloseSocket(sock);
    netClearPlayerPackets(); // stop drawing bodies for a session that has ended
    gServerRunning = false;  // so isServerThreadRunning() is honest after a timeout
    std::printf("[server] stopped\n");
}

void startServerThread() {
    if (gServerRunning) {
        std::printf("[server] already running\n");
        return;
    }
    // A previous run may have exited on its own (bind failure). Assigning over a
    // still-joinable std::thread calls std::terminate, so reap it first.
    if (gServerThread.joinable()) gServerThread.join();

    gServerRunning = true;
    gServerThread = std::thread(serverThreadMain);
}

void stopServerThread() {
    gServerRunning = false;
    if (gServerThread.joinable()) gServerThread.join();
}

bool isServerThreadRunning() {
    return gServerRunning;
}
