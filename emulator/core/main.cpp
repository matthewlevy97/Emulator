#include <iostream>
#include <spdlog/spdlog.h>

#include "emumanager.h"

#include <debugger/debugger.h>

static const std::array<std::string, 2> emulators = {
    "chip8",
    "gameboy"};

static int RunEmulator(emulator::core::EmulatorManager* manager, std::string name, bool enableDebugger = false)
{
    auto createSystem = manager->GetSystem(name);
    if (!createSystem) {
        spdlog::error("Failed to get system handle for {}", name);
        return -1;
    }
    auto system = createSystem();

    system->PowerOn();

    auto sysDebugger = system->GetDebugger();
    if (enableDebugger && sysDebugger != nullptr) {
        auto& debugger = manager->GetDebugger();
        debugger.RegisterDebugger(sysDebugger);
        debugger.SelectDebugger(sysDebugger->GetName());
        system->UseDebugger(true);
        spdlog::info("Registed {} for remote debugging", sysDebugger->GetName());
    }

    spdlog::info("Starting emulator: {}", system->Name());
    try {
        system->Run();
        spdlog::info("Emulator {} exited", system->Name());
        system->PowerOff();
    } catch (const std::exception& e) {
        spdlog::critical("EXCEPTION: {}", e.what());
        system->LogStacktrace();
    }

    return 0;
}

int main()
{
    spdlog::set_level(spdlog::level::trace);
    spdlog::set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] %v");

    // Load all emulators
    auto manager = new emulator::core::EmulatorManager();
    for (const auto& emulator : emulators) {
        if (!manager->LoadEmulator(emulator)) {
            spdlog::error("Failed loading emulator: {}", emulator);
        } else {
            spdlog::info("Loaded emulator: {}", emulator);
        }
    }

    // Run target emulator
    return RunEmulator(manager, "chip8");
}
