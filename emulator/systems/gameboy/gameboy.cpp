#include <fstream>

#include <components/display.h>
#include <components/memory.h>
#include <components/multimappedmemory.h>
#include <emulator.h>

#include "cpu.h"
#include "ppu.h"
#include "debugger.h"
#include "names.h"

emulator::component::System* CreateSystem()
{
    auto cpu = new emulator::gameboy::CPU();
    auto debugger = new emulator::gameboy::Debugger(cpu);

    // TODO: 1) Load actual ROM here
    // TODO: 2) Add UI option to file picker load ROM
    // TODO: 3) Replace BIOS w/ non-copywritten version / allow loading custom BIOS

    auto system = new emulator::component::System(
        "GameBoy",
        4194304, // 4.194304 MHz
        {
            {emulator::gameboy::kDisplayName, new emulator::gameboy::PPU()},

            {emulator::gameboy::kCPUName, cpu},

            // 8 KiB VRAM
            {emulator::gameboy::kVRAMName, new emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(0x8000, 0x2000)},

            // 8 KiB Internal RAM
            {emulator::gameboy::kInternal8KiBRAMName, new emulator::component::MultiMappedMemory<emulator::component::MemoryType::ReadWrite>({{0xC000, 0xE000}, {0xE000, 0xFDFF}}, 0x2000)},

            // Internal RAM
            {emulator::gameboy::kUpperInternalRAMName, new emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(0xFF80, 0x7F)},

            // Cartridge ROMs
            {emulator::gameboy::kCartridgeSwitchableName, new emulator::component::Memory<emulator::component::MemoryType::ReadOnly>(0x4000, 0x4000)},
            {emulator::gameboy::kCartridge0Name, new emulator::component::Memory<emulator::component::MemoryType::ReadOnly>(0, 0x4000)},
        },
        debugger);

    debugger->SetSystem(system);

    system->RegisterFrontendFunction("Load ROM", [cpu](emulator::component::FrontendInterface& frontend) {
        auto selectedFile = frontend.OpenFileDialog();
        if (selectedFile.empty()) {
            frontend.Log("No file selected");
            return;
        }

        std::ifstream file(selectedFile, std::ios::binary);
        if (file.eof()) {
            frontend.Log("Failed to open ROM");
            return;
        }
        std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        frontend.RestartSystem([cpu, buffer]() {
            cpu->LoadStartup();
            cpu->LoadROM(buffer.data(), buffer.size());
        });
        frontend.Log("Loaded ROM");
    });

    return system;
}
