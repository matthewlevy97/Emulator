#include <components/bus.h>
#include "cpu.h"

namespace emulator::gameboy {

static auto CycleNoOp = [](CPU* cpu) {};

// a + b
static inline bool IsHC(std::uint8_t a, std::uint8_t b)
{
    return (((a & 0xF) + (b & 0xF)) & 0x10) == 0x10;
}

// a + b
static inline bool IsHC(std::uint16_t a, std::uint16_t b)
{
    return (((a & 0xFFF) + (b & 0xFFF)) & 0x1000) == 0x1000;
}

// a - b
static inline bool IsHCSub(std::uint8_t a, std::uint8_t b)
{
    return (((a & 0xF) - (b & 0xF)) & 0x10) == 0x10;
}

// a - b
static inline bool IsHCSub(std::uint16_t a, std::uint16_t b)
{
    return (((a & 0xFFF) - (b & 0xFFF)) & 0x1000) == 0x1000;
}

void CPU::DecodeOpcode(std::uint8_t opcode)
{
    void *scratch = nullptr;
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
    
    // DEC BC
    case 0x0B:
        // 8 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::BC>(cpu->GetRegister<Registers::BC>() - 1);
        });
        PushMicrocode(CycleNoOp);
        break;
    
    // INC B
    case 0x04:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            std::uint8_t reg = cpu->GetRegister<Registers::B>();
            std::uint8_t value = 1 + reg;

            cpu->SetRegister<Registers::B>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::H>(IsHC(reg, 1));
        });
        break;
    
    // DEC B
    case 0x05:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            std::uint8_t reg = cpu->GetRegister<Registers::B>();
            std::uint8_t value = reg - 1;

            cpu->SetRegister<Registers::B>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::H>(IsHCSub(reg, 1));
        });
        break;
    
    // INC C
    case 0x0C:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            std::uint8_t reg = cpu->GetRegister<Registers::C>();
            std::uint8_t value = 1 + reg;

            cpu->SetRegister<Registers::C>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::H>(IsHC(reg, 1));
        });
        break;
    
    // DEC C
    case 0x0D:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            std::uint8_t reg = cpu->GetRegister<Registers::C>();
            std::uint8_t value = reg - 1;

            cpu->SetRegister<Registers::C>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::H>(IsHCSub(reg, 1));
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
    
    // DEC DE
    case 0x1B:
        // 8 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::DE>(cpu->GetRegister<Registers::DE>() - 1);
        });
        PushMicrocode(CycleNoOp);
        break;
    
    // INC D
    case 0x14:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            std::uint8_t reg = cpu->GetRegister<Registers::D>();
            std::uint8_t value = 1 + reg;

            cpu->SetRegister<Registers::D>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::H>(IsHC(reg, 1));
        });
        break;
    
    // DEC D
    case 0x15:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            std::uint8_t reg = cpu->GetRegister<Registers::D>();
            std::uint8_t value = reg - 1;

            cpu->SetRegister<Registers::D>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::H>(IsHCSub(reg, 1));
        });
        break;
    
    // INC E
    case 0x1C:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            std::uint8_t reg = cpu->GetRegister<Registers::E>();
            std::uint8_t value = 1 + reg;

            cpu->SetRegister<Registers::E>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::H>(IsHC(reg, 1));
        });
        break;
    
    // DEC E
    case 0x1D:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            std::uint8_t reg = cpu->GetRegister<Registers::E>();
            std::uint8_t value = reg - 1;

            cpu->SetRegister<Registers::E>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::H>(IsHCSub(reg, 1));
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
    
    // DEC HL
    case 0x2B:
        // 8 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::HL>(cpu->GetRegister<Registers::HL>() - 1);
        });
        PushMicrocode(CycleNoOp);
        break;
    
    // INC H
    case 0x24:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            std::uint8_t reg = cpu->GetRegister<Registers::H>();
            std::uint8_t value = reg + 1;

            cpu->SetRegister<Registers::H>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::H>(IsHC(reg, 1));
        });
        break;
    
    // DEC H
    case 0x25:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            std::uint8_t reg = cpu->GetRegister<Registers::H>();
            std::uint8_t value = reg - 1;

            cpu->SetRegister<Registers::H>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::H>(IsHCSub(reg, 1));
        });
        break;
    
    // INC L
    case 0x2C:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            std::uint8_t reg = cpu->GetRegister<Registers::L>();
            std::uint8_t value = 1 + reg;

            cpu->SetRegister<Registers::L>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::H>(IsHC(reg, 1));
        });
        break;
    
    // DEC L
    case 0x2D:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            std::uint8_t reg = cpu->GetRegister<Registers::L>();
            std::uint8_t value = reg - 1;

            cpu->SetRegister<Registers::L>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::H>(IsHCSub(reg, 1));
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
    
    // DEC SP
    case 0x3B:
        // 8 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::SP>(cpu->GetRegister<Registers::SP>() - 1);
        });
        PushMicrocode(CycleNoOp);
        break;

    // INC A
    case 0x3C:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            std::uint8_t reg = cpu->GetRegister<Registers::A>();
            std::uint8_t value = 1 + reg;

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::H>(IsHC(reg, 1));
        });
        break;

    // DEC A
    case 0x3D:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            std::uint8_t reg = cpu->GetRegister<Registers::A>();
            std::uint8_t value = reg - 1;

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::H>(IsHCSub(reg, 1));
        });
        break;

    // POP BC
    case 0xC1:
        scratch = new std::uint16_t;

        // 12 Cycles
        PushMicrocode([scratch](CPU* cpu) {
            // Set register
            auto s = static_cast<std::uint16_t*>(scratch);
            cpu->SetRegister<Registers::BC>(*s);
            delete s;
        });
        PushMicrocode([scratch](CPU* cpu) {
            // Pop 1-byte for upper
            auto bus = cpu->bus_;
            auto upper = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::SP>());

            auto s = static_cast<std::uint16_t*>(scratch);
            *s = (std::uint16_t(upper) << 8) | std::uint16_t(*s & 0xFF);

            cpu->AddRegister<Registers::SP>(1);
        });
        PushMicrocode([scratch](CPU* cpu) {
            // Pop 1-byte for lower
            auto bus = cpu->bus_;

            auto s = static_cast<std::uint16_t*>(scratch);
            *s = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::SP>());

            cpu->AddRegister<Registers::SP>(1);
        });
        break;
    
    // PUSH BC
    case 0xC5:
        // 16 Cycles
        PushMicrocode(CycleNoOp);
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto val = cpu->GetRegister<Registers::C>();
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::SP>(), val);
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto val = cpu->GetRegister<Registers::B>();
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::SP>(), val);
            cpu->SubRegister<Registers::SP>(1);
        });
        PushMicrocode([scratch](CPU* cpu) {
            cpu->SubRegister<Registers::SP>(1);
        });
        break;
    
    // POP DE
    case 0xD1:
        scratch = new std::uint16_t;

        // 12 Cycles
        PushMicrocode([scratch](CPU* cpu) {
            // Set register
            auto s = static_cast<std::uint16_t*>(scratch);
            cpu->SetRegister<Registers::DE>(*s);
            delete s;
        });
        PushMicrocode([scratch](CPU* cpu) {
            // Pop 1-byte for upper
            auto bus = cpu->bus_;
            auto upper = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::SP>());

            auto s = static_cast<std::uint16_t*>(scratch);
            *s = (std::uint16_t(upper) << 8) | std::uint16_t(*s & 0xFF);

            cpu->AddRegister<Registers::SP>(1);
        });
        PushMicrocode([scratch](CPU* cpu) {
            // Pop 1-byte for lower
            auto bus = cpu->bus_;

            auto s = static_cast<std::uint16_t*>(scratch);
            *s = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::SP>());

            cpu->AddRegister<Registers::SP>(1);
        });
        break;
        
    // PUSH DE
    case 0xD5:
        // 16 Cycles
        PushMicrocode(CycleNoOp);
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto val = cpu->GetRegister<Registers::E>();
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::SP>(), val);
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto val = cpu->GetRegister<Registers::D>();
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::SP>(), val);
            cpu->SubRegister<Registers::SP>(1);
        });
        PushMicrocode([scratch](CPU* cpu) {
            cpu->SubRegister<Registers::SP>(1);
        });
        break;
    
    // POP HL
    case 0xE1:
        scratch = new std::uint16_t;

        // 12 Cycles
        PushMicrocode([scratch](CPU* cpu) {
            // Set register
            auto s = static_cast<std::uint16_t*>(scratch);
            cpu->SetRegister<Registers::HL>(*s);
            delete s;
        });
        PushMicrocode([scratch](CPU* cpu) {
            // Pop 1-byte for upper
            auto bus = cpu->bus_;
            auto upper = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::SP>());

            auto s = static_cast<std::uint16_t*>(scratch);
            *s = (std::uint16_t(upper) << 8) | std::uint16_t(*s & 0xFF);

            cpu->AddRegister<Registers::SP>(1);
        });
        PushMicrocode([scratch](CPU* cpu) {
            // Pop 1-byte for lower
            auto bus = cpu->bus_;

            auto s = static_cast<std::uint16_t*>(scratch);
            *s = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::SP>());

            cpu->AddRegister<Registers::SP>(1);
        });
        break;
    
    // PUSH HL
    case 0xE5:
        // 16 Cycles
        PushMicrocode(CycleNoOp);
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto val = cpu->GetRegister<Registers::L>();
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::SP>(), val);
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto val = cpu->GetRegister<Registers::H>();
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::SP>(), val);
            cpu->SubRegister<Registers::SP>(1);
        });
        PushMicrocode([scratch](CPU* cpu) {
            cpu->SubRegister<Registers::SP>(1);
        });
        break;
    
    // POP AF
    case 0xF1:
        scratch = new std::uint16_t;

        // 12 Cycles
        PushMicrocode([scratch](CPU* cpu) {
            // Set register
            auto s = static_cast<std::uint16_t*>(scratch);
            cpu->SetRegister<Registers::AF>(*s);
            delete s;
        });
        PushMicrocode([scratch](CPU* cpu) {
            // Pop 1-byte for upper
            auto bus = cpu->bus_;
            auto upper = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::SP>());

            auto s = static_cast<std::uint16_t*>(scratch);
            *s = (std::uint16_t(upper) << 8) | std::uint16_t(*s & 0xFF);

            cpu->AddRegister<Registers::SP>(1);
        });
        PushMicrocode([scratch](CPU* cpu) {
            // Pop 1-byte for lower
            auto bus = cpu->bus_;

            auto s = static_cast<std::uint16_t*>(scratch);
            *s = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::SP>());

            cpu->AddRegister<Registers::SP>(1);
        });
        break;

    // PUSH AF
    case 0xF5:
        // 16 Cycles
        PushMicrocode(CycleNoOp);
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto val = cpu->GetRegister<Registers::F>();
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::SP>(), val);
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto val = cpu->GetRegister<Registers::A>();
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::SP>(), val);
            cpu->SubRegister<Registers::SP>(1);
        });
        PushMicrocode([scratch](CPU* cpu) {
            cpu->SubRegister<Registers::SP>(1);
        });
        break;

    default:
        throw std::runtime_error("Unknown opcode");
    }
}

}; // namespace emulator::gameboy
