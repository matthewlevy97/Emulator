#include <emulator.h>
#include <components/memory.h>
#include <components/multimappedmemory.h>

#include "cpu.h"
#include "debugger.h"
#include "boot_data.h"

emulator::component::System* CreateSystem() {
    auto cpu = new emulator::gameboy::CPU();
    auto debugger = new emulator::gameboy::Debugger(cpu);

    auto bootROM = new emulator::component::Memory<emulator::component::MemoryType::ReadOnly>{0x0000, 0x1000};
    bootROM->LoadData((char*)bootData, sizeof(bootData));

    auto system = new emulator::component::System("GameBoy", 1.0f, {
        {"CPU", cpu},

        // 8 KiB VRAM
        {"VRAM", new emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(0x8000, 0x2000)},

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

        // Interrupt Enable Register (TODO: Make memory mapped IO)
        {"InterruptRegister", new emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(0xFFFF, 0x01)},

        // Cartridge ROMs
        {"CartridgeSwitchable", new emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(0x4000, 0x3FFF)},
        {"Cartridge0", new emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(0x0100, 0x3EFF)},

        // Boot ROM
        {"BootROM", bootROM},
    }, debugger);

    debugger->SetSystem(system);

    return system;
}