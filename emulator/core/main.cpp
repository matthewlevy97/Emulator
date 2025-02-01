#include <spdlog/spdlog.h>

#include "emumanager.h"

#include <ImGUIOpenGL/frontend.h>
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

    system->PowerOn();

    auto sysDebugger = system->GetDebugger();
    if (enableDebugger && sysDebugger != nullptr) {
        auto& debugger = manager->GetDebugger();
        debugger.RegisterDebugger(sysDebugger);
        debugger.SelectDebugger(sysDebugger->GetName());
        system->UseDebugger(true);
        spdlog::info("Registed {} for remote debugging", sysDebugger->GetName());
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

    auto system = GetSystem(manager, "chip8");
    auto frontend = new emulator::frontend::imgui_opengl::ImGuiFrontend(system);
    frontend->Initialize();
    frontend->Run();
}
