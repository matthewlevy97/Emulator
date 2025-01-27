#include <spdlog/spdlog.h>

#include "emumanager.h"

#include <debugger/debugger.h>
#include <ImGUIOpenGL/frontend.h>

static const std::array<std::string, 2> emulators = {
    "chip8",
    "gameboy"};

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

    auto frontend = new emulator::frontend::imgui_opengl::ImGuiFrontend(manager);

    auto system = frontend->GetSystem("chip8");
    spdlog::info("Starting emulator: {}", system->Name());
    try {
        system->Run();
        spdlog::info("Emulator {} exited", system->Name());
        system->PowerOff();
    } catch (const std::exception& e) {
        spdlog::critical("EXCEPTION: {}", e.what());
        system->LogStacktrace();
    }
}
