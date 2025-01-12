#include "socket_internal.h"

#include <atomic>
#include <cstring>
#include <string>

namespace emulator::debugger::socket::internal {

#if defined(_WIN32) || defined(_WIN64)
    std::atomic<int> netRefCount;

    bool InitNetworking() noexcept
    {
        if (netRefCount.fetch_add(1, std::memory_order_seq_cst) > 1) {
            return true;
        }

        WSADATA wsaData;
        if (WSAStartup(0x0202, &wsaData) != 0) {
            netRefCount -= 1;
            return false;
        }

        return true;
    }

    void DeinitNetworking() noexcept
    {
        if (netRefCount.fetch_sub(1, std::memory_order_seq_cst) == 1) {
            WSACleanup();
        }
    }
#else
    bool InitNetworking() noexcept { return true; }
    void DeinitNetworking() noexcept {}
#endif

SOCKET CreateServerSocket(std::uint16_t port, bool localhost, int listenConns) noexcept
{
    struct addrinfo hints, *res;

    std::memset(&hints, 0, sizeof(hints)); 
    hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

    std::string portStr = std::to_string(port);
    if (getaddrinfo(
        localhost ? "127.0.0.1" : "0.0.0.0",
        portStr.c_str(),
        &hints,
        &res) != 0) {
        return SOCKET_ERROR;
    }

    SOCKET sock = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == SOCKET_ERROR) {
        freeaddrinfo(res);
        return INVALID_SOCKET;
    }

    int reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)(&reuse), sizeof(reuse)) < 0) {
        freeaddrinfo(res);
        closesocket(sock);
        return INVALID_SOCKET;
    }

    if (bind(sock, res->ai_addr, static_cast<int>(res->ai_addrlen)) < 0) {
        freeaddrinfo(res);
        closesocket(sock);
        return INVALID_SOCKET;
    }

    freeaddrinfo(res);
    res = nullptr;

    if (listen(sock, listenConns) < 0) {
        closesocket(sock);
        return INVALID_SOCKET;
    }

    return sock;
}

}; // namespace emulator::debugger::socket::internal
