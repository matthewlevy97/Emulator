#pragma once

#include <cstdint>

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN

#pragma comment(lib, "ws2_32.lib")

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using SOCKET = int;

const int INVALID_SOCKET = -1;
const int SOCKET_ERROR = -1;

static inline void closesocket(SOCKET sock) noexcept { close(sock); }
#endif

namespace emulator::debugger::socket::internal
{

bool InitNetworking() noexcept;
void DeinitNetworking() noexcept;

SOCKET CreateServerSocket(std::uint16_t port, bool localhost = false, int listenConns = 5) noexcept;

}; // namespace emulator::debugger::socket::internal
