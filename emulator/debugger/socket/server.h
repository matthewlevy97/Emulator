#pragma once

#include "client.h"
#include "socket_internal.h"

#include <stdexcept>

namespace emulator::debugger::socket
{

class DebuggerSocketServer
{
private:
    struct timeval timeout_;
    SOCKET server_;

public:
    DebuggerSocketServer(std::uint16_t port, bool onlyLocalhost);
    ~DebuggerSocketServer();

    void SetTimeout(float seconds) noexcept;
    DebuggerSocketClient* Accept();
};

}; // namespace emulator::debugger::socket
