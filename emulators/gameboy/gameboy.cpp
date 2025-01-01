#include <emulator.h>
#include <components/memory.h>
#include <components/multimappedmemory.h>

#include "cpu.h"

emulator::component::System* CreateSystem() {
    return new emulator::component::System("GameBoy", 1.0f, {
        new emulator::gameboy::CPU(),

         // 8 KiB Internal RAM
        new emulator::component::MultiMappedMemory<emulator::component::MemoryType::ReadWrite>(
            {
                {0xC000, 0xE000},
                {0xE000, 0xFDFF}
            },
            0x2000
        ),

        // Internal RAM
        new emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(0xFF80, 0x7F)
    });
}