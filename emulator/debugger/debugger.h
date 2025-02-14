#pragma once

#include "sysdebugger.h"

#include <thread>
#include <vector>

namespace emulator::debugger
{

class Debugger
{
private:
    volatile bool runServerThread_;
    std::thread serverThread_;

    std::vector<ISystemDebugger*> debuggers_;
    ISystemDebugger* currentDebugger_;

public:
    Debugger();
    ~Debugger();

    ISystemDebugger* GetCurrentDebugger() const { return currentDebugger_; };

    void StartRemote(std::uint16_t port, bool onlyLocalhost = true);

    void RegisterDebugger(ISystemDebugger*) noexcept;
    bool SelectDebugger(std::string name) noexcept;
};

}; // namespace emulator::debugger
