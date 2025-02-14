#include <fstream>

#include <components/display.h>
#include <components/memory.h>
#include <components/timer.h>
#include <components/input.h>

#include <emulator.h>

#include "cpu.h"

static const std::uint8_t fontset[] =
    {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

emulator::component::System* CreateSystem()
{
    auto interpreter = new emulator::component::Memory<emulator::component::MemoryType::ReadOnly>(0, 0x200);
    auto memory = new emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(0x200, 0xE00);

    interpreter->LoadData((const char*)fontset, sizeof(fontset), emulator::chip8::CPU::kFontSetBaseAddress);

    auto input = new emulator::component::Input();
    auto cpu = new emulator::chip8::CPU();

    /*
    1 2 3 C          1 2 3 4
    4 5 6 D   ---\   Q W E R
    7 8 9 E   ---/   A S D F
    A 0 B F          Z X C V
     */
    constexpr static std::uint8_t kKeycodes[] = {
        0x31, 0x32, 0x33, 0x34,
        0x71, 0x77, 0x65, 0x72,
        0x61, 0x73, 0x64, 0x66,
        0x7A, 0x78, 0x63, 0x76,
    };
    for (int i = 0; i < sizeof(kKeycodes); i++) {
        input->RegisterKey(kKeycodes[i]);
    }
    cpu->LoadKeymap(kKeycodes);

    constexpr static std::size_t kBusSpeed = 60 * 8;
    auto system = new emulator::component::System(
        "Chip8",
        kBusSpeed,
        {
            {"Display", new emulator::component::Display(64, 32)},
            {"CPU", cpu},
            {"Memory", memory},
            {"Interpreter", interpreter},
            {"Delay", new emulator::component::Timer("Delay", kBusSpeed / 60)}, // Bus/CPU tick speed, 60Hz * 8
            {"Sound", new emulator::component::Timer("Sound", kBusSpeed / 60)}, // Bus/CPU tick speed, 60Hz * 8
            {"Input", input},
        });

    system->RegisterFrontendFunction("Load ROM", [memory](emulator::component::FrontendInterface& frontend) {
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

        frontend.RestartSystem([memory, buffer]() {
            memory->LoadData(buffer.data(), buffer.size());
        });
        frontend.Log("Loaded ROM");
    });
    
    return system;
}
