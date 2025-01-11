#include "gdbserver.h"

#include <unordered_map>
#include <spdlog/spdlog.h>

namespace emulator::debugger {

GDBServerConnection::GDBServerConnection(Debugger* debugger, socket::DebuggerSocketClient* client) :
    debugger_(debugger), client_(client), state_(ConnectionState::PRECONNECT)
{}

GDBServerConnection::~GDBServerConnection()
{}

void GDBServerConnection::ServeWhile(volatile bool& check) noexcept
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
        case ConnectionState::SHUTDOWN:
            return;
        case ConnectionState::FATAL_ERROR:
            spdlog::critical("Fatal Error handling GDB connection");
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

bool GDBServerConnection::SendSignal(std::uint8_t signal, StopReason reason) noexcept
{
    std::string msg;
    
    switch (reason) {
    case StopReason::NONE:
        msg = std::format("S{:02X}", signal);
        break;
    case StopReason::HWBREAK:
        msg = std::format("T{:02X}hwbreak:", kSIGTRAP);
        break;
    case StopReason::WATCH:
        msg = std::format("T{:02X}watch:{:04X}", kSIGTRAP, 0); // TODO: Get address of watch address
        break;
    }

    spdlog::debug("{}:{} Sending Signal: {}", __FUNCTION__, __LINE__, signal);
    debugger_->GetCurrentDebugger()->HandleSignal(signal);
    return SendResponse(msg);
}

bool GDBServerConnection::SendTerminate(std::uint8_t signal) noexcept
{
    std::string msg = std::format("X{:02X}", signal);
    return SendResponse(msg);
}

bool GDBServerConnection::SendDebugMessage(std::string str) noexcept
{
    if (!debugger_->GetCurrentDebugger()->IsStopped()) {
        return false;
    }

    std::string msg = "O";
    for (const auto& c : str) {
        msg += std::format("{:02x}", static_cast<unsigned char>(c));
    }
    return SendResponse(msg);
}

bool GDBServerConnection::SendError(std::uint8_t code) noexcept
{
    auto msg = std::format("E{:02X}", code);
    return SendResponse(msg);
}

std::size_t GDBServerConnection::ExtractPacket(GDBPacket& pkt, std::uint8_t* buf, std::size_t len) noexcept
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
    return eop + 3;
}

void GDBServerConnection::ProcessHandshakeMessage() noexcept
{
    if (!client_->IsReadable()) {
        return;
    }

    std::uint8_t* buf = nullptr;
    int n = client_->ReadAll(&buf);
    if (n <= 0 || buf == nullptr) {
        if (n == -1) {
            state_ = ConnectionState::SHUTDOWN;
        }
        return;
    }

    std::size_t cursor = 0;
    while (cursor < n && state_ == ConnectionState::HANDSHAKE) {
        if (!QStartNoAckMode_) {
            if (buf[cursor] == '+') [[likely]] {
                cursor++;
                alreadyProcessedAck_ = true;
            } else if (!alreadyProcessedAck_) {
                spdlog::debug("{}:{} Expected Ack Character", __FUNCTION__, __LINE__);
                state_ = ConnectionState::FATAL_ERROR;
                break;
            }
        }

        GDBPacket packet;
        if (cursor+1 < n && buf[cursor] == '$') [[likely]] {
            // Start of Packet
            cursor++;
            cursor += ExtractPacket(packet, buf + cursor, n - cursor);
        } else if (cursor < n) {
            spdlog::debug("{}:{} Expected Packet Start", __FUNCTION__, __LINE__);
            state_ = ConnectionState::FATAL_ERROR;
            break;
        } else {
            break;
        }

        if (!packet.valid) {
            spdlog::debug("{}:{} Invalid Packet", __FUNCTION__, __LINE__);
            state_ = ConnectionState::FATAL_ERROR;
            break;
        }

        if (packet.data == "QStartNoAckMode" ||
            packet.data == "QThreadSuffixSupported") {
            SendOKResponse();
            QStartNoAckMode_ = true;
        } else if (packet.data == "qHostInfo") {
            auto msg = std::format("hostname:emulator;vendor:{}",
                debugger_->GetCurrentDebugger()->GetName()
            );
            SendResponse(msg);
        } else if (packet.data == "qProcessInfo") {
            auto debugger = debugger_->GetCurrentDebugger();
            auto msg = std::format("pid:{};vendor:{}",
                debugger->GetCurrentPID(),
                debugger->GetName()
            );

            SendResponse(msg);
        } else if (packet.data.starts_with("qGetWorkingDir")) {
            // Just say root directory
            SendResponse("2f", 2);
        } else if (packet.data.starts_with("qSupported")) {
            // Handle supported options
            HandleQSupportedPacket(packet);
        } else if (packet.data == "vCont?") {
            // Don't support vCont yet
            SendEmptyResponse();
        } else if (packet.data == "?") {
            SendSignal(kSIGTRAP);
            state_ = ConnectionState::RUNNING;
        } else if (packet.data == "c") {
            SendOKResponse();
            state_ = ConnectionState::RUNNING;
        } else if (packet.data == "QEnableErrorStrings" || packet.data == "qVAttachOrWaitSupported") {
            // Ignore These Packets
            SendEmptyResponse();
        } else {
            spdlog::debug("Unknown Handshake Packet: {}", packet.data);
            SendEmptyResponse();
        }

        alreadyProcessedAck_ = false;
    }

    delete[] buf;
}

