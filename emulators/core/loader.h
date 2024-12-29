#pragma once

#include <string>

namespace emulator::core {

class EmulatorManager {
private:
    #ifdef _WIN32
    #include <Windows.h>
    #else
    #include <dlfcn.h>
    #endif

    std::vector<
    #ifdef _WIN32
        HMODULE
    #else
        void*
    #endif
    > loadedEmulators_;

public:
    EmulatorManager() {}
    ~EmulatorManager() {}

    bool LoadEmulator(std::string name);
};

}; // namespace emulator::core
