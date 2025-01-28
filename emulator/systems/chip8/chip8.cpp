#include <components/display.h>
#include <components/memory.h>
#include <emulator.h>

#include "cpu.h"

emulator::component::System* CreateSystem()
{
    auto memory = new emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(0, 4096);

    auto system = new emulator::component::System(
        "Chip8",
        1.0f,
        {
            {"Display", new emulator::component::Display(64, 32)},
            {"CPU", new emulator::chip8::CPU()},
            {"Memory", memory},
        });
    return system;
}