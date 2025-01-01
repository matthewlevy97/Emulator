#include "cpu.h"

namespace emulator::gameboy {

CPU::CPU() : microcodeStackLength_(0), microcode_{}, registers_{}
{}

CPU::~CPU() {}

void CPU::ReceiveTick()
{
    // Pipeline executes fetch on same cycle as end of execute
    // Decode and execute are same cycle(s) on real system
    // Fetch and decode are same cycle(s) when emulating

    if (microcodeStackLength_ > 0) {
        auto microcode = microcode_[--microcodeStackLength_];
        if (microcode != nullptr) {
            microcode_[microcodeStackLength_] = nullptr;
            microcode(this);
        }
    }

    if (microcodeStackLength_ == 0) {
        // Fetch and generate microcode for execution
        auto pc = GetRegister<Registers::PC>();
        auto opcode = bus_->Read<std::uint8_t>(pc);
        SetRegister<Registers::PC>(pc + 1);

        DecodeOpcode(opcode);
    }
}

void CPU::PowerOn() {}

void CPU::PowerOff() {}

void CPU::PushMicrocode(MicroCode code)
{
    if (microcodeStackLength_ >= sizeof(microcode_) / sizeof(MicroCode)) {
        throw std::runtime_error("Not enough space for registering microcode for execution");
    }

    microcode_[microcodeStackLength_++] = code;
}

}; // namespace emulator::gameboy