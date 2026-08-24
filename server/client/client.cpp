#include "../server_util.h"
#include "client.h"
#include "../net_platform.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>

static std::thread gClientThread;
static std::atomic<bool> gClientRunning(false);

static void clientSend(netSocketT sock, const sockaddr_in& to, uint32_t seq,
                       uint32_t type, const char* text) {
    netPacket out;
    std::memset(&out, 0, sizeof(out));
    out.seq = seq;
    out.type = type;
    out.sender = getInstanceNumber();
    out.value = 0;
    std::strncpy(out.id, getInstanceId(), sizeof(out.id) - 1);
    std::strncpy(out.text, text, sizeof(out.text) - 1);

    netPacketToWire(out); // host -> network byte order, last thing before the send
    sendto(sock, (const char*)&out, sizeof(out), 0, (const sockaddr*)&to, sizeof(to));
}

// This instance's own position, once per tick.
static void clientSendState(netSocketT sock, const sockaddr_in& to, uint32_t seq) {
    netStatePacket out;
    std::memset(&out, 0, sizeof(out));
    out.seq = seq;
    out.type = NET_STATE;
    out.sender = getInstanceNumber();
    out.tps = 0; // the server mandates the rate; a client never asserts one
    out.count = 1;

    float x, y, z;
    netGetLocalPlayer(x, y, z);
    out.players[0].playerId = getInstanceNumber();
    netSetPos(out.players[0], x, y, z);

    netStatePacketToWire(out);
    sendto(sock, (const char*)&out, sizeof(out), 0, (const sockaddr*)&to, sizeof(to));
}

static void clientThreadMain() {
    netSocketT sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == NET_BAD_SOCKET) {
        std::printf("[client] socket() failed\n");
        gClientRunning = false;
        return;
    }

    // No bind: UDP is connectionless, so the OS assigns an ephemeral source port
    // on the first sendto and the server replies to whatever it sees in from.
    sockaddr_in server;
    std::memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(NET_PORT);
    server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    uint32_t outSeq = 0;
    uint32_t lastServerSeq = 0;
    bool haveServerSeq = false;

    // Provisional until the JOIN_ACK lands and the server states its rate.
    uint32_t tps = NET_TPS;
    bool tpsFromServer = false;

    std::printf("[client] joining 127.0.0.1:%u as %s\n", (unsigned)NET_PORT, getInstanceId());
    clientSend(sock, server, ++outSeq, NET_JOIN, "hello");

    auto tickPeriod = std::chrono::microseconds(1000000 / tps);
    auto nextTick = std::chrono::steady_clock::now() + tickPeriod;
    auto lastPing = std::chrono::steady_clock::now();

    // Anything arriving from the server refreshes this. Seeded at join so the
    // timeout also covers a server that never answers the JOIN at all.
    auto lastHeard = std::chrono::steady_clock::now();

    while (gClientRunning) {
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
            char buf[sizeof(netStatePacket)];
            sockaddr_in from;
            socklen_t fromLen = sizeof(from);
            int got = recvfrom(sock, buf, sizeof(buf), 0, (sockaddr*)&from, &fromLen);

            // Any well-formed datagram proves the server is alive, whatever it
            // was — snapshots at NET_TPS carry the liveness, the ping only covers
            // a lull with no state traffic.
            if (got == (int)sizeof(netStatePacket) || got == (int)sizeof(netPacket))
                lastHeard = std::chrono::steady_clock::now();

            if (got == (int)sizeof(netStatePacket)) {
                netStatePacket in;
                std::memcpy(&in, buf, sizeof(in));
                netStatePacketToHost(in);

                if (in.type == NET_SNAPSHOT &&
                    netAcceptSeq(in.seq, lastServerSeq, haveServerSeq)) {
                    // The only thing that ever moves the spawned players.
                    netApplySnapshot(in);
                }
            } else if (got == (int)sizeof(netPacket)) {
                netPacket in;
                std::memcpy(&in, buf, sizeof(in));
                netPacketToHost(in);
                in.id[sizeof(in.id) - 1] = '\0';     // never trust a remote string
                in.text[sizeof(in.text) - 1] = '\0';

                if (netAcceptSeq(in.seq, lastServerSeq, haveServerSeq)) {
                    if (in.type == NET_JOIN_ACK) {
                        // Adopt the server's rate rather than the local constant.
                        if (in.value > 0 && in.value <= 240) {
                            tps = in.value;
                            tickPeriod = std::chrono::microseconds(1000000 / tps);
                            nextTick = std::chrono::steady_clock::now() + tickPeriod;
                            tpsFromServer = true;
                        }
                        std::printf("[client] JOIN_ACK from %s (%08x): %s, %u tps%s\n",
                                    in.id, in.sender, in.text, (unsigned)tps,
                                    tpsFromServer ? " (server mandated)" : "");
                    } else if (in.type == NET_PONG) {
                        std::printf("[client] PONG seq %u from %s: %s\n",
                                    in.seq, in.id, in.text);
                    }
                }
            }
        }

        now = std::chrono::steady_clock::now();
        if (now >= nextTick) {
            clientSendState(sock, server, ++outSeq);

            nextTick += tickPeriod;
            if (nextTick < now) nextTick = now + tickPeriod; // snap forward after a stall
        }

        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastPing).count()
                >= NET_PING_INTERVAL_SECONDS) {
            lastPing = now;
            char text[64];
            std::snprintf(text, sizeof(text), "ping %u", outSeq + 1);
            clientSend(sock, server, ++outSeq, NET_PING, text);
        }

        // Server has gone silent: tear the connection down rather than sitting
        // here sending into a void.
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastHeard).count()
                >= NET_TIMEOUT_SECONDS) {
            std::printf("[client] server silent for %ds, disconnecting\n", NET_TIMEOUT_SECONDS);
            break;
        }
    }

    netCloseSocket(sock);
    netClearPlayerPackets(); // stop drawing bodies for a session that has ended
    gClientRunning = false;  // so isClientThreadRunning() is honest after a timeout
    std::printf("[client] stopped\n");
}

void startClientThread() {
    if (gClientRunning) {
        std::printf("[client] already running\n");
        return;
    }
    // Reap a previous run that exited on its own — assigning over a joinable
    // std::thread calls std::terminate.
    if (gClientThread.joinable()) gClientThread.join();

    gClientRunning = true;
    gClientThread = std::thread(clientThreadMain);
}

void stopClientThread() {
    gClientRunning = false;
    if (gClientThread.joinable()) gClientThread.join();
}

bool isClientThreadRunning() {
    return gClientRunning;
}
