#include "cpu.h"

#include <iostream>

namespace emulator::gameboy {

void CPU::DecodeOpcode(std::uint8_t opcode)
{
    switch (opcode)
    {
    case 0x00:
        PushMicrocode([](CPU* cpu) {
            // No-Op
        });
        break;
    }
}

}; // namespace emulator::gameboy
