#pragma once

// Socket headers live HERE and are included only by server/*.cpp — never by
// server_util.h, server.h or client.h. windows.h (pulled in by mouse.cpp and
// keyboard.cpp) carries the old winsock.h, which collides head-on with
// winsock2.h if both reach the same translation unit. Keeping these out of the
// public headers means main.cpp never sees either one.

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>

    typedef SOCKET netSocketT;
    #define NET_BAD_SOCKET  INVALID_SOCKET
    #define netCloseSocket  closesocket
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <sys/select.h>
    #include <unistd.h>
    #include <cerrno>

    typedef int netSocketT;
    #define NET_BAD_SOCKET  (-1)
    #define netCloseSocket  close
#endif
