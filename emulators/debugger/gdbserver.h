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
    bool SendEmptyResponse() noexcept;
    bool SendOKResponse() noexcept;

    bool isStopped_{false};

    static constexpr int kSIGTRAP = 5;
    enum class StopReason {
        NONE,
        WATCH,
        HWBREAK
    };
    bool SendSignal(std::uint8_t signal, StopReason = StopReason::NONE);
    bool SendTerminate(std::uint8_t signal);
    bool SendDebugMessage(std::string);

    std::size_t ExtractPacket(GDBPacket&, std::uint8_t*, std::size_t);
    void ProcessHandshakeMessage();
    void ProcessRunningMessage();
    bool ProcessRunningNonPacket(std::uint8_t*, std::size_t&, std::size_t);

    void HandleQSupportedPacket(GDBPacket&);
    void HandleVCont(GDBPacket& pkt);

public:
    GDBServerConnection(Debugger* debugger, socket::DebuggerSocketClient* client);
    ~GDBServerConnection();

    void ServeWhile(volatile bool&);
};

}; // namespace emulator::debugger