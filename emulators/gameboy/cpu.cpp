#include <string>

#include "cpu.h"

namespace emulator::gameboy {

CPU::CPU() {
    std::memset(registers_, 0, sizeof(registers_));
}

CPU::~CPU() {}

void CPU::ReceiveTick() {}

void CPU::PowerOn() {}

void CPU::PowerOff() {}

}; // namespace emulator::gameboy