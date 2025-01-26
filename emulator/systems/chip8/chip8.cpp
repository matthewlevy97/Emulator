#include <emulator.h>
#include <components/memory.h>

#include "cpu.h"

emulator::component::System* CreateSystem() {
    auto memory = new emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(0, 4096);
    
    auto system = new emulator::component::System("Chip8", 1.0f, {
        {"CPU", new emulator::chip8::CPU()},
        {"Memory", memory},
    });
    return system;
}