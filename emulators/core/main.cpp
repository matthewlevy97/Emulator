#include <iostream>

#include "emumanager.h"

int main()
{
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
