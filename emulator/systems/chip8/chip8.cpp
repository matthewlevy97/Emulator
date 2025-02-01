#include <components/display.h>
#include <components/memory.h>
#include <emulator.h>

#include "cpu.h"

unsigned const char ___Maze__alt___David_Winter__199x__ch8[] = {
  0x60, 0x00, 0x61, 0x00, 0xa2, 0x22, 0xc2, 0x01, 0x32, 0x01, 0xa2, 0x1e,
  0xd0, 0x14, 0x70, 0x04, 0x30, 0x40, 0x12, 0x04, 0x60, 0x00, 0x71, 0x04,
  0x31, 0x20, 0x12, 0x04, 0x12, 0x1c, 0x80, 0x40, 0x20, 0x10, 0x20, 0x40,
  0x80, 0x10
};
unsigned int ___Maze__alt___David_Winter__199x__ch8_len = 38;

emulator::component::System* CreateSystem()
{
    auto interpreter = new emulator::component::Memory<emulator::component::MemoryType::ReadOnly>(0, 0x200);
    auto memory = new emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(0x200, 0xE00);

    memory->LoadData((const char*)___Maze__alt___David_Winter__199x__ch8, ___Maze__alt___David_Winter__199x__ch8_len);

    auto system = new emulator::component::System(
        "Chip8",
        1.0f,
        {
            {"Display", new emulator::component::Display(64, 32)},
            {"CPU", new emulator::chip8::CPU()},
            {"Memory", memory},
            {"Interpreter", interpreter},
        });
    return system;
}
