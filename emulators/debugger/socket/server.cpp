#include "server.h"

#include <spdlog/spdlog.h>

#include <math.h>
#include <string>

namespace emulator::debugger::socket {

DebuggerSocketServer::DebuggerSocketServer(std::uint16_t port, bool onlyLocalhost)
{
    server_ = socket::internal::CreateServerSocket(port, onlyLocalhost);
    if (server_ == INVALID_SOCKET) {
        throw std::runtime_error("Failed creating socket");
    }

    SetTimeout(1.0);
}

DebuggerSocketServer::~DebuggerSocketServer()
{
    if (server_ != INVALID_SOCKET) {
        closesocket(server_);
    }
}

void DebuggerSocketServer::SetTimeout(float seconds) noexcept
{
    float secs = 0;
    float usecs = 0;

    usecs = modf(seconds, &secs);

    timeout_.tv_sec = static_cast<long>(secs);
    timeout_.tv_usec = static_cast<long>(usecs * 1000000); // usecs in second
}

DebuggerSocketClient* DebuggerSocketServer::Accept()
{
    if (server_ == INVALID_SOCKET) {
        return nullptr;
    }

    fd_set readable;
    fd_set exceptionable;

    FD_ZERO(&readable);
    FD_ZERO(&exceptionable);
    FD_SET(server_, &readable);
    FD_SET(server_, &exceptionable);

#if defined(_WIN32) || defined(_WIN64)
    // First argument ignore on Windows
    int activity = select(1, &readable, nullptr, &exceptionable, &timeout_);
#else
    int activity = select(server_ + 1, &readable, nullptr, &exceptionable, &timeout_);
#endif
    if (activity < 0) {
        throw std::runtime_error("Select failed");
    }
    
    if (FD_ISSET(server_, &readable)) {
        struct sockaddr_in sa;
        std::memset(&sa, 0, sizeof(sa));
        socklen_t client_len = sizeof(sa);
        SOCKET client = accept(server_, (struct sockaddr*)&sa, &client_len);

        if (spdlog::get_level() <= spdlog::level::info) {
            char s[INET_ADDRSTRLEN];
            inet_ntop(
                sa.sin_family,
                &sa.sin_addr,
                s,
                sizeof(s));
            spdlog::info("New gdbserver connection: {}", s);
        }

        return new DebuggerSocketClient(client);
    }

    if (FD_ISSET(server_, &exceptionable)) {
        // Server closed!
        closesocket(server_);
        server_ = INVALID_SOCKET;
        throw std::runtime_error("Server socket exception");
    }

    return nullptr;
}

}; // namespace emulator::debugger::socket
