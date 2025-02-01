#include "debugger.h"
#include "gdbserver.h"

#include "socket/client.h"
#include "socket/server.h"

#include <chrono>

namespace emulator::debugger {

Debugger::Debugger(std::uint16_t port, bool onlyLocalhost)
    : runServerThread_(false), currentDebugger_(nullptr),
    port_(port), onlyLocalhost_(onlyLocalhost)
{}

Debugger::~Debugger()
{
    if (runServerThread_) {
        runServerThread_ = false;

        if (serverThread_.joinable()) {
            serverThread_.join();
        }
    }

    for (int i = 0; i < debuggers_.size(); i++) {
        auto& debugger = debuggers_[i];

        delete debugger;
        debugger = nullptr;
    }
}

void Debugger::Start()
{
    serverThread_ = std::thread{[&, this]() {
        runServerThread_ = true;
        socket::DebuggerSocketClient* client = nullptr;
        auto server = socket::DebuggerSocketServer(port_, onlyLocalhost_);

        while (runServerThread_) {
            if (currentDebugger_ == nullptr) {
                // Prevent doing anything until a system is chosen and debugger loaded
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            if (client == nullptr) {
                client = server.Accept();
            } else {
                auto gdbserver = GDBServerConnection(this, client);
                gdbserver.ServeWhile(runServerThread_);

                delete client;
                client = nullptr;
            }
        }
    }};
}

void Debugger::RegisterDebugger(ISystemDebugger* debugger) noexcept
{
    debuggers_.push_back(debugger);
}

bool Debugger::SelectDebugger(std::string name) noexcept
{
    for (auto debugger : debuggers_) {
        if (debugger->GetName() == name) {
            currentDebugger_ = debugger;
            return true;
        }
    }
    return false;
}

}; // namespace emulator::debugger
