#include "gdbserver.h"

#include <unordered_map>
#include <spdlog/spdlog.h>

namespace emulator::debugger {

GDBServerConnection::GDBServerConnection(Debugger* debugger, socket::DebuggerSocketClient* client) :
    debugger_(debugger), client_(client), state_(ConnectionState::PRECONNECT)
{}

GDBServerConnection::~GDBServerConnection()
{}

void GDBServerConnection::ServeWhile(volatile bool& check)
{
    if (client_ == nullptr) {
        return;
    }

    while(check) {
        switch(state_)
        {
        case ConnectionState::PRECONNECT:
            if (client_->IsReadable()) {
                state_ = ConnectionState::HANDSHAKE;
            }
            break;
        case ConnectionState::HANDSHAKE:
            ProcessHandshakeMessage();
            break;
        case ConnectionState::RUNNING:
            ProcessRunningMessage();
            break;
        case ConnectionState::FATAL_ERROR:
            spdlog::debug("FATAL_ERROR");
            return;
        }
    }
}

bool GDBServerConnection::SendResponse(std::string& str) noexcept
{
    return SendResponse(str.c_str(), str.size());
}

bool GDBServerConnection::SendResponse(const char* str, std::size_t len) noexcept
{
    if (!client_->IsWritable()) {
        return false;
    }

    std::uint8_t chksum = 0;
    for (std::size_t i = 0; i < len; i++) {
        chksum += str[i];
    }

    auto msg = std::format("{}${}#{:02X}", !QStartNoAckMode_ ? "+" : "", str, chksum);
    return client_->Write(reinterpret_cast<const std::uint8_t*>(msg.c_str()), msg.size()) == msg.size();
}

bool GDBServerConnection::SendEmptyResponse() noexcept
{
    return SendResponse("", 0);
}

bool GDBServerConnection::SendOKResponse() noexcept
{
    return SendResponse("OK", 2);
}

std::size_t GDBServerConnection::ExtractPacket(GDBPacket& pkt, std::uint8_t* buf, std::size_t len)
{
    // Scan for end of packet
    std::size_t eop = 0;
    std::uint8_t chksum = 0;
    while(eop < len && buf[eop] != '#') {
        chksum += buf[eop];
        eop++;
    }

    if (eop + 2 >= len) {
        pkt.valid = false;
        return 0;
    }

    // Validate Checksum
    char chksumStr[3] = {
        char(buf[eop+1]),
        char(buf[eop+2]),
        '\0'
    };
    pkt.chksum = std::strtoul(chksumStr, nullptr, 16);
    if (chksum != pkt.chksum) {
        pkt.valid = false;
        return 0;
    }

    pkt.data = std::string((char*)buf, eop);

    pkt.valid = true;
    return eop + 2;
}

void GDBServerConnection::ProcessHandshakeMessage()
{
    if (!client_->IsReadable()) {
        return;
    }

    std::uint8_t* buf = nullptr;
    int n = client_->ReadAll(&buf);

    // Smallest Packet Size: $#00 == 4
    if (n < 4 || buf == nullptr) {
        return;
    }

    if (spdlog::get_level() <= spdlog::level::debug) {
        std::string msg((char*)buf, n);
        spdlog::debug("GDBServerConnection::ProcessHandshakeMessage Recv: {}", msg);
    }

    std::size_t cursor = 0;

    while (cursor + 4 < n) {
        if (!QStartNoAckMode_) {
            if (buf[cursor] == '+') [[likely]] {
                cursor++;
            } else {
                state_ = ConnectionState::FATAL_ERROR;
                return;
            }
        }

        GDBPacket packet;
        if (buf[cursor] == '$') [[likely]] {
            // Start of Packet
            cursor++;
            cursor += ExtractPacket(packet, buf + cursor, n - cursor);
        } else {
            state_ = ConnectionState::FATAL_ERROR;
            return;
        }

        if (!packet.valid) {
            state_ = ConnectionState::FATAL_ERROR;
            return;
        }

        if (packet.data == "QStartNoAckMode" ||
            packet.data == "QThreadSuffixSupported") {
            if (!SendOKResponse()) {
                state_ = ConnectionState::FATAL_ERROR;
                return;
            }
            QStartNoAckMode_ = true;
        } else if (packet.data == "qHostInfo") {
            auto debugger = debugger_->GetCurrentDebugger();
            auto msg = std::format("hostname:emulator;vendor:{}",
                debugger ? debugger->GetName() : "UNKNOWN"
            );

            if (!SendResponse(msg)) {
                state_ = ConnectionState::FATAL_ERROR;
                return;
            }
        } else if (packet.data == "qProcessInfo") {
            auto debugger = debugger_->GetCurrentDebugger();
            auto msg = std::format("pid:{};vendor:{}",
                debugger ? debugger->GetCurrentPID() : 1,
                debugger ? debugger->GetName() : "UNKNOWN"
            );

            if (!SendResponse(msg)) {
                state_ = ConnectionState::FATAL_ERROR;
                return;
            }
        } else if (packet.data.starts_with("qGetWorkingDir")) {
            // Just say root directory
            if (!SendResponse("2f", 2)) {
                state_ = ConnectionState::FATAL_ERROR;
                return;
            }
        } else if (packet.data.starts_with("qSupported")) {
            // Handle supported options
            HandleQSupportedPacket(packet);
        } else if (packet.data == "vCont?") {
            std::string msg = "vCont;c;s;t;r";
            if (!SendResponse(msg)) {
                state_ = ConnectionState::FATAL_ERROR;
                return;
            }
        } else if (packet.data == "?") {
            SendSignal(kSIGTRAP);
            state_ = ConnectionState::RUNNING;
        } else if (packet.data == "c") {
            SendOKResponse();
            state_ = ConnectionState::RUNNING;
        } else if (packet.data == "k") {
            SendEmptyResponse();
        } else if (packet.data == "QEnableErrorStrings" || packet.data == "qVAttachOrWaitSupported") {
            // Ignore These Packets
            SendEmptyResponse();
        } else {
            spdlog::info("Unknown Packet: {}", packet.data);
            SendEmptyResponse();
        }
    }
}

void GDBServerConnection::ProcessRunningMessage()
{
    if (!client_->IsReadable()) {
        return;
    }

    std::uint8_t* buf = nullptr;
    int n = client_->ReadAll(&buf);

    for (int i = 0; i < n; i++)
        spdlog::info("RECV: {}", buf[i]);
    spdlog::info("DONE");

    // Smallest Packet Size: $#00 == 4
    if (n < 4 || buf == nullptr) {
        return;
    }

    if (spdlog::get_level() <= spdlog::level::debug) {
        std::string msg((char*)buf, n);
        spdlog::debug("GDBServerConnection::ProcessRunningMessage Recv: {}", msg);
    }

    std::size_t cursor = 0;

    while (cursor < n) {
        if (!QStartNoAckMode_) {
            if (buf[cursor] == '+') [[likely]] {
                cursor++;
            } else {
                state_ = ConnectionState::FATAL_ERROR;
                return;
            }
        }

        GDBPacket packet;
        if (cursor < n && buf[cursor] == '$') [[likely]] {
            // Start of Packet
            cursor++;
            cursor += ExtractPacket(packet, buf + cursor, n - cursor);
        } else {
            // Not a packet so figure out what to do
            if (!ProcessRunningNonPacket(buf, cursor, n)) {
                return;
            }
            continue;
        }

        if (!packet.valid) {
            state_ = ConnectionState::FATAL_ERROR;
            return;
        }

        if (packet.data == "qProcessInfo") {
            auto debugger = debugger_->GetCurrentDebugger();
            auto msg = std::format("pid:{};vendor:{}",
                debugger ? debugger->GetCurrentPID() : 1,
                debugger ? debugger->GetName() : "UNKNOWN"
            );

            if (!SendResponse(msg)) {
                state_ = ConnectionState::FATAL_ERROR;
                return;
            }
        } else if (packet.data == "qfThreadInfo" || packet.data == "qsThreadInfo") {
            SendEmptyResponse();
        } else if (packet.data == "qC") {
            auto debugger = debugger_->GetCurrentDebugger();
            std::string msg = std::format("QC {}", debugger ? debugger->GetCurrentPID() : 1);
            SendResponse(msg);
        } else if (packet.data == "?") {
            SendSignal(kSIGTRAP);
        } else if (packet.data == "k") {
            SendEmptyResponse();
        } else if (packet.data == "c") {
            SendOKResponse();
        } else if (packet.data.starts_with("vCont")) {
            HandleVCont(packet);
        } else {
            spdlog::info("Unknown Packet: {}", packet.data);
            SendEmptyResponse();
        }
    }
}

bool GDBServerConnection::ProcessRunningNonPacket(std::uint8_t* buf, std::size_t& cursor, std::size_t len)
{
    // TODO: Handle this
    return true;
}

void GDBServerConnection::HandleQSupportedPacket(GDBPacket& pkt)
{
    // Request: qSupported [:gdbfeature [;gdbfeature]...]
    std::unordered_map<std::string, std::string> kv;

    char *dup, *tmp, *token;
    dup = tmp = strdup(pkt.data.c_str());

    while (*tmp != '\0' && *tmp != ':') tmp++;
    if (*tmp == '\0') {
        // No Options
        spdlog::info("qSupported Packet contains no options");
        return;
    }
    tmp++; // Move past ':'

    while ((token = strsep(&tmp, ";")) != nullptr) {
        // Get value
        char* valStart = token;
        while (*valStart != '=' && *valStart != '+' && *valStart != '-' && *valStart != '?' && *valStart != '\0') {
            valStart++;
        }

        if (*valStart == '=') {
            // We have a KV pair
            *valStart = '\0';
            valStart++;
            kv[std::string(token)] = std::string(valStart);
        } else if (*valStart == '+' || *valStart == '-' || *valStart == '?') {
            char c = *valStart;
            *valStart = '\0';
            kv[std::string(token)] = std::to_string(c);
        } else {
            // No KV V=pair
            kv[std::string(token)] = "";
        }
    }

    std::unordered_map<std::string, bool> supportedFeatures = {
        {"QStartNoAckMode", true},
        {"hwbreak", true},
        {"qXfer:memory-map:read", true},
        {"qXfer:osdata:read", true},
        {"qXfer:features:read", true},

        {"fork", false},
        {"vfork", false},
        {"multiprocess", false}
    };

    std::string response = "";
    for (const auto& opt : kv) {
        spdlog::debug("GDBServerConnection::HandleQSupportedPacket Requested Options: {} = {}", opt.first, opt.second);

        auto it = supportedFeatures.find(opt.first);
        if (it != supportedFeatures.end()) {
            response += opt.first + (it->second ? "+" : "-");
        } else {
            // Don't support if not in list
            response += opt.first + "-";
        }
    }

    // Can lead to duplicates, but IDC for now
    for (const auto& opt : supportedFeatures) {
        response += opt.first + (opt.second ? "+" : "-");
    }

    // Send response
    SendResponse(response);
}

void GDBServerConnection::HandleVCont(GDBPacket& pkt)
{
    auto cmd = pkt.data.find_first_of(";");
    if (cmd == std::string::npos) {
        state_ = ConnectionState::FATAL_ERROR;
        return;
    }

    std::stringstream actionStr(pkt.data.substr(cmd));
    std::string action;
    std::vector<std::string> actions;
    while(std::getline(actionStr, action, ';')) {
        if (action.empty()) {
            continue;
        }
        
        spdlog::debug("Received Control Action: {}", action);
        actions.push_back(action);
    }

    // Only support 1 thread for now
    if (actions.size() < 1) {
        // Just resume
        isStopped_ = false;
        SendOKResponse();
    } else {
        if (actions[0].starts_with("c")) {
            isStopped_ = false;
            SendOKResponse();
        } else if (actions[0].starts_with("s")) {
            isStopped_ = false;
            // TODO: Single step CPU
            SendOKResponse();
        } else if (actions[0].starts_with("t")) {
            isStopped_ = true;
            SendSignal(kSIGTRAP);
        }
    }
}

bool GDBServerConnection::SendSignal(std::uint8_t signal, StopReason reason)
{
    std::string msg;
    
    switch (reason) {
    case StopReason::NONE:
        msg = std::format("S {:02X}", signal);
        break;
    case StopReason::HWBREAK:
        msg = std::format("T {:02X} hwbreak:", kSIGTRAP);
        break;
    case StopReason::WATCH:
        msg = std::format("T {:02X} watch:{:04X}", kSIGTRAP, 0); // TODO: Get address of watch address
        break;
    }

    isStopped_ = true;
    return SendResponse(msg);
}

bool GDBServerConnection::SendTerminate(std::uint8_t signal)
{
    std::string msg = std::format("X {:02X}", signal);
    return SendResponse(msg);
}

bool GDBServerConnection::SendDebugMessage(std::string msg)
{
    if (!isStopped_) {
        return false;
    }

    // TODO: O XX… (‘XX…’ is hex encoding of ASCII data)

    return true;
}

}; // namespace emulator::debugger