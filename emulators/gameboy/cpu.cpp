#include <string>

#include "cpu.h"

namespace emulator::gameboy {

CPU::CPU() : microcodeStackIdx_(0), microcode_{}, registers_{}
{}

CPU::~CPU() {}

void CPU::ReceiveTick()
{
    // Pipeline executes fetch on same cycle as end of execute
    // Decode and execute are same cycle(s) on real system
    // Fetch and decode are same cycle(s) when emulating

    auto microcode = microcode_[microcodeStackIdx_];
    if (microcode != nullptr) {
        microcode_[microcodeStackIdx_] = nullptr;
        if (microcodeStackIdx_ > 0) {
            microcodeStackIdx_--;
        }

        microcode(this);
    }

    if (microcodeStackIdx_ == 0) {
        // Fetch and generate microcode for execution
        auto pc = GetRegister<Registers::PC>();
        
    }
}

void CPU::PowerOn() {}

void CPU::PowerOff() {}

}; // namespace emulator::gameboy