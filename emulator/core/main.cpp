#include <spdlog/spdlog.h>

#include "emumanager.h"

#include <frontend.h>
#include <debugger/debugger.h>

static const std::array<std::string, 2> emulators = {
    "chip8",
    "gameboy"};

static emulator::component::System* GetSystem(emulator::core::EmulatorManager* manager,
                                              std::string name,
                                              bool enableDebugger = false) noexcept
{
    auto createSystem = manager->GetSystem(name);
    if (!createSystem) {
        spdlog::error("Failed to get system handle for {}", name);
        return nullptr;
    }
    auto system = createSystem();

    auto sysDebugger = system->GetDebugger();
    if (sysDebugger != nullptr) {
        auto& debugger = manager->GetDebugger();
        debugger.RegisterDebugger(sysDebugger);
        debugger.SelectDebugger(sysDebugger->GetName());
        system->UseDebugger(enableDebugger);
        spdlog::info("Registed debugger for {}", sysDebugger->GetName());
    }

    return system;
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

    auto system = GetSystem(manager, "gameboy");
    auto frontend = emulator::frontend::CreateFrontend(system);
    frontend->Initialize();
    frontend->Run();
}