void GDBServerConnection::ProcessRunningMessage() noexcept
{
    if (!client_->IsReadable()) {
        return;
    }

    std::uint8_t* buf = nullptr;
    int n = client_->ReadAll(&buf);
    if (n <= 0 || buf == nullptr) {
        if (n == -1) {
            state_ = ConnectionState::SHUTDOWN;
        }
        return;
    }

    std::size_t cursor = 0;
    while (cursor < n && state_ == ConnectionState::RUNNING) {
        if (!QStartNoAckMode_) {
            if (buf[cursor] == '+') [[likely]] {
                cursor++;
                alreadyProcessedAck_ = true;
            } else if (!alreadyProcessedAck_) {
                spdlog::debug("{}:{} Expected Ack Character", __FUNCTION__, __LINE__);
                state_ = ConnectionState::FATAL_ERROR;
                break;
            }
        }

        if (cursor+1 < n && buf[cursor] == '$') [[likely]] {
            // Start of Packet
            cursor++;
            ProcessRunningPacket(buf, n, cursor);
        } else if (cursor+1 < n && buf[cursor] == '%') {
            cursor++;
            ProcessRunningNotification(buf, n, cursor);
        } else {
            // Not a packet so figure out what to do
            if (!ProcessRunningNonPacket(buf, cursor, n)) {
                break;
            }
        }
        alreadyProcessedAck_ = false;
    }

    delete[] buf;
}

void GDBServerConnection::ProcessRunningPacket(std::uint8_t* buf, std::size_t n, std::size_t& cursor) noexcept
{
    GDBPacket packet;
    cursor += ExtractPacket(packet, buf + cursor, n - cursor);

    if (!packet.valid) {
        spdlog::debug("{}:{} Invalid Packet", __FUNCTION__, __LINE__);
        state_ = ConnectionState::FATAL_ERROR;
        return;
    }

    if (packet.data == "qProcessInfo") {
        auto debugger = debugger_->GetCurrentDebugger();
        auto msg = std::format("pid:{};vendor:{}",
            debugger->GetCurrentPID(),
            debugger->GetName()
        );
        SendResponse(msg);
    } else if (packet.data == "qfThreadInfo") {
        if (debugger_->GetCurrentDebugger()->IsStopped()) {
            SendResponse("l", 1);
        } else {
            SendResponse("m1", 2);
        }
    } else if (packet.data == "qsThreadInfo") {
        SendResponse("l", 1);
    } else if (packet.data.starts_with("qRegisterInfo")) {
        auto regNum = std::strtol(packet.data.c_str() + sizeof("qRegisterInfo"), nullptr, 16);
        auto regInfo = debugger_->GetCurrentDebugger()->GetRegisterInfo(regNum);
        if (regInfo == nullptr) {
            SendError(1);
        } else {
            auto regStr = regInfo->ToString();
            SendResponse(regStr);
        }
    } else if (packet.data == "qC") {
        std::string msg = std::format("QC {}", debugger_->GetCurrentDebugger()->GetCurrentPID());
        SendResponse(msg);
    } else if (packet.data == "?") {
        SendSignal(kSIGTRAP);
    } else if (packet.data == "k") {
        state_ = ConnectionState::SHUTDOWN;
        SendEmptyResponse();
    } else if (packet.data == "c") {
        debugger_->GetCurrentDebugger()->RunCPU();
        SendOKResponse();
    } else if (packet.data == "s") {
        debugger_->GetCurrentDebugger()->StepCPU(1, [this]() {
            this->SendSignal(kSIGTRAP);
        });
    } else if (packet.data.starts_with("vCont")) {
        HandleVCont(packet);
    } else if (packet.data[0] == 'm' || packet.data[0] == 'x') {
        // Memory inspect
        HandleMemoryInspect(packet);
    } else {
        spdlog::debug("Unknown MainLoop Packet: {}", packet.data);
        SendEmptyResponse();
    }
}

