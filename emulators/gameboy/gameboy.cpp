#include <emulator.h>
#include <components/memory.h>
#include <components/multimappedmemory.h>

#include "cpu.h"
#include "debugger.h"

emulator::component::System* CreateSystem() {
    auto cpu = new emulator::gameboy::CPU();
    auto debugger = new emulator::gameboy::Debugger(cpu);

    auto system = new emulator::component::System("GameBoy", 1.0f, {
        {"CPU", cpu},

         // 8 KiB Internal RAM
        {"Internal8KiBRAM", new emulator::component::MultiMappedMemory<emulator::component::MemoryType::ReadWrite>(
            {
                {0xC000, 0xE000},
                {0xE000, 0xFDFF}
            },
            0x2000
        )},

        // Internal RAM
        {"UpperInternalRAM", new emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(0xFF80, 0x7F)},

        // Boot ROM
        {"BootROM", new emulator::component::Memory<emulator::component::MemoryType::ReadOnly>{0x0000, 0x1000}},
    }, debugger);

    debugger->SetSystem(system);

    return system;
}