#pragma once

#include "sysdebugger.h"

#include <thread>
#include <vector>

namespace emulator::debugger {

class Debugger {
private:
    volatile bool runServerThread_;
    std::thread serverThread_;

    std::vector<ISystemDebugger*> debuggers_;
    ISystemDebugger* currentDebugger_;

public:
    Debugger(std::uint16_t port, bool onlyLocalhost = true);
    ~Debugger();

    void RegisterDebugger(ISystemDebugger*);
    ISystemDebugger* GetCurrentDebugger() const { return currentDebugger_; };
    bool SelectDebugger(std::string name);
};

}; // namespace emulator::debugger