void GDBServerConnection::ProcessRunningNotification(std::uint8_t* buf, std::size_t n, std::size_t& cursor) noexcept
{
    GDBPacket packet;
    cursor += ExtractPacket(packet, buf + cursor, n - cursor);

    if (!packet.valid) {
        spdlog::debug("{}:{} Invalid Packet", __FUNCTION__, __LINE__);
        state_ = ConnectionState::FATAL_ERROR;
        return;
    }

    spdlog::trace("{}:{} Recv Notification: {}", __FUNCTION__, __LINE__, packet.data);
}

bool GDBServerConnection::ProcessRunningNonPacket(std::uint8_t* buf, std::size_t& cursor, std::size_t len) noexcept
{
    if ((int)buf[cursor] == 0x03) {
        spdlog::trace("GDBServerConnection Recv: CTRL+C");
        cursor++;
        return SendSignal(kSIGTRAP);
    }

    return false;
}

void GDBServerConnection::HandleQSupportedPacket(GDBPacket& pkt) noexcept
{
    // Request: qSupported [:gdbfeature [;gdbfeature]...]
    std::unordered_map<std::string, std::string> kv;

    char *dup, *tmp, *token;
    dup = tmp = strdup(pkt.data.c_str());

    while (*tmp != '\0' && *tmp != ':') tmp++;
    if (*tmp == '\0') {
        // No Options
        spdlog::debug("qSupported Packet contains no options");
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

    static std::unordered_map<std::string, bool> supportedFeatures = {
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
        spdlog::trace("GDBServerConnection::HandleQSupportedPacket Requested Options: {} = {}", opt.first, opt.second);

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

void GDBServerConnection::HandleVCont(GDBPacket& pkt) noexcept
{
    auto cmd = pkt.data.find_first_of(";");
    if (cmd == std::string::npos) {
        spdlog::debug("{}:{} Missing ';'", __FUNCTION__, __LINE__);
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
        
        spdlog::trace("{}:{} Received Control Action: {}", __FUNCTION__, __LINE__, action);
        actions.push_back(action);
    }

    // Only support 1 thread for now
    if (actions.size() < 1) {
        // Just resume
        debugger_->GetCurrentDebugger()->RunCPU();
        SendOKResponse();
    } else {
        if (actions[0].starts_with("c")) {
            debugger_->GetCurrentDebugger()->RunCPU();
            SendOKResponse();
        } else if (actions[0].starts_with("s")) {
            // Single step CPU
            debugger_->GetCurrentDebugger()->StepCPU(1, [this]() {
                this->SendSignal(kSIGTRAP);
            });
        } else if (actions[0].starts_with("t")) {
            SendSignal(kSIGTRAP);
        }
    }
}

void GDBServerConnection::HandleMemoryInspect(GDBPacket& pkt) noexcept
{
    char* endptr = nullptr;
    auto addr = std::strtoll(pkt.data.c_str() + 1, &endptr, 16);
    std::size_t length = std::strtoll(endptr+1, nullptr, 16);

    // Read memory from system
    auto memory = debugger_->GetCurrentDebugger()->ReadMemory(addr, length);
    if (memory == nullptr) {
        if (pkt.data[0] == 'x') SendError(1);
        else SendEmptyResponse();
    }

    std::string hexStr = pkt.data[0] == 'm' ? "" : "b ";
    for (std::size_t i = 0; i < length; i++) {
        hexStr += std::format("{:02x}", memory[i]);
    }
    SendResponse(hexStr);
}

}; // namespace emulator::debugger