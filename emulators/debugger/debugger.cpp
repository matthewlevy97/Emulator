#include "debugger.h"
#include "gdbserver.h"

#include "socket/client.h"
#include "socket/server.h"

namespace emulator::debugger {

Debugger::Debugger(std::uint16_t port, bool onlyLocalhost) : currentDebugger_(nullptr)
{
    serverThread_ = std::thread{[&, this]() {
        runServerThread_ = true;
        socket::DebuggerSocketClient* client = nullptr;
        auto server = socket::DebuggerSocketServer(port, onlyLocalhost);

        while (runServerThread_) {
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

Debugger::~Debugger()
{
    if (runServerThread_) {
        runServerThread_ = false;
        serverThread_.join();
    }

    for (int i = 0; i < debuggers_.size(); i++) {
        auto& debugger = debuggers_[i];

        delete debugger;
        debugger = nullptr;
    }
}

void Debugger::RegisterDebugger(ISystemDebugger* debugger)
{
    debuggers_.push_back(debugger);
}

bool Debugger::SelectDebugger(std::string name)
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