#include "cpu.h"

#include <components/bus.h>

#include <spdlog/spdlog.h>

namespace emulator::gameboy
{

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
    void* scratch = nullptr;
    switch (opcode) {
    // No-Op
    case 0x00:
        PushMicrocode(CycleNoOp);
        break;

    // LD BC, d16
    case 0x01:
        // 12 Cycles
        scratch = new std::uint16_t;
        PushMicrocode([scratch](CPU* cpu) {
            auto tmp = static_cast<std::uint16_t*>(scratch);
            cpu->SetRegister<Registers::BC>(*tmp);
            delete tmp;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint16_t*>(scratch) |= static_cast<std::uint16_t>(bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>())) << 8;
            cpu->AddRegister<Registers::PC>(1);
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint16_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    // LD (BC), A
    case 0x02:
        PushMicrocode(CycleNoOp);
        PushMicrocode([](CPU* cpu) {
            auto bus = cpu->bus_;
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::BC>(),
                                     static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>()));
        });
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
            std::uint8_t reg = static_cast<std::uint8_t>(cpu->GetRegister<Registers::B>());
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
            std::uint8_t reg = static_cast<std::uint8_t>(cpu->GetRegister<Registers::B>());
            std::uint8_t value = reg - 1;

            cpu->SetRegister<Registers::B>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::H>(IsHCSub(reg, 1));
        });
        break;

    // LD B, d8
    case 0x06:
        scratch = new std::uint8_t;

        // 8 Cycles
        PushMicrocode([scratch](CPU* cpu) {
            auto tmp = static_cast<std::uint8_t*>(scratch);
            cpu->SetRegister<Registers::B>(*tmp);
            delete tmp;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint8_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    // RLCA
    case 0x07:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            std::uint8_t val = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            int carry = (val >> 7) & 0x1;
            val = (val << 1) | carry;
            cpu->SetRegister<Registers::A>(val);
            cpu->SetFlag<Flags::Z>(false);
            cpu->SetFlag<Flags::H>(false);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(carry);
        });
        break;

    // LD (u16), SP
    case 0x08:
        scratch = new std::uint16_t;

        // 20 Cycles
        PushMicrocode(CycleNoOp);
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto WZ = static_cast<std::uint16_t*>(scratch);
            auto spMSB = static_cast<std::uint8_t>(cpu->GetRegister<Registers::SP>() >> 8);
            bus->Write<std::uint8_t>(*WZ + 1, spMSB);

            delete WZ;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto WZ = static_cast<std::uint16_t*>(scratch);
            auto spLSB = static_cast<std::uint8_t>(cpu->GetRegister<Registers::SP>() & 0xFF);
            bus->Write<std::uint8_t>(*WZ, spLSB);
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto WZ = static_cast<std::uint16_t*>(scratch);
            *WZ |= static_cast<std::uint16_t>(bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>())) << 8;
            cpu->AddRegister<Registers::PC>(1);
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto WZ = static_cast<std::uint16_t*>(scratch);
            *WZ = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    // ADD HL, BC
    case 0x09:
        // 8 Cycles
        PushMicrocode([](CPU* cpu) {
            auto reg = static_cast<std::uint8_t>(cpu->GetRegister<Registers::H>());
            auto other = static_cast<std::uint8_t>(cpu->GetRegister<Registers::B>());
            std::uint8_t value = reg + other + (cpu->GetFlag<Flags::C>() ? 1 : 0);

            cpu->SetRegister<Registers::H>(value);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(reg > value);
            cpu->SetFlag<Flags::H>(IsHC(reg, other));
        });
        PushMicrocode([](CPU* cpu) {
            auto reg = static_cast<std::uint8_t>(cpu->GetRegister<Registers::L>());
            auto other = static_cast<std::uint8_t>(cpu->GetRegister<Registers::C>());
            std::uint8_t value = reg + other;

            cpu->SetRegister<Registers::L>(value);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(reg > value);
            cpu->SetFlag<Flags::H>(IsHC(reg, other));
        });
        break;

    // LD A, (BC)
    case 0x0A:
        scratch = new std::uint8_t;

        // 8 Cycles
        PushMicrocode([scratch](CPU* cpu) {
            auto tmp = static_cast<std::uint8_t*>(scratch);
            cpu->SetRegister<Registers::A>(*tmp);
            delete tmp;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint8_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::BC>());
        });
        break;

    // DEC BC
    case 0x0B:
        // 8 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::BC>(cpu->GetRegister<Registers::BC>() - 1);
        });
        PushMicrocode(CycleNoOp);
        break;

    // INC C
    case 0x0C:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            std::uint8_t reg = static_cast<std::uint8_t>(cpu->GetRegister<Registers::C>());
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
            std::uint8_t reg = static_cast<std::uint8_t>(cpu->GetRegister<Registers::C>());
            std::uint8_t value = reg - 1;

            cpu->SetRegister<Registers::C>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::H>(IsHCSub(reg, 1));
        });
        break;

    // LD C, d8
    case 0x0E:
        scratch = new std::uint8_t;

        // 8 Cycles
        PushMicrocode([scratch](CPU* cpu) {
            auto tmp = static_cast<std::uint8_t*>(scratch);
            cpu->SetRegister<Registers::C>(*tmp);
            delete tmp;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint8_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    // RRCA
    case 0x0F:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            std::uint8_t val = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            int carry = val & 0x1;
            val = (val >> 1) | (carry << 7);
            cpu->SetRegister<Registers::A>(val);
            cpu->SetFlag<Flags::Z>(false);
            cpu->SetFlag<Flags::H>(false);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(carry);
        });
        break;

    // LD DE, d16
    case 0x11:
        // 12 Cycles
        scratch = new std::uint16_t;
        PushMicrocode([scratch](CPU* cpu) {
            auto tmp = static_cast<std::uint16_t*>(scratch);
            cpu->SetRegister<Registers::DE>(*tmp);
            delete tmp;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint16_t*>(scratch) |= static_cast<std::uint16_t>(bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>())) << 8;
            cpu->AddRegister<Registers::PC>(1);
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint16_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    // LD (DE), A
    case 0x12:
        PushMicrocode(CycleNoOp);
        PushMicrocode([](CPU* cpu) {
            auto bus = cpu->bus_;
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::DE>(),
                                     static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>()));
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
            std::uint8_t reg = static_cast<std::uint8_t>(cpu->GetRegister<Registers::D>());
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
            std::uint8_t reg = static_cast<std::uint8_t>(cpu->GetRegister<Registers::D>());
            std::uint8_t value = reg - 1;

            cpu->SetRegister<Registers::D>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::H>(IsHCSub(reg, 1));
        });
        break;

    // LD D, d8
    case 0x16:
        scratch = new std::uint8_t;

        // 8 Cycles
        PushMicrocode([scratch](CPU* cpu) {
            auto tmp = static_cast<std::uint8_t*>(scratch);
            cpu->SetRegister<Registers::D>(*tmp);
            delete tmp;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint8_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    // RLA
    case 0x17:
        PushMicrocode([](CPU* cpu) {
            std::uint8_t val = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());

            int carry = cpu->GetFlag<Flags::C>();
            cpu->SetFlag<Flags::C>(val >> 7);
            val <<= 1;
            val |= carry;

            cpu->SetRegister<Registers::A>(val);
            cpu->SetFlag<Flags::Z>(false);
            cpu->SetFlag<Flags::H>(false);
            cpu->SetFlag<Flags::N>(false);
        });
        break;

    // JR r8
    case 0x18:
        scratch = new std::uint16_t;
        // Need to actually do parsing
        PushMicrocode([scratch](CPU* cpu) {
            // PC = WZ
            auto WZ = static_cast<std::uint16_t*>(scratch);
            cpu->SetRegister<Registers::PC>(*WZ);
            delete WZ;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto tmp = static_cast<std::uint16_t*>(scratch);

            std::uint8_t Z = *tmp & 0xFF;
            bool sign = (Z >> 7) & 0x1;

            std::uint8_t WZlower = Z + (cpu->GetRegister<Registers::PC>() & 0xFF);
            bool carry = WZlower < Z;
            int adj = (carry && !sign) ? 1 : ((!carry && sign) ? -1 : 0);
            std::uint8_t WZupper = (cpu->GetRegister<Registers::PC>() >> 8) + adj;

            *tmp = (std::uint16_t(WZupper) << 8) | WZlower;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto Z = static_cast<std::uint16_t*>(scratch);

            // Read r8
            auto bus = cpu->bus_;
            *Z = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    // ADD HL, DE
    case 0x19:
        // 8 Cycles
        PushMicrocode([](CPU* cpu) {
            std::uint8_t reg = static_cast<std::uint8_t>(cpu->GetRegister<Registers::H>());
            auto other = static_cast<std::uint8_t>(cpu->GetRegister<Registers::D>());
            std::uint8_t value = reg + other + (cpu->GetFlag<Flags::C>() ? 1 : 0);

            cpu->SetRegister<Registers::H>(value);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(reg > value);
            cpu->SetFlag<Flags::H>(IsHC(reg, other));
        });
        PushMicrocode([](CPU* cpu) {
            std::uint8_t reg = static_cast<std::uint8_t>(cpu->GetRegister<Registers::L>());
            auto other = static_cast<std::uint8_t>(cpu->GetRegister<Registers::E>());
            std::uint8_t value = reg + other;

            cpu->SetRegister<Registers::L>(value);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(reg > value);
            cpu->SetFlag<Flags::H>(IsHC(reg, other));
        });
        break;

    // LD A, (DE)
    case 0x1A:
        scratch = new std::uint8_t;

        // 8 Cycles
        PushMicrocode([scratch](CPU* cpu) {
            auto tmp = static_cast<std::uint8_t*>(scratch);
            cpu->SetRegister<Registers::A>(*tmp);
            delete tmp;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint8_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::DE>());
        });
        break;

    // DEC DE
    case 0x1B:
        // 8 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::DE>(cpu->GetRegister<Registers::DE>() - 1);
        });
        PushMicrocode(CycleNoOp);
        break;

    // INC E
    case 0x1C:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            std::uint8_t reg = static_cast<std::uint8_t>(cpu->GetRegister<Registers::E>());
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
            std::uint8_t reg = static_cast<std::uint8_t>(cpu->GetRegister<Registers::E>());
            std::uint8_t value = reg - 1;

            cpu->SetRegister<Registers::E>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::H>(IsHCSub(reg, 1));
        });
        break;

    // LD E, d8
    case 0x1E:
        scratch = new std::uint8_t;

        // 8 Cycles
        PushMicrocode([scratch](CPU* cpu) {
            auto tmp = static_cast<std::uint8_t*>(scratch);
            cpu->SetRegister<Registers::E>(*tmp);
            delete tmp;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint8_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    // RRA
    case 0x1F:
        PushMicrocode([](CPU* cpu) {
            std::uint8_t val = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());

            int carry = cpu->GetFlag<Flags::C>();
            cpu->SetFlag<Flags::C>(val & 0x1);
            val >>= 1;
            val |= carry << 7;

            cpu->SetRegister<Registers::A>(val);
            cpu->SetFlag<Flags::Z>(false);
            cpu->SetFlag<Flags::H>(false);
            cpu->SetFlag<Flags::N>(false);
        });
        break;

    // JR NZ, r8
    case 0x20:
        scratch = new std::uint16_t;
        PushMicrocode([scratch](CPU* cpu) {
            auto Z = static_cast<std::uint16_t*>(scratch);

            // Read r8
            auto bus = cpu->bus_;
            *Z = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            cpu->AddRegister<Registers::PC>(1);

            if (!cpu->GetFlag<Flags::Z>()) {
                // Need to actually do parsing
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    // PC = WZ
                    auto WZ = static_cast<std::uint16_t*>(scratch);
                    cpu->SetRegister<Registers::PC>(*WZ);
                    delete WZ;
                });

                cpu->PushMicrocode([scratch](CPU* cpu) {
                    auto tmp = static_cast<std::uint16_t*>(scratch);

                    std::uint8_t Z = *tmp & 0xFF;
                    bool sign = (Z >> 7) & 0x1;

                    std::uint8_t WZlower = Z + (cpu->GetRegister<Registers::PC>() & 0xFF);
                    bool carry = WZlower < Z;
                    int adj = (carry && !sign) ? 1 : ((!carry && sign) ? -1 : 0);
                    std::uint8_t WZupper = (cpu->GetRegister<Registers::PC>() >> 8) + adj;

                    *tmp = (std::uint16_t(WZupper) << 8) | WZlower;
                });
            } else {
                // No Jump
                cpu->PushMicrocode(CycleNoOp);
                delete Z;
            }
        });
        break;

    // LD HL, d16
    case 0x21:
        // 12 Cycles
        scratch = new std::uint16_t;
        PushMicrocode([scratch](CPU* cpu) {
            auto tmp = static_cast<std::uint16_t*>(scratch);
            cpu->SetRegister<Registers::HL>(*tmp);
            delete tmp;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint16_t*>(scratch) |= static_cast<std::uint16_t>(bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>())) << 8;
            cpu->AddRegister<Registers::PC>(1);
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint16_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    // LD (HL+), A
    case 0x22:
        PushMicrocode(CycleNoOp);
        PushMicrocode([](CPU* cpu) {
            auto bus = cpu->bus_;
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::HL>(),
                                     static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>()));
            cpu->AddRegister<Registers::HL>(1);
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
            std::uint8_t reg = static_cast<std::uint8_t>(cpu->GetRegister<Registers::H>());
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
            std::uint8_t reg = static_cast<std::uint8_t>(cpu->GetRegister<Registers::H>());
            std::uint8_t value = reg - 1;

            cpu->SetRegister<Registers::H>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::H>(IsHCSub(reg, 1));
        });
        break;

    // LD H, d8
    case 0x26:
        scratch = new std::uint8_t;

        // 8 Cycles
        PushMicrocode([scratch](CPU* cpu) {
            auto tmp = static_cast<std::uint8_t*>(scratch);
            cpu->SetRegister<Registers::H>(*tmp);
            delete tmp;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint8_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    // JR Z, r8
    case 0x28:
        scratch = new std::uint16_t;
        PushMicrocode([scratch](CPU* cpu) {
            auto Z = static_cast<std::uint16_t*>(scratch);

            // Read r8
            auto bus = cpu->bus_;
            *Z = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            cpu->AddRegister<Registers::PC>(1);

            if (cpu->GetFlag<Flags::Z>()) {
                // Need to actually do parsing
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    // PC = WZ
                    auto WZ = static_cast<std::uint16_t*>(scratch);
                    cpu->SetRegister<Registers::PC>(*WZ);
                    delete WZ;
                });

                cpu->PushMicrocode([scratch](CPU* cpu) {
                    auto tmp = static_cast<std::uint16_t*>(scratch);

                    std::uint8_t Z = *tmp & 0xFF;
                    bool sign = (Z >> 7) & 0x1;

                    std::uint8_t WZlower = Z + (cpu->GetRegister<Registers::PC>() & 0xFF);
                    bool carry = WZlower < Z;
                    int adj = (carry && !sign) ? 1 : ((!carry && sign) ? -1 : 0);
                    std::uint8_t WZupper = (cpu->GetRegister<Registers::PC>() >> 8) + adj;

                    *tmp = (std::uint16_t(WZupper) << 8) | WZlower;
                });
            } else {
                // No Jump
                cpu->PushMicrocode(CycleNoOp);
                delete Z;
            }
        });
        break;

    // ADD HL, HL
    case 0x29:
        // 8 Cycles
        PushMicrocode([](CPU* cpu) {
            std::uint8_t reg = static_cast<std::uint8_t>(cpu->GetRegister<Registers::H>());
            auto other = static_cast<std::uint8_t>(cpu->GetRegister<Registers::H>());
            std::uint8_t value = reg + other + (cpu->GetFlag<Flags::C>() ? 1 : 0);

            cpu->SetRegister<Registers::H>(value);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(reg > value);
            cpu->SetFlag<Flags::H>(IsHC(reg, other));
        });
        PushMicrocode([](CPU* cpu) {
            std::uint8_t reg = static_cast<std::uint8_t>(cpu->GetRegister<Registers::L>());
            auto other = static_cast<std::uint8_t>(cpu->GetRegister<Registers::L>());
            std::uint8_t value = reg + other;

            cpu->SetRegister<Registers::L>(value);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(reg > value);
            cpu->SetFlag<Flags::H>(IsHC(reg, other));
        });
        break;

    // LD A, (HL+)
    case 0x2A:
        scratch = new std::uint8_t;

        // 8 Cycles
        PushMicrocode([scratch](CPU* cpu) {
            auto tmp = static_cast<std::uint8_t*>(scratch);
            cpu->SetRegister<Registers::A>(*tmp);
            delete tmp;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint8_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::HL>());

            cpu->AddRegister<Registers::HL>(1);
        });
        break;

    // DEC HL
    case 0x2B:
        // 8 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::HL>(cpu->GetRegister<Registers::HL>() - 1);
        });
        PushMicrocode(CycleNoOp);
        break;

    // INC L
    case 0x2C:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            std::uint8_t reg = static_cast<std::uint8_t>(cpu->GetRegister<Registers::L>());
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
            std::uint8_t reg = static_cast<std::uint8_t>(cpu->GetRegister<Registers::L>());
            std::uint8_t value = reg - 1;

            cpu->SetRegister<Registers::L>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::H>(IsHCSub(reg, 1));
        });
        break;

    // LD L, d8
    case 0x2E:
        scratch = new std::uint8_t;

        // 8 Cycles
        PushMicrocode([scratch](CPU* cpu) {
            auto tmp = static_cast<std::uint8_t*>(scratch);
            cpu->SetRegister<Registers::L>(*tmp);
            delete tmp;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint8_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    // JR NC, r8
    case 0x30:
        scratch = new std::uint16_t;
        PushMicrocode([scratch](CPU* cpu) {
            auto Z = static_cast<std::uint16_t*>(scratch);

            // Read r8
            auto bus = cpu->bus_;
            *Z = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            cpu->AddRegister<Registers::PC>(1);

            if (!cpu->GetFlag<Flags::C>()) {
                // Need to actually do parsing
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    // PC = WZ
                    auto WZ = static_cast<std::uint16_t*>(scratch);
                    cpu->SetRegister<Registers::PC>(*WZ);
                    delete WZ;
                });

                cpu->PushMicrocode([scratch](CPU* cpu) {
                    auto tmp = static_cast<std::uint16_t*>(scratch);

                    std::uint8_t Z = *tmp & 0xFF;
                    bool sign = (Z >> 7) & 0x1;

                    std::uint8_t WZlower = Z + (cpu->GetRegister<Registers::PC>() & 0xFF);
                    bool carry = WZlower < Z;
                    int adj = (carry && !sign) ? 1 : ((!carry && sign) ? -1 : 0);
                    std::uint8_t WZupper = (cpu->GetRegister<Registers::PC>() >> 8) + adj;

                    *tmp = (std::uint16_t(WZupper) << 8) | WZlower;
                });
            } else {
                // No Jump
                cpu->PushMicrocode(CycleNoOp);
                delete Z;
            }
        });
        break;

    // LD SP, d16
    case 0x31:
        // 12 Cycles
        scratch = new std::uint16_t;
        PushMicrocode([scratch](CPU* cpu) {
            auto tmp = static_cast<std::uint16_t*>(scratch);
            cpu->SetRegister<Registers::SP>(*tmp);
            delete tmp;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint16_t*>(scratch) |= static_cast<std::uint16_t>(bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>())) << 8;
            cpu->AddRegister<Registers::PC>(1);
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint16_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    // LD (HL-), A
    case 0x32:
        PushMicrocode(CycleNoOp);
        PushMicrocode([](CPU* cpu) {
            auto bus = cpu->bus_;
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::HL>(),
                                     static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>()));
            cpu->SubRegister<Registers::HL>(1);
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

    // LD (HL), d8
    case 0x36:
        scratch = new std::uint8_t;

        // 12 Cycles
        PushMicrocode(CycleNoOp);
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto tmp = static_cast<std::uint8_t*>(scratch);
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::HL>(), *tmp);
            spdlog::info("Writing");
            delete tmp;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint8_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    // JR C, r8
    case 0x38:
        scratch = new std::uint16_t;
        PushMicrocode([scratch](CPU* cpu) {
            auto Z = static_cast<std::uint16_t*>(scratch);

            // Read r8
            auto bus = cpu->bus_;
            *Z = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            cpu->AddRegister<Registers::PC>(1);

            if (cpu->GetFlag<Flags::C>()) {
                // Need to actually do parsing
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    // PC = WZ
                    auto WZ = static_cast<std::uint16_t*>(scratch);
                    cpu->SetRegister<Registers::PC>(*WZ);
                    delete WZ;
                });

                cpu->PushMicrocode([scratch](CPU* cpu) {
                    auto tmp = static_cast<std::uint16_t*>(scratch);

                    std::uint8_t Z = *tmp & 0xFF;
                    bool sign = (Z >> 7) & 0x1;

                    std::uint8_t WZlower = Z + (cpu->GetRegister<Registers::PC>() & 0xFF);
                    bool carry = WZlower < Z;
                    int adj = (carry && !sign) ? 1 : ((!carry && sign) ? -1 : 0);
                    std::uint8_t WZupper = (cpu->GetRegister<Registers::PC>() >> 8) + adj;

                    *tmp = (std::uint16_t(WZupper) << 8) | WZlower;
                });
            } else {
                // No Jump
                cpu->PushMicrocode(CycleNoOp);
                delete Z;
            }
        });
        break;

    // ADD HL, SP
    case 0x39:
        // 8 Cycles
        PushMicrocode([](CPU* cpu) {
            std::uint8_t reg = static_cast<std::uint8_t>(cpu->GetRegister<Registers::H>());
            auto other = static_cast<std::uint8_t>(cpu->GetRegister<Registers::SP>() >> 8);
            std::uint8_t value = reg + other + (cpu->GetFlag<Flags::C>() ? 1 : 0);

            cpu->SetRegister<Registers::H>(value);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(reg > value);
            cpu->SetFlag<Flags::H>(IsHC(reg, other));
        });
        PushMicrocode([](CPU* cpu) {
            std::uint8_t reg = static_cast<std::uint8_t>(cpu->GetRegister<Registers::L>());
            auto other = static_cast<std::uint8_t>(cpu->GetRegister<Registers::SP>() & 0xFF);
            std::uint8_t value = reg + other;

            cpu->SetRegister<Registers::L>(value);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(reg > value);
            cpu->SetFlag<Flags::H>(IsHC(reg, other));
        });
        break;

    // LD A, (HL-)
    case 0x3A:
        scratch = new std::uint8_t;

        // 8 Cycles
        PushMicrocode([scratch](CPU* cpu) {
            auto tmp = static_cast<std::uint8_t*>(scratch);
            cpu->SetRegister<Registers::A>(*tmp);
            delete tmp;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint8_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::HL>());

            cpu->SubRegister<Registers::HL>(1);
        });
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
            std::uint8_t reg = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
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
            std::uint8_t reg = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            std::uint8_t value = reg - 1;

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::H>(IsHCSub(reg, 1));
        });
        break;

    // LD A, d8
    case 0x3E:
        scratch = new std::uint8_t;

        // 8 Cycles
        PushMicrocode([scratch](CPU* cpu) {
            auto tmp = static_cast<std::uint8_t*>(scratch);
            cpu->SetRegister<Registers::A>(*tmp);
            delete tmp;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint8_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    // LD B, B
    case 0x40:
        // 4 Cycles
        PushMicrocode(CycleNoOp); /* LD self is no-op */
        break;

    // LD B, C
    case 0x41:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::B>(cpu->GetRegister<Registers::C>());
        });
        break;

    // LD B, D
    case 0x42:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::B>(cpu->GetRegister<Registers::D>());
        });
        break;

    // LD B, E
    case 0x43:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::B>(cpu->GetRegister<Registers::E>());
        });
        break;

    // LD B, H
    case 0x44:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::B>(cpu->GetRegister<Registers::H>());
        });
        break;

    // LD B, L
    case 0x45:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::B>(cpu->GetRegister<Registers::L>());
        });
        break;

    // LD B, (HL)
    case 0x46:
        scratch = new std::uint8_t;

        // 8 Cycles
        PushMicrocode([scratch](CPU* cpu) {
            auto tmp = static_cast<std::uint8_t*>(scratch);
            cpu->SetRegister<Registers::B>(*tmp);
            delete tmp;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint8_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::HL>());
        });
        break;

    // LD B, A
    case 0x47:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::B>(cpu->GetRegister<Registers::A>());
        });
        break;

    // LD C, B
    case 0x48:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::C>(cpu->GetRegister<Registers::B>());
        });
        break;

    // LD C, C
    case 0x49:
        // 4 Cycles
        PushMicrocode(CycleNoOp); /* LD self is no-op */
        break;

    // LD C, D
    case 0x4A:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::C>(cpu->GetRegister<Registers::D>());
        });
        break;

    // LD C, E
    case 0x4B:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::C>(cpu->GetRegister<Registers::E>());
        });
        break;

    // LD C, H
    case 0x4C:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::C>(cpu->GetRegister<Registers::H>());
        });
        break;

    // LD C, L
    case 0x4D:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::C>(cpu->GetRegister<Registers::L>());
        });
        break;

    // LD C, (HL)
    case 0x4E:
        scratch = new std::uint8_t;

        // 8 Cycles
        PushMicrocode([scratch](CPU* cpu) {
            auto tmp = static_cast<std::uint8_t*>(scratch);
            cpu->SetRegister<Registers::C>(*tmp);
            delete tmp;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint8_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::HL>());
        });
        break;

    // LD C, A
    case 0x4F:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::C>(cpu->GetRegister<Registers::A>());
        });
        break;

    // LD D, B
    case 0x50:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::D>(cpu->GetRegister<Registers::B>());
        });
        break;

    // LD D, C
    case 0x51:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::D>(cpu->GetRegister<Registers::C>());
        });
        break;

    // LD D, D
    case 0x52:
        // 4 Cycles
        PushMicrocode(CycleNoOp);
        break;

    // LD D, E
    case 0x53:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::D>(cpu->GetRegister<Registers::E>());
        });
        break;

    // LD D, H
    case 0x54:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::D>(cpu->GetRegister<Registers::H>());
        });
        break;

    // LD D, L
    case 0x55:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::D>(cpu->GetRegister<Registers::L>());
        });
        break;

    // LD D, (HL)
    case 0x56:
        scratch = new std::uint8_t;

        // 8 Cycles
        PushMicrocode([scratch](CPU* cpu) {
            auto tmp = static_cast<std::uint8_t*>(scratch);
            cpu->SetRegister<Registers::D>(*tmp);
            delete tmp;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint8_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::HL>());
        });
        break;

    // LD D, A
    case 0x57:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::D>(cpu->GetRegister<Registers::A>());
        });
        break;

    // LD E, B
    case 0x58:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::E>(cpu->GetRegister<Registers::B>());
        });
        break;

    // LD E, C
    case 0x59:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::E>(cpu->GetRegister<Registers::C>());
        });
        break;

    // LD E, D
    case 0x5A:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::E>(cpu->GetRegister<Registers::D>());
        });
        break;

    // LD E, E
    case 0x5B:
        // 4 Cycles
        PushMicrocode(CycleNoOp);
        break;

    // LD E, H
    case 0x5C:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::E>(cpu->GetRegister<Registers::H>());
        });
        break;

    // LD E, L
    case 0x5D:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::E>(cpu->GetRegister<Registers::L>());
        });
        break;

    // LD E, (HL)
    case 0x5E:
        scratch = new std::uint8_t;

        // 8 Cycles
        PushMicrocode([scratch](CPU* cpu) {
            auto tmp = static_cast<std::uint8_t*>(scratch);
            cpu->SetRegister<Registers::E>(*tmp);
            delete tmp;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint8_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::HL>());
        });
        break;

    // LD E, A
    case 0x5F:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::E>(cpu->GetRegister<Registers::A>());
        });
        break;

    // LD H, B
    case 0x60:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::H>(cpu->GetRegister<Registers::B>());
        });
        break;

    // LD H, C
    case 0x61:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::H>(cpu->GetRegister<Registers::C>());
        });
        break;

    // LD H, D
    case 0x62:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::H>(cpu->GetRegister<Registers::D>());
        });
        break;

    // LD H, E
    case 0x63:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::H>(cpu->GetRegister<Registers::E>());
        });
        break;

    // LD H, H
    case 0x64:
        // 4 Cycles
        PushMicrocode(CycleNoOp);
        break;

    // LD H, L
    case 0x65:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::H>(cpu->GetRegister<Registers::L>());
        });
        break;

    // LD H, (HL)
    case 0x66:
        scratch = new std::uint8_t;

        // 8 Cycles
        PushMicrocode([scratch](CPU* cpu) {
            auto tmp = static_cast<std::uint8_t*>(scratch);
            cpu->SetRegister<Registers::H>(*tmp);
            delete tmp;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint8_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::HL>());
        });
        break;

    // LD H, A
    case 0x67:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::H>(cpu->GetRegister<Registers::A>());
        });
        break;

    // LD L, B
    case 0x68:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::L>(cpu->GetRegister<Registers::B>());
        });
        break;

    // LD L, C
    case 0x69:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::L>(cpu->GetRegister<Registers::C>());
        });
        break;

    // LD L, D
    case 0x6A:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::L>(cpu->GetRegister<Registers::D>());
        });
        break;

    // LD L, E
    case 0x6B:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::L>(cpu->GetRegister<Registers::E>());
        });
        break;

    // LD L, H
    case 0x6C:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::L>(cpu->GetRegister<Registers::H>());
        });
        break;

    // LD L, L
    case 0x6D:
        // 4 Cycles
        PushMicrocode(CycleNoOp);
        break;

    // LD L, (HL)
    case 0x6E:
        scratch = new std::uint8_t;

        // 8 Cycles
        PushMicrocode([scratch](CPU* cpu) {
            auto tmp = static_cast<std::uint8_t*>(scratch);
            cpu->SetRegister<Registers::L>(*tmp);
            delete tmp;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint8_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::HL>());
        });
        break;

    // LD L, A
    case 0x6F:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::L>(cpu->GetRegister<Registers::A>());
        });
        break;

    // LD (HL), B
    case 0x70:
        PushMicrocode([](CPU* cpu) {
            auto bus = cpu->bus_;
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::HL>(),
                                     static_cast<std::uint8_t>(cpu->GetRegister<Registers::B>()));
        });
        PushMicrocode(CycleNoOp);
        break;

    // LD (HL), C
    case 0x71:
        PushMicrocode([](CPU* cpu) {
            auto bus = cpu->bus_;
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::HL>(),
                                     static_cast<std::uint8_t>(cpu->GetRegister<Registers::C>()));
        });
        PushMicrocode(CycleNoOp);
        break;

    // LD (HL), D
    case 0x72:
        PushMicrocode([](CPU* cpu) {
            auto bus = cpu->bus_;
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::HL>(),
                                     static_cast<std::uint8_t>(cpu->GetRegister<Registers::D>()));
        });
        PushMicrocode(CycleNoOp);
        break;

    // LD (HL), E
    case 0x73:
        PushMicrocode([](CPU* cpu) {
            auto bus = cpu->bus_;
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::HL>(),
                                     static_cast<std::uint8_t>(cpu->GetRegister<Registers::E>()));
        });
        PushMicrocode(CycleNoOp);
        break;

    // LD (HL), H
    case 0x74:
        PushMicrocode([](CPU* cpu) {
            auto bus = cpu->bus_;
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::HL>(),
                                     static_cast<std::uint8_t>(cpu->GetRegister<Registers::H>()));
        });
        PushMicrocode(CycleNoOp);
        break;

    // LD (HL), L
    case 0x75:
        PushMicrocode([](CPU* cpu) {
            auto bus = cpu->bus_;
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::HL>(),
                                     static_cast<std::uint8_t>(cpu->GetRegister<Registers::L>()));
        });
        PushMicrocode(CycleNoOp);
        break;

    // LD (HL), A
    case 0x77:
        PushMicrocode([](CPU* cpu) {
            auto bus = cpu->bus_;
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::HL>(),
                                     static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>()));
        });
        PushMicrocode(CycleNoOp);
        break;

    // LD A, B
    case 0x78:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::A>(cpu->GetRegister<Registers::B>());
        });
        break;

    // LD A, C
    case 0x79:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::A>(cpu->GetRegister<Registers::C>());
        });
        break;

    // LD A, D
    case 0x7A:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::A>(cpu->GetRegister<Registers::D>());
        });
        break;

    // LD A, E
    case 0x7B:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::A>(cpu->GetRegister<Registers::E>());
        });
        break;

    // LD A, H
    case 0x7C:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::A>(cpu->GetRegister<Registers::H>());
        });
        break;

    // LD A, L
    case 0x7D:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::A>(cpu->GetRegister<Registers::L>());
        });
        break;

    // LD A, (HL)
    case 0x7E:
        scratch = new std::uint8_t;

        // 8 Cycles
        PushMicrocode([scratch](CPU* cpu) {
            auto tmp = static_cast<std::uint8_t*>(scratch);
            cpu->SetRegister<Registers::A>(*tmp);
            delete tmp;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint8_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::HL>());
        });
        break;

    // LD A, A
    case 0x7F:
        // 4 Cycles
        PushMicrocode(CycleNoOp);
        break;

    // ADD A, B
    case 0x80:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            auto other = static_cast<std::uint8_t>(cpu->GetRegister<Registers::B>());
            std::uint8_t value = a + other;

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(value < a);
            cpu->SetFlag<Flags::H>(IsHC(a, other));
        });
        break;

    // ADD A, C
    case 0x81:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            auto other = static_cast<std::uint8_t>(cpu->GetRegister<Registers::C>());
            std::uint8_t value = a + other;

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(value < a);
            cpu->SetFlag<Flags::H>(IsHC(a, other));
        });
        break;

    // ADD A, D
    case 0x82:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            auto other = static_cast<std::uint8_t>(cpu->GetRegister<Registers::D>());
            std::uint8_t value = a + other;

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(value < a);
            cpu->SetFlag<Flags::H>(IsHC(a, other));
        });
        break;

    // ADD A, E
    case 0x83:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            auto other = static_cast<std::uint8_t>(cpu->GetRegister<Registers::E>());
            std::uint8_t value = a + other;

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(value < a);
            cpu->SetFlag<Flags::H>(IsHC(a, other));
        });
        break;

    // ADD A, H
    case 0x84:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            auto other = static_cast<std::uint8_t>(cpu->GetRegister<Registers::H>());
            std::uint8_t value = a + other;

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(value < a);
            cpu->SetFlag<Flags::H>(IsHC(a, other));
        });
        break;

    // ADD A, L
    case 0x85:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            auto other = static_cast<std::uint8_t>(cpu->GetRegister<Registers::L>());
            std::uint8_t value = a + other;

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(value < a);
            cpu->SetFlag<Flags::H>(IsHC(a, other));
        });
        break;

    // ADD A, (HL)
    case 0x86:
        // 8 Cycles
        scratch = new std::uint8_t;
        PushMicrocode([scratch](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            auto tmp = static_cast<std::uint8_t*>(scratch);
            std::uint8_t value = a + *tmp;

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(value < a);
            cpu->SetFlag<Flags::H>(IsHC(a, *tmp));

            delete tmp;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint8_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::HL>());
        });
        break;

    // ADD A, A
    case 0x87:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            auto other = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            std::uint8_t value = a + other;

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(value < a);
            cpu->SetFlag<Flags::H>(IsHC(a, other));
        });
        break;
    
    // SUB A, B
    case 0x90:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            auto other = static_cast<std::uint8_t>(cpu->GetRegister<Registers::B>());
            std::uint8_t value = a - other;

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::C>(a < other);
            cpu->SetFlag<Flags::H>(IsHCSub(a, other));
        });
        break;

    // SUB A, C
    case 0x91:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            auto other = static_cast<std::uint8_t>(cpu->GetRegister<Registers::C>());
            std::uint8_t value = a - other;

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::C>(a < other);
            cpu->SetFlag<Flags::H>(IsHCSub(a, other));
        });
        break;
    
    // SUB A, D
    case 0x92:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            auto other = static_cast<std::uint8_t>(cpu->GetRegister<Registers::D>());
            std::uint8_t value = a - other;

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::C>(a < other);
            cpu->SetFlag<Flags::H>(IsHCSub(a, other));
        });
        break;
    
    // SUB A, E
    case 0x93:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            auto other = static_cast<std::uint8_t>(cpu->GetRegister<Registers::E>());
            std::uint8_t value = a - other;

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::C>(a < other);
            cpu->SetFlag<Flags::H>(IsHCSub(a, other));
        });
        break;
    
    // SUB A, H
    case 0x94:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            auto other = static_cast<std::uint8_t>(cpu->GetRegister<Registers::H>());
            std::uint8_t value = a - other;

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::C>(a < other);
            cpu->SetFlag<Flags::H>(IsHCSub(a, other));
        });
        break;
    
    // SUB A, L
    case 0x95:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            auto other = static_cast<std::uint8_t>(cpu->GetRegister<Registers::L>());
            std::uint8_t value = a - other;

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::C>(a < other);
            cpu->SetFlag<Flags::H>(IsHCSub(a, other));
        });
        break;
    
    // SUB A, A
    case 0x97:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetRegister<Registers::A>(0);
            cpu->SetFlag<Flags::Z>(true);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::C>(false);
            cpu->SetFlag<Flags::H>(false);
        });
        break;

    // AND A, B
    case 0xA0:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            std::uint8_t value = a & cpu->GetRegister<Registers::B>();

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(false);
            cpu->SetFlag<Flags::H>(true);
        });
        break;
    
    // AND A, C
    case 0xA1:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            std::uint8_t value = a & cpu->GetRegister<Registers::C>();

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(false);
            cpu->SetFlag<Flags::H>(true);
        });
        break;
    
    // AND A, D
    case 0xA2:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            std::uint8_t value = a & cpu->GetRegister<Registers::D>();

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(false);
            cpu->SetFlag<Flags::H>(true);
        });
        break;
    
    // AND A, E
    case 0xA3:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            std::uint8_t value = a & cpu->GetRegister<Registers::E>();

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(false);
            cpu->SetFlag<Flags::H>(true);
        });
        break;
    
    // AND A, H
    case 0xA4:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            std::uint8_t value = a & cpu->GetRegister<Registers::H>();

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(false);
            cpu->SetFlag<Flags::H>(true);
        });
        break;
    
    // AND A, L
    case 0xA5:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            std::uint8_t value = a & cpu->GetRegister<Registers::L>();

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(false);
            cpu->SetFlag<Flags::H>(true);
        });
        break;
    
    // AND A, A
    case 0xA7:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            cpu->SetFlag<Flags::Z>(a == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(false);
            cpu->SetFlag<Flags::H>(true);
        });
        break;

    // XOR B
    case 0xA8:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            std::uint8_t value = a ^ cpu->GetRegister<Registers::B>();

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(false);
            cpu->SetFlag<Flags::H>(false);
        });
        break;

    // XOR C
    case 0xA9:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            std::uint8_t value = a ^ cpu->GetRegister<Registers::C>();

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(false);
            cpu->SetFlag<Flags::H>(false);
        });
        break;

    // XOR D
    case 0xAA:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            std::uint8_t value = a ^ cpu->GetRegister<Registers::D>();

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(false);
            cpu->SetFlag<Flags::H>(false);
        });
        break;

    // XOR E
    case 0xAB:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            std::uint8_t value = a ^ cpu->GetRegister<Registers::E>();

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(false);
            cpu->SetFlag<Flags::H>(false);
        });
        break;

    // XOR H
    case 0xAC:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            std::uint8_t value = a ^ cpu->GetRegister<Registers::H>();

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(false);
            cpu->SetFlag<Flags::H>(false);
        });
        break;

    // XOR L
    case 0xAD:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            std::uint8_t value = a ^ cpu->GetRegister<Registers::L>();

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(false);
            cpu->SetFlag<Flags::H>(false);
        });
        break;

    // XOR (HL)
    case 0xAE:
        // 8 Cycles
        scratch = new std::uint8_t;
        PushMicrocode([scratch](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());

            auto tmp = static_cast<std::uint8_t*>(scratch);
            std::uint8_t value = a ^ *tmp;
            delete tmp;

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(false);
            cpu->SetFlag<Flags::H>(false);
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint8_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::HL>());
        });
        break;

    // XOR A
    case 0xAF:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            // A ^ A == 0
            cpu->SetRegister<Registers::A>(0x00);
            cpu->SetFlag<Flags::Z>(true);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(false);
            cpu->SetFlag<Flags::H>(false);
        });
        break;

    // OR A, B
    case 0xB0:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            std::uint8_t value = a | cpu->GetRegister<Registers::B>();

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(false);
            cpu->SetFlag<Flags::H>(false);
        });
        break;
    
    // OR A, C
    case 0xB1:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            std::uint8_t value = a | cpu->GetRegister<Registers::C>();

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(false);
            cpu->SetFlag<Flags::H>(false);
        });
        break;

    // OR A, D
    case 0xB2:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            std::uint8_t value = a | cpu->GetRegister<Registers::D>();

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(false);
            cpu->SetFlag<Flags::H>(false);
        });
        break;

    // OR A, E
    case 0xB3:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            std::uint8_t value = a | cpu->GetRegister<Registers::E>();

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(false);
            cpu->SetFlag<Flags::H>(false);
        });
        break;

    // OR A, H
    case 0xB4:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            std::uint8_t value = a | cpu->GetRegister<Registers::H>();

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(false);
            cpu->SetFlag<Flags::H>(false);
        });
        break;

    // OR A, L
    case 0xB5:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            std::uint8_t value = a | cpu->GetRegister<Registers::L>();

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(false);
            cpu->SetFlag<Flags::H>(false);
        });
        break;

    // OR A, (HL)
    case 0xB6:
        // 8 Cycles
        scratch = new std::uint8_t;
        PushMicrocode([scratch](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());

            auto tmp = static_cast<std::uint8_t*>(scratch);
            std::uint8_t value = a | *tmp;
            delete tmp;

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(false);
            cpu->SetFlag<Flags::H>(false);
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint8_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::HL>());
        });
        break;

    // OR A, A
    case 0xB7:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());

            cpu->SetFlag<Flags::Z>(a == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(false);
            cpu->SetFlag<Flags::H>(false);
        });
        break;

    // CP A, B
    case 0xB8:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            auto other = static_cast<std::uint8_t>(cpu->GetRegister<Registers::B>());
            cpu->SetFlag<Flags::Z>(a == other);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::H>(IsHCSub(a, other));
            cpu->SetFlag<Flags::C>(a < other);
        });
        break;

    // CP A, C
    case 0xB9:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            auto other = static_cast<std::uint8_t>(cpu->GetRegister<Registers::C>());
            cpu->SetFlag<Flags::Z>(a == other);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::H>(IsHCSub(a, other));
            cpu->SetFlag<Flags::C>(a < other);
        });
        break;

    // CP A, D
    case 0xBA:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            auto other = static_cast<std::uint8_t>(cpu->GetRegister<Registers::D>());
            cpu->SetFlag<Flags::Z>(a == other);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::H>(IsHCSub(a, other));
            cpu->SetFlag<Flags::C>(a < other);
        });
        break;

    // CP A, E
    case 0xBB:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            auto other = static_cast<std::uint8_t>(cpu->GetRegister<Registers::E>());
            cpu->SetFlag<Flags::Z>(a == other);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::H>(IsHCSub(a, other));
            cpu->SetFlag<Flags::C>(a < other);
        });
        break;

    // CP A, H
    case 0xBC:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            auto other = static_cast<std::uint8_t>(cpu->GetRegister<Registers::H>());
            cpu->SetFlag<Flags::Z>(a == other);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::H>(IsHCSub(a, other));
            cpu->SetFlag<Flags::C>(a < other);
        });
        break;

    // CP A, L
    case 0xBD:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            auto other = static_cast<std::uint8_t>(cpu->GetRegister<Registers::L>());
            cpu->SetFlag<Flags::Z>(a == other);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::H>(IsHCSub(a, other));
            cpu->SetFlag<Flags::C>(a < other);
        });
        break;
    
    // CP A, (HL)
    case 0xBE:
        // 8 Cycles
        scratch = new std::uint8_t;

        PushMicrocode([scratch](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            auto tmp = static_cast<std::uint8_t*>(scratch);
            cpu->SetFlag<Flags::Z>(a == *tmp);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::H>(IsHCSub(a, *tmp));
            cpu->SetFlag<Flags::C>(a < *tmp);

            delete tmp;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint8_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::HL>());
        });
        break;

    // CP A, A
    case 0xBF:
        // 4 Cycles
        PushMicrocode([](CPU* cpu) {
            cpu->SetFlag<Flags::Z>(true);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::H>(false);
            cpu->SetFlag<Flags::C>(false);
        });
        break;

    // RET NZ
    case 0xC0:
        scratch = new std::uint16_t;

        // Always need a re-fetch cycle
        PushMicrocode(CycleNoOp);

        PushMicrocode([scratch](CPU* cpu) {
            if (!cpu->GetFlag<Flags::Z>()) {
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    // Set register
                    auto s = static_cast<std::uint16_t*>(scratch);
                    cpu->SetRegister<Registers::PC>(*s);
                    delete s;
                });
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    // Pop 1-byte for upper
                    auto bus = cpu->bus_;
                    auto upper = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::SP>());

                    auto s = static_cast<std::uint16_t*>(scratch);
                    *s = (std::uint16_t(upper) << 8) | std::uint16_t(*s & 0xFF);

                    cpu->AddRegister<Registers::SP>(1);
                });
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    // Pop 1-byte for lower
                    auto bus = cpu->bus_;

                    auto s = static_cast<std::uint16_t*>(scratch);
                    *s = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::SP>());

                    cpu->AddRegister<Registers::SP>(1);
                });
            }
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

    // JP NZ, a16
    case 0xC2:
        scratch = new std::uint16_t;

        // 12/16 Cycles
        PushMicrocode(CycleNoOp); // Pipeline refresh
        PushMicrocode([scratch](CPU* cpu) {
            // Pop 1-byte for upper
            auto bus = cpu->bus_;
            auto s = static_cast<std::uint16_t*>(scratch);
            auto upper = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            *s = (std::uint16_t(upper) << 8) | std::uint16_t(*s & 0xFF);

            cpu->AddRegister<Registers::PC>(1);

            // Do actual JUMP is needed
            if (!cpu->GetFlag<Flags::Z>()) {
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    auto newPC = static_cast<std::uint16_t*>(scratch);
                    cpu->SetRegister<Registers::PC>(*newPC);
                    delete newPC;
                });
            } else {
                delete s;
            }
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto s = static_cast<std::uint16_t*>(scratch);
            *s = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());

            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    // JP a16:
    case 0xC3:
        scratch = new std::uint16_t;

        // 16 Cycles
        PushMicrocode(CycleNoOp); // Pipeline refresh
        PushMicrocode([scratch](CPU* cpu) {
            auto newPC = static_cast<std::uint16_t*>(scratch);
            cpu->SetRegister<Registers::PC>(*newPC);
            delete newPC;
        });
        PushMicrocode([scratch](CPU* cpu) {
            // Pop 1-byte for upper
            auto bus = cpu->bus_;
            auto s = static_cast<std::uint16_t*>(scratch);
            auto upper = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            *s = (std::uint16_t(upper) << 8) | std::uint16_t(*s & 0xFF);

            cpu->AddRegister<Registers::PC>(1);
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto s = static_cast<std::uint16_t*>(scratch);
            *s = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());

            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    // CALL NZ, u16
    case 0xC4:
        scratch = new std::uint16_t;

        // 12/24 Cycles
        PushMicrocode(CycleNoOp); // Pipeline refresh
        PushMicrocode([scratch](CPU* cpu) {
            // Pop 1-byte for upper
            auto bus = cpu->bus_;
            auto s = static_cast<std::uint16_t*>(scratch);
            auto upper = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            *s = (std::uint16_t(upper) << 8) | std::uint16_t(*s & 0xFF);

            cpu->AddRegister<Registers::PC>(1);

            // Do actual JUMP is needed
            if (!cpu->GetFlag<Flags::Z>()) {
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    auto bus = cpu->bus_;
                    auto lsb = cpu->GetRegister<Registers::PC>() & 0xFF;
                    bus->Write<std::uint8_t>(cpu->GetRegister<Registers::SP>(), lsb);

                    auto newPC = static_cast<std::uint16_t*>(scratch);
                    cpu->SetRegister<Registers::PC>(*newPC);
                    delete newPC;
                });
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    auto bus = cpu->bus_;
                    auto msb = cpu->GetRegister<Registers::PC>() >> 8;
                    bus->Write<std::uint8_t>(cpu->GetRegister<Registers::SP>(), msb);
                    cpu->SubRegister<Registers::SP>(1);
                });
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    cpu->SubRegister<Registers::SP>(1);
                });
            } else {
                delete s;
            }
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto s = static_cast<std::uint16_t*>(scratch);
            *s = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());

            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    // PUSH BC
    case 0xC5:
        // 16 Cycles
        PushMicrocode(CycleNoOp);
        PushMicrocode([](CPU* cpu) {
            auto bus = cpu->bus_;
            auto val = static_cast<std::uint8_t>(cpu->GetRegister<Registers::C>());
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::SP>(), val);
        });
        PushMicrocode([](CPU* cpu) {
            auto bus = cpu->bus_;
            auto val = static_cast<std::uint8_t>(cpu->GetRegister<Registers::B>());
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::SP>(), val);
            cpu->SubRegister<Registers::SP>(1);
        });
        PushMicrocode([](CPU* cpu) {
            cpu->SubRegister<Registers::SP>(1);
        });
        break;

    // RET Z
    case 0xC8:
        scratch = new std::uint16_t;

        // Always need a re-fetch cycle
        PushMicrocode(CycleNoOp);

        PushMicrocode([scratch](CPU* cpu) {
            if (cpu->GetFlag<Flags::Z>()) {
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    // Set register
                    auto s = static_cast<std::uint16_t*>(scratch);
                    cpu->SetRegister<Registers::PC>(*s);
                    delete s;
                });
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    // Pop 1-byte for upper
                    auto bus = cpu->bus_;
                    auto upper = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::SP>());

                    auto s = static_cast<std::uint16_t*>(scratch);
                    *s = (std::uint16_t(upper) << 8) | std::uint16_t(*s & 0xFF);

                    cpu->AddRegister<Registers::SP>(1);
                });
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    // Pop 1-byte for lower
                    auto bus = cpu->bus_;

                    auto s = static_cast<std::uint16_t*>(scratch);
                    *s = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::SP>());

                    cpu->AddRegister<Registers::SP>(1);
                });
            }
        });
        break;

    // RET
    case 0xC9:
        scratch = new std::uint16_t;

        // Always need a re-fetch cycle
        PushMicrocode(CycleNoOp);
        PushMicrocode([scratch](CPU* cpu) {
            // Set register
            auto s = static_cast<std::uint16_t*>(scratch);
            cpu->SetRegister<Registers::PC>(*s);
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

    // JP Z, a16
    case 0xCA:
        scratch = new std::uint16_t;

        // 12/16 Cycles
        PushMicrocode(CycleNoOp); // Pipeline refresh
        PushMicrocode([scratch](CPU* cpu) {
            // Pop 1-byte for upper
            auto bus = cpu->bus_;
            auto s = static_cast<std::uint16_t*>(scratch);
            auto upper = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            *s = (std::uint16_t(upper) << 8) | std::uint16_t(*s & 0xFF);

            cpu->AddRegister<Registers::PC>(1);

            // Do actual JUMP is needed
            if (cpu->GetFlag<Flags::Z>()) {
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    auto newPC = static_cast<std::uint16_t*>(scratch);
                    cpu->SetRegister<Registers::PC>(*newPC);
                    delete newPC;
                });
            } else {
                delete s;
            }
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto s = static_cast<std::uint16_t*>(scratch);
            *s = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());

            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    // CB Prefix
    case 0xCB:
        DecodeCBOpcode();
        PushMicrocode([](CPU* cpu) {
            // Increment past the CB instruction
            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    // CALL Z, u16
    case 0xCC:
        scratch = new std::uint16_t;

        // 12/24 Cycles
        PushMicrocode(CycleNoOp); // Pipeline refresh
        PushMicrocode([scratch](CPU* cpu) {
            // Pop 1-byte for upper
            auto bus = cpu->bus_;
            auto s = static_cast<std::uint16_t*>(scratch);
            auto upper = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            *s = (std::uint16_t(upper) << 8) | std::uint16_t(*s & 0xFF);

            cpu->AddRegister<Registers::PC>(1);

            // Do actual JUMP is needed
            if (cpu->GetFlag<Flags::Z>()) {
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    auto bus = cpu->bus_;
                    auto lsb = cpu->GetRegister<Registers::PC>() & 0xFF;
                    bus->Write<std::uint8_t>(cpu->GetRegister<Registers::SP>(), lsb);

                    auto newPC = static_cast<std::uint16_t*>(scratch);
                    cpu->SetRegister<Registers::PC>(*newPC);
                    delete newPC;
                });
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    auto bus = cpu->bus_;
                    auto msb = cpu->GetRegister<Registers::PC>() >> 8;
                    bus->Write<std::uint8_t>(cpu->GetRegister<Registers::SP>(), msb);
                    cpu->SubRegister<Registers::SP>(1);
                });
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    cpu->SubRegister<Registers::SP>(1);
                });
            } else {
                delete s;
            }
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto s = static_cast<std::uint16_t*>(scratch);
            *s = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());

            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    // CALL u16
    case 0xCD:
        scratch = new std::uint16_t;

        // 12/24 Cycles
        PushMicrocode(CycleNoOp); // Pipeline refresh
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto lsb = cpu->GetRegister<Registers::PC>() & 0xFF;
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::SP>(), lsb);

            auto newPC = static_cast<std::uint16_t*>(scratch);
            cpu->SetRegister<Registers::PC>(*newPC);
            delete newPC;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto msb = cpu->GetRegister<Registers::PC>() >> 8;
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::SP>(), msb);
            cpu->SubRegister<Registers::SP>(1);
        });
        PushMicrocode([scratch](CPU* cpu) {
            cpu->SubRegister<Registers::SP>(1);
        });
        PushMicrocode([scratch](CPU* cpu) {
            // Pop 1-byte for upper
            auto bus = cpu->bus_;
            auto s = static_cast<std::uint16_t*>(scratch);
            auto upper = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            *s = (std::uint16_t(upper) << 8) | std::uint16_t(*s & 0xFF);

            cpu->AddRegister<Registers::PC>(1);
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto s = static_cast<std::uint16_t*>(scratch);
            *s = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());

            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    // RET NC
    case 0xD0:
        scratch = new std::uint16_t;

        // Always need a re-fetch cycle
        PushMicrocode(CycleNoOp);

        PushMicrocode([scratch](CPU* cpu) {
            if (!cpu->GetFlag<Flags::C>()) {
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    // Set register
                    auto s = static_cast<std::uint16_t*>(scratch);
                    cpu->SetRegister<Registers::PC>(*s);
                    delete s;
                });
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    // Pop 1-byte for upper
                    auto bus = cpu->bus_;
                    auto upper = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::SP>());

                    auto s = static_cast<std::uint16_t*>(scratch);
                    *s = (std::uint16_t(upper) << 8) | std::uint16_t(*s & 0xFF);

                    cpu->AddRegister<Registers::SP>(1);
                });
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    // Pop 1-byte for lower
                    auto bus = cpu->bus_;

                    auto s = static_cast<std::uint16_t*>(scratch);
                    *s = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::SP>());

                    cpu->AddRegister<Registers::SP>(1);
                });
            }
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

    // JP NC, a16
    case 0xD2:
        scratch = new std::uint16_t;

        // 12/16 Cycles
        PushMicrocode(CycleNoOp); // Pipeline refresh
        PushMicrocode([scratch](CPU* cpu) {
            // Pop 1-byte for upper
            auto bus = cpu->bus_;
            auto s = static_cast<std::uint16_t*>(scratch);
            auto upper = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            *s = (std::uint16_t(upper) << 8) | std::uint16_t(*s & 0xFF);

            cpu->AddRegister<Registers::PC>(1);

            // Do actual JUMP is needed
            if (!cpu->GetFlag<Flags::C>()) {
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    auto newPC = static_cast<std::uint16_t*>(scratch);
                    cpu->SetRegister<Registers::PC>(*newPC);
                    delete newPC;
                });
            } else {
                delete s;
            }
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto s = static_cast<std::uint16_t*>(scratch);
            *s = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());

            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    // CALL NC, u16
    case 0xD4:
        scratch = new std::uint16_t;

        // 12/24 Cycles
        PushMicrocode(CycleNoOp); // Pipeline refresh
        PushMicrocode([scratch](CPU* cpu) {
            // Pop 1-byte for upper
            auto bus = cpu->bus_;
            auto s = static_cast<std::uint16_t*>(scratch);
            auto upper = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            *s = (std::uint16_t(upper) << 8) | std::uint16_t(*s & 0xFF);

            cpu->AddRegister<Registers::PC>(1);

            // Do actual JUMP is needed
            if (!cpu->GetFlag<Flags::C>()) {
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    auto bus = cpu->bus_;
                    auto lsb = cpu->GetRegister<Registers::PC>() & 0xFF;
                    bus->Write<std::uint8_t>(cpu->GetRegister<Registers::SP>(), lsb);

                    auto newPC = static_cast<std::uint16_t*>(scratch);
                    cpu->SetRegister<Registers::PC>(*newPC);
                    delete newPC;
                });
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    auto bus = cpu->bus_;
                    auto msb = cpu->GetRegister<Registers::PC>() >> 8;
                    bus->Write<std::uint8_t>(cpu->GetRegister<Registers::SP>(), msb);
                    cpu->SubRegister<Registers::SP>(1);
                });
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    cpu->SubRegister<Registers::SP>(1);
                });
            } else {
                delete s;
            }
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto s = static_cast<std::uint16_t*>(scratch);
            *s = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());

            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    // PUSH DE
    case 0xD5:
        // 16 Cycles
        PushMicrocode(CycleNoOp);
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto val = static_cast<std::uint8_t>(cpu->GetRegister<Registers::E>());
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::SP>(), val);
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto val = static_cast<std::uint8_t>(cpu->GetRegister<Registers::D>());
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::SP>(), val);
            cpu->SubRegister<Registers::SP>(1);
        });
        PushMicrocode([scratch](CPU* cpu) {
            cpu->SubRegister<Registers::SP>(1);
        });
        break;

    // RET C
    case 0xD8:
        scratch = new std::uint16_t;

        // Always need a re-fetch cycle
        PushMicrocode(CycleNoOp);

        PushMicrocode([scratch](CPU* cpu) {
            if (cpu->GetFlag<Flags::C>()) {
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    // Set register
                    auto s = static_cast<std::uint16_t*>(scratch);
                    cpu->SetRegister<Registers::PC>(*s);
                    delete s;
                });
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    // Pop 1-byte for upper
                    auto bus = cpu->bus_;
                    auto upper = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::SP>());

                    auto s = static_cast<std::uint16_t*>(scratch);
                    *s = (std::uint16_t(upper) << 8) | std::uint16_t(*s & 0xFF);

                    cpu->AddRegister<Registers::SP>(1);
                });
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    // Pop 1-byte for lower
                    auto bus = cpu->bus_;

                    auto s = static_cast<std::uint16_t*>(scratch);
                    *s = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::SP>());

                    cpu->AddRegister<Registers::SP>(1);
                });
            }
        });
        break;

    // JP C, a16
    case 0xDA:
        scratch = new std::uint16_t;

        // 12/16 Cycles
        PushMicrocode(CycleNoOp); // Pipeline refresh
        PushMicrocode([scratch](CPU* cpu) {
            // Pop 1-byte for upper
            auto bus = cpu->bus_;
            auto s = static_cast<std::uint16_t*>(scratch);
            auto upper = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            *s = (std::uint16_t(upper) << 8) | std::uint16_t(*s & 0xFF);

            cpu->AddRegister<Registers::PC>(1);

            // Do actual JUMP is needed
            if (cpu->GetFlag<Flags::C>()) {
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    auto newPC = static_cast<std::uint16_t*>(scratch);
                    cpu->SetRegister<Registers::PC>(*newPC);
                    delete newPC;
                });
            } else {
                delete s;
            }
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto s = static_cast<std::uint16_t*>(scratch);
            *s = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());

            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    // CALL C, u16
    case 0xDC:
        scratch = new std::uint16_t;

        // 12/24 Cycles
        PushMicrocode(CycleNoOp); // Pipeline refresh
        PushMicrocode([scratch](CPU* cpu) {
            // Pop 1-byte for upper
            auto bus = cpu->bus_;
            auto s = static_cast<std::uint16_t*>(scratch);
            auto upper = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            *s = (std::uint16_t(upper) << 8) | std::uint16_t(*s & 0xFF);

            cpu->AddRegister<Registers::PC>(1);

            // Do actual JUMP is needed
            if (cpu->GetFlag<Flags::C>()) {
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    auto bus = cpu->bus_;
                    auto lsb = cpu->GetRegister<Registers::PC>() & 0xFF;
                    bus->Write<std::uint8_t>(cpu->GetRegister<Registers::SP>(), lsb);

                    auto newPC = static_cast<std::uint16_t*>(scratch);
                    cpu->SetRegister<Registers::PC>(*newPC);
                    delete newPC;
                });
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    auto bus = cpu->bus_;
                    auto msb = cpu->GetRegister<Registers::PC>() >> 8;
                    bus->Write<std::uint8_t>(cpu->GetRegister<Registers::SP>(), msb);
                    cpu->SubRegister<Registers::SP>(1);
                });
                cpu->PushMicrocode([scratch](CPU* cpu) {
                    cpu->SubRegister<Registers::SP>(1);
                });
            } else {
                delete s;
            }
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto s = static_cast<std::uint16_t*>(scratch);
            *s = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());

            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    // LD (0xFF00 + n), A
    case 0xE0:
        scratch = new std::uint8_t;

        // 12 Cycles
        PushMicrocode(CycleNoOp);
        PushMicrocode([scratch](CPU* cpu) {
            auto n = static_cast<std::uint8_t*>(scratch);
            std::uint16_t addr = std::uint16_t(0xFF00) | *n;
            delete n;

            auto lsb = std::uint8_t(cpu->GetRegister<Registers::A>());

            auto bus = cpu->bus_;
            bus->Write<std::uint8_t>(addr, lsb);
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint8_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            cpu->AddRegister<Registers::PC>(1);
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

    // LD (0xFF00 + C), A
    case 0xE2:
        // 8 Cycles
        PushMicrocode(CycleNoOp);
        PushMicrocode([](CPU* cpu) {
            std::uint16_t addr = std::uint16_t(0xFF00) | cpu->GetRegister<Registers::C>();
            auto lsb = std::uint8_t(cpu->GetRegister<Registers::A>());

            auto bus = cpu->bus_;
            bus->Write<std::uint8_t>(addr, lsb);
        });
        break;

    // PUSH HL
    case 0xE5:
        // 16 Cycles
        PushMicrocode(CycleNoOp);
        PushMicrocode([](CPU* cpu) {
            auto bus = cpu->bus_;
            auto val = static_cast<std::uint8_t>(cpu->GetRegister<Registers::L>());
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::SP>(), val);
        });
        PushMicrocode([](CPU* cpu) {
            auto bus = cpu->bus_;
            auto val = static_cast<std::uint8_t>(cpu->GetRegister<Registers::H>());
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::SP>(), val);
            cpu->SubRegister<Registers::SP>(1);
        });
        PushMicrocode([](CPU* cpu) {
            cpu->SubRegister<Registers::SP>(1);
        });
        break;

    // JP HL
    case 0xE9:
        PushMicrocode([](CPU* cpu) {
            auto newPC = cpu->GetRegister<Registers::HL>();
            cpu->SetRegister<Registers::PC>(newPC);
        });
        break;

    // LD (u16), A
    case 0xEA:
        scratch = new std::uint16_t;

        // 16 Cycles
        PushMicrocode(CycleNoOp);
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;

            auto tmp = static_cast<std::uint16_t*>(scratch);
            bus->Write<std::uint8_t>(*tmp, static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>()));
            delete tmp;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto val = bus->Read<std::uint16_t>(cpu->GetRegister<Registers::PC>());
            *static_cast<std::uint16_t*>(scratch) |= val << 8;
            cpu->AddRegister<Registers::PC>(1);
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto val = bus->Read<std::uint16_t>(cpu->GetRegister<Registers::PC>());
            *static_cast<std::uint16_t*>(scratch) = val;
            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    // XOR d8
    case 0xEE:
        scratch = new std::uint8_t;

        // 8 Cycles
        PushMicrocode([scratch](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());

            auto tmp = static_cast<std::uint8_t*>(scratch);
            std::uint8_t value = a ^ *tmp;
            delete tmp;

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(false);
            cpu->SetFlag<Flags::H>(false);
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint8_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    // LD A, (0xFF00 + n)
    case 0xF0:
        scratch = new std::uint8_t;

        // 12 Cycles
        PushMicrocode([scratch](CPU* cpu) {
            auto n = static_cast<std::uint8_t*>(scratch);
            cpu->SetRegister<Registers::A>(*n);
            delete n;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto n = static_cast<std::uint8_t*>(scratch);
            std::uint16_t addr = std::uint16_t(0xFF00) | *n;

            auto bus = cpu->bus_;
            *n = bus->Read<std::uint8_t>(addr);
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint8_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            cpu->AddRegister<Registers::PC>(1);
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
            auto val = static_cast<std::uint8_t>(cpu->GetRegister<Registers::F>());
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::SP>(), val);
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto val = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            bus->Write<std::uint8_t>(cpu->GetRegister<Registers::SP>(), val);
            cpu->SubRegister<Registers::SP>(1);
        });
        PushMicrocode([scratch](CPU* cpu) {
            cpu->SubRegister<Registers::SP>(1);
        });
        break;

    // OR A, u8
    case 0xF6:
        scratch = new std::uint8_t;

        // 8 Cycles
        PushMicrocode([scratch](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());

            auto tmp = static_cast<std::uint8_t*>(scratch);
            std::uint8_t value = a | *tmp;
            delete tmp;

            cpu->SetRegister<Registers::A>(value);
            cpu->SetFlag<Flags::Z>(value == 0);
            cpu->SetFlag<Flags::N>(false);
            cpu->SetFlag<Flags::C>(false);
            cpu->SetFlag<Flags::H>(false);
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint8_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    // LD A, (u16)
    case 0xFA:
        scratch = new std::uint16_t;

        // 16 Cycles
        PushMicrocode([scratch](CPU* cpu) {
            auto tmp = static_cast<std::uint16_t*>(scratch);
            cpu->SetRegister<Registers::A>(*tmp);
            delete tmp;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto val = bus->Read<std::uint8_t>(*static_cast<std::uint16_t*>(scratch));

            *static_cast<std::uint16_t*>(scratch) = val;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto val = bus->Read<std::uint16_t>(cpu->GetRegister<Registers::PC>());
            *static_cast<std::uint16_t*>(scratch) |= val << 8;
            cpu->AddRegister<Registers::PC>(1);
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            auto val = bus->Read<std::uint16_t>(cpu->GetRegister<Registers::PC>());
            *static_cast<std::uint16_t*>(scratch) = val;
            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    // CP A, u8
    case 0xFE:
        scratch = new std::uint8_t;

        // 4 Cycles
        PushMicrocode([scratch](CPU* cpu) {
            auto a = static_cast<std::uint8_t>(cpu->GetRegister<Registers::A>());
            auto other = static_cast<std::uint8_t*>(scratch);
            cpu->SetFlag<Flags::Z>(a == *other);
            cpu->SetFlag<Flags::N>(true);
            cpu->SetFlag<Flags::H>(IsHCSub(a, *other));
            cpu->SetFlag<Flags::C>(a < *other);
            delete other;
        });
        PushMicrocode([scratch](CPU* cpu) {
            auto bus = cpu->bus_;
            *static_cast<std::uint8_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::PC>());
            cpu->AddRegister<Registers::PC>(1);
        });
        break;

    default:
        spdlog::critical("Unknown Opcode 0x{:02X} @ 0x{:04X}", opcode, GetRegister<Registers::PC>() - 1);
        throw std::runtime_error("Unknown opcode");
    }
}

