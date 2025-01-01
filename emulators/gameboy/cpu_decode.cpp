#include "cpu.h"

#include <iostream>

namespace emulator::gameboy {

static auto CycleNoOp = [](CPU* cpu) {};

static inline bool IsHC(std::uint8_t a, std::uint8_t b)
{
    return (((a & 0xF) + (b & 0xF)) & 0x10) == 0x10;
}

static inline bool IsHC(std::uint16_t a, std::uint16_t b)
{
    return (((a & 0xFFF) + (b & 0xFFF)) & 0x1000) == 0x1000;
}

void CPU::DecodeOpcode(std::uint8_t opcode)
{
    switch (opcode)
    {
    // No-Op
    case 0x00:
        PushMicrocode(CycleNoOp);
        break;
    
    // INC BC
    case 0x03:
        // 8 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::BC>(1 + cpu->GetRegister<Registers::BC>());
        });
        PushMicrocode(CycleNoOp);
        break;
    
    // INC B
    case 0x04:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto reg = cpu->GetRegister<Registers::B>();
            std::uint16_t value = 1 + reg;

            cpu->SetRegister<Registers::B>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(0);
            cpu->SetFlag<Flags::H>(IsHC(reg, 1));
        });
        break;

    // INC DE
    case 0x13:
        // 8 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::DE>(1 + cpu->GetRegister<Registers::DE>());
        });
        PushMicrocode(CycleNoOp);
        break;
    
    // INC D
    case 0x14:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto reg = cpu->GetRegister<Registers::D>();
            std::uint16_t value = 1 + reg;

            cpu->SetRegister<Registers::D>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(0);
            cpu->SetFlag<Flags::H>(IsHC(reg, 1));
        });
        break;
    
    // INC HL
    case 0x23:
        // 8 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::HL>(1 + cpu->GetRegister<Registers::HL>());
        });
        PushMicrocode(CycleNoOp);
        break;
    
    // INC H
    case 0x24:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto reg = cpu->GetRegister<Registers::H>();
            std::uint16_t value = 1 + reg;

            cpu->SetRegister<Registers::H>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(0);
            cpu->SetFlag<Flags::H>(IsHC(reg, 1));
        });
        break;
    
    // INC SP
    case 0x33:
        // 8 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::SP>(1 + cpu->GetRegister<Registers::SP>());
        });
        PushMicrocode(CycleNoOp);
        break;

    }
}

}; // namespace emulator::gameboy
