#pragma once

#include <string>
#include <unordered_map>

#include <components/system.h>

namespace emulator::core {

class EmulatorManager {
private:
    #ifdef _WIN32
        using SO_HANDLE = HMODULE;
    #else
        using SO_HANDLE = void*;
    #endif

    std::unordered_map<std::string, SO_HANDLE> loadedEmulators_;

public:
    EmulatorManager();
    ~EmulatorManager();

    bool LoadEmulator(std::string name);

    emulator::component::CreateSystemFunc GetSystem(std::string name);
};

}; // namespace emulator::core
