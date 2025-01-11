#pragma once

#include <string>
#include <unordered_map>

#include <components/system.h>
#include <debugger/debugger.h>

namespace emulator::core {

class EmulatorManager {
private:
    #ifdef _WIN32
        using SO_HANDLE = HMODULE;
    #else
        using SO_HANDLE = void*;
    #endif

    std::unordered_map<std::string, SO_HANDLE> loadedEmulators_;

    emulator::debugger::Debugger debugger_;

public:
    EmulatorManager(std::uint16_t debuggerPort = 1234);
    ~EmulatorManager() noexcept;

    bool LoadEmulator(std::string name) noexcept;

    emulator::component::CreateSystemFunc GetSystem(std::string name) noexcept;
    emulator::debugger::Debugger& GetDebugger() noexcept { return debugger_; }
};

}; // namespace emulator::core
