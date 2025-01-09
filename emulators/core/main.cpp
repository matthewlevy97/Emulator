#include <iostream>
#include <spdlog/spdlog.h>

#include "emumanager.h"

#include <debugger/debugger.h>

int main()
{
    spdlog::set_level(spdlog::level::debug);

    auto debugger = emulator::debugger::Debugger(1234);
    while(true) continue;

    auto manager = new emulator::core::EmulatorManager();
    if (!manager->LoadEmulator("gameboy")) {
        return -1;
    }

    auto createSystem = manager->GetSystem("gameboy");
    if (!createSystem) {
        return -1;
    }
    auto system = createSystem();
    std::cout << system->Name() << std::endl;

    system->PowerOn();
    system->Run();

    return 0;
}
