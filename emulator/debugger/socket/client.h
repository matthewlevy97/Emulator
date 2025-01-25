#pragma once

#include "socket_internal.h"
#include <cstddef>

namespace emulator::debugger::socket {

class DebuggerSocketClient {
private:
    struct timeval timeout_;
    SOCKET client_;

public:
    DebuggerSocketClient(SOCKET);
    ~DebuggerSocketClient();

    bool IsReadable() noexcept;
    bool IsWritable() noexcept;

    int ReadAll(std::uint8_t**) noexcept;
    int Write(const std::uint8_t*, std::size_t) noexcept;
};

}; // namespace emulator::debugger::socket