void CPU::DecodeCBOpcode()
{
    void* scratch = nullptr;
    int bit = 0;

    auto pc = GetRegister<Registers::PC>();
    auto opcode = bus_->Read<std::uint8_t>(pc);

    switch (opcode & 0xF0) {
    case 0x00:
        // RLC and RRC
        switch ((opcode - 0x10) & 0x7) {
        case 0x0:
            PushMicrocode(GenerateRLC_RRC<Registers::B>((opcode & 0xF) < 0x8));
            break;
        case 0x1:
            PushMicrocode(GenerateRLC_RRC<Registers::C>((opcode & 0xF) < 0x8));
            break;
        case 0x2:
            PushMicrocode(GenerateRLC_RRC<Registers::D>((opcode & 0xF) < 0x8));
            break;
        case 0x3:
            PushMicrocode(GenerateRLC_RRC<Registers::E>((opcode & 0xF) < 0x8));
            break;
        case 0x4:
            PushMicrocode(GenerateRLC_RRC<Registers::H>((opcode & 0xF) < 0x8));
            break;
        case 0x5:
            PushMicrocode(GenerateRLC_RRC<Registers::L>((opcode & 0xF) < 0x8));
            break;
        case 0x6:
            scratch = new std::uint8_t;
            PushMicrocode([scratch](CPU* cpu) {
                // Write Back
                auto bus = cpu->bus_;
                auto val = static_cast<std::uint8_t*>(scratch);
                bus->Write<std::uint8_t>(cpu->GetRegister<Registers::HL>(), *val);
                delete val;
            });
            PushMicrocode([opcode, scratch](CPU* cpu) {
                int carry;
                auto val = static_cast<std::uint8_t*>(scratch);
                if ((opcode & 0xF) < 0x8) {
                    carry = (*val >> 7) & 0x1;
                    *val = (*val << 1) | carry;
                } else {
                    carry = *val & 0x1;
                    *val = (*val >> 1) | (carry << 7);
                }
                cpu->SetFlag<Flags::Z>(*val == 0);
                cpu->SetFlag<Flags::H>(false);
                cpu->SetFlag<Flags::N>(false);
                cpu->SetFlag<Flags::C>(carry);
            });
            PushMicrocode([scratch](CPU* cpu) {
                // Write Back
                auto bus = cpu->bus_;
                auto val = static_cast<std::uint8_t*>(scratch);
                *val = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::HL>());
            });
            break;
        case 0x7:
            PushMicrocode(GenerateRLC_RRC<Registers::A>((opcode & 0xF) < 0x8));
            break;
        }
        break;

    case 0x10:
        // RL and RR
        switch ((opcode - 0x10) & 0x7) {
        case 0x0:
            PushMicrocode(GenerateRL_RR<Registers::B>((opcode & 0xF) < 0x8));
            break;
        case 0x1:
            PushMicrocode(GenerateRL_RR<Registers::C>((opcode & 0xF) < 0x8));
            break;
        case 0x2:
            PushMicrocode(GenerateRL_RR<Registers::D>((opcode & 0xF) < 0x8));
            break;
        case 0x3:
            PushMicrocode(GenerateRL_RR<Registers::E>((opcode & 0xF) < 0x8));
            break;
        case 0x4:
            PushMicrocode(GenerateRL_RR<Registers::H>((opcode & 0xF) < 0x8));
            break;
        case 0x5:
            PushMicrocode(GenerateRL_RR<Registers::L>((opcode & 0xF) < 0x8));
            break;
        case 0x6:
            scratch = new std::uint8_t;
            PushMicrocode([scratch](CPU* cpu) {
                // Write Back
                auto bus = cpu->bus_;
                auto val = static_cast<std::uint8_t*>(scratch);
                bus->Write<std::uint8_t>(cpu->GetRegister<Registers::HL>(), *val);
                delete val;
            });
            PushMicrocode([opcode, scratch](CPU* cpu) {
                int carry = cpu->GetFlag<Flags::C>();
                auto val = static_cast<std::uint8_t*>(scratch);

                if ((opcode & 0xF) < 0x8) {
                    cpu->SetFlag<Flags::C>(*val >> 7);
                    *val <<= 1;
                    *val |= carry;
                } else {
                    cpu->SetFlag<Flags::C>(*val & 0x1);
                    *val >>= 1;
                    *val |= carry << 7;
                }
                cpu->SetFlag<Flags::Z>(*val == 0);
                cpu->SetFlag<Flags::H>(false);
                cpu->SetFlag<Flags::N>(false);
            });
            PushMicrocode([scratch](CPU* cpu) {
                // Write Back
                auto bus = cpu->bus_;
                auto val = static_cast<std::uint8_t*>(scratch);
                *val = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::HL>());
            });
            break;
        case 0x7:
            PushMicrocode(GenerateRL_RR<Registers::A>((opcode & 0xF) < 0x8));
            break;
        }
        break;
    case 0x40:
    case 0x50:
    case 0x60:
    case 0x70:
        bit = (opcode - 0x40) / 0x8;
        switch ((opcode - 0x40) & 0x7) {
        case 0x0:
            // B
            PushMicrocode(GenerateBit8<Registers::B>(bit));
            break;
        case 0x1:
            // C
            PushMicrocode(GenerateBit8<Registers::C>(bit));
            break;
        case 0x2:
            // D
            PushMicrocode(GenerateBit8<Registers::D>(bit));
            break;
        case 0x3:
            // E
            PushMicrocode(GenerateBit8<Registers::E>(bit));
            break;
        case 0x4:
            // H
            PushMicrocode(GenerateBit8<Registers::H>(bit));
            break;
        case 0x5:
            // L
            PushMicrocode(GenerateBit8<Registers::L>(bit));
            break;
        case 0x6:
            scratch = new std::uint8_t;

            // (HL)
            PushMicrocode([scratch, bit](CPU* cpu) {
                auto tmp = static_cast<std::uint8_t*>(scratch);
                auto z = (*tmp >> bit) & 0x1;
                delete tmp;

                cpu->SetFlag<Flags::Z>(z);
                cpu->SetFlag<Flags::N>(0);
                cpu->SetFlag<Flags::H>(1);
            });
            PushMicrocode([scratch](CPU* cpu) {
                auto bus = cpu->bus_;
                *static_cast<std::uint8_t*>(scratch) = bus->Read<std::uint8_t>(cpu->GetRegister<Registers::HL>());
            });
            break;
        case 0x7:
            // A
            PushMicrocode(GenerateBit8<Registers::A>(bit));
            break;
        }
        break;
    default:
        spdlog::critical("Unknown CB Opcode 0x{:02X} @ 0x{:04X}", opcode, GetRegister<Registers::PC>() - 1);
        throw std::runtime_error("Unknown opcode");
    }
}

}; // namespace emulator::gameboy
