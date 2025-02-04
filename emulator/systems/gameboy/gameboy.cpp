#include <components/display.h>
#include <components/memory.h>
#include <components/multimappedmemory.h>
#include <emulator.h>

#include "boot_data.h"
#include "cpu.h"
#include "debugger.h"
#include "names.h"

emulator::component::System* CreateSystem()
{
    auto cpu = new emulator::gameboy::CPU();
    auto debugger = new emulator::gameboy::Debugger(cpu);

    auto cartridge0 = new emulator::component::Memory<emulator::component::MemoryType::ReadOnly>(0, 0x3EFF);
    cartridge0->LoadData((char*)bootData, sizeof(bootData));
    cartridge0->SaveContext(sizeof(bootData)); // Make Context 0 the boot ROM

    auto system = new emulator::component::System(
        "GameBoy",
        1.0f,
        {
            {emulator::gameboy::kDisplayName, new emulator::component::Display(64, 32)},

            {emulator::gameboy::kCPUName, cpu},

            // 8 KiB VRAM
            {emulator::gameboy::kVRAMName, new emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(0x8000, 0x2000)},

            // 8 KiB Internal RAM
            {emulator::gameboy::kInternal8KiBRAMName, new emulator::component::MultiMappedMemory<emulator::component::MemoryType::ReadWrite>({{0xC000, 0xE000}, {0xE000, 0xFDFF}}, 0x2000)},

            // Internal RAM
            {emulator::gameboy::kUpperInternalRAMName, new emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(0xFF80, 0x7F)},

            // Cartridge ROMs
            {emulator::gameboy::kCartridgeSwitchableName, new emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(0x4000, 0x3FFF)},
            {emulator::gameboy::kCartridge0Name, cartridge0},
        },
        debugger);

    debugger->SetSystem(system);

    return system;
}