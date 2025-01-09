#pragma once

#include "debugger.h"
#include "socket/client.h"

#include <string>

namespace emulator::debugger {

class GDBServerConnection {
private:
    struct GDBPacket {
        bool valid;
        std::uint8_t chksum;

        std::string data;
    };

    enum class ConnectionState {
        PRECONNECT,
        HANDSHAKE,
        RUNNING,
        FATAL_ERROR
    } state_;

    Debugger* debugger_;
    socket::DebuggerSocketClient* client_;

    bool QStartNoAckMode_{false};

    bool SendResponse(std::string&) noexcept;
    bool SendResponse(const char*, std::size_t) noexcept;

    std::size_t ExtractPacket(GDBPacket&, std::uint8_t*, std::size_t);
    void ProcessHandshakeMessage();
    void ProcessRunningMessage();

    void HandleQSupportedPacket(GDBPacket&);
    void HandleVCont(GDBPacket& pkt);

public:
    GDBServerConnection(Debugger* debugger, socket::DebuggerSocketClient* client);
    ~GDBServerConnection();

    void ServeWhile(volatile bool&);
};

}; // namespace emulator::debugger