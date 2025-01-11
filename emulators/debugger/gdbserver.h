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
        SHUTDOWN,
        FATAL_ERROR
    } state_;

    Debugger* debugger_;
    socket::DebuggerSocketClient* client_;

    bool QStartNoAckMode_{false};
    bool alreadyProcessedAck_{false};

    bool SendResponse(std::string&) noexcept;
    bool SendResponse(const char*, std::size_t) noexcept;
    bool SendEmptyResponse() noexcept;
    bool SendOKResponse() noexcept;

    enum class StopReason {
        NONE,
        WATCH,
        HWBREAK
    };
    bool SendSignal(std::uint8_t signal, StopReason = StopReason::NONE) noexcept;
    bool SendTerminate(std::uint8_t signal) noexcept;
    bool SendDebugMessage(std::string) noexcept;
    bool SendError(std::uint8_t) noexcept;

    std::size_t ExtractPacket(GDBPacket&, std::uint8_t*, std::size_t) noexcept;
    void ProcessHandshakeMessage() noexcept;
    void ProcessRunningMessage() noexcept;
    void ProcessRunningPacket(std::uint8_t*, std::size_t, std::size_t&) noexcept;
    void ProcessRunningNotification(std::uint8_t*, std::size_t, std::size_t&) noexcept;
    bool ProcessRunningNonPacket(std::uint8_t*, std::size_t&, std::size_t) noexcept;

    void HandleQSupportedPacket(GDBPacket&) noexcept;
    void HandleVCont(GDBPacket&) noexcept;
    void HandleMemoryInspect(GDBPacket&) noexcept;

public:
    GDBServerConnection(Debugger* debugger, socket::DebuggerSocketClient* client);
    ~GDBServerConnection();

    void ServeWhile(volatile bool&) noexcept;
};

}; // namespace emulator::debugger