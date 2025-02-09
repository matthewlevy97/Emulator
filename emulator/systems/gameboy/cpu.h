#pragma once

#include <array>
#include <functional>
#include <type_traits>
#include <utility>

#include <components/bus.h>
#include <components/cpu.h>
#include <components/system.h>

#include "names.h"

namespace emulator::gameboy {

class CPU : public emulator::component::CPU {
public:
    static constexpr std::size_t TCycleToMCycle = 4;

    /*
    Registers:
        Name	Hi	Lo	Full Name
        AF	    A	-	Accumulator & Flags
        BC	    B	C	BC
        DE	    D	E	DE
        HL	    H	L	HL
        SP	    -	-	Stack Pointer
        PC	    -	-	Program Counter/Pointer
    */
    enum class Registers {
        AF,
        A,
        F,
        BC,
        B,
        C,
        DE,
        D,
        E,
        HL,
        H,
        L,
        SP,
        PC
    };

    /*
    Flags (Lower 8 bits of AF register):
        Bit	Name	Explanation
        7	z	Zero flag
        6	n	Subtraction flag (BCD)
        5	h	Half Carry flag (BCD)
        4	c	Carry flag
    */
    enum class Flags {
        Z,
        N,
        H,
        C
    };

    template<Registers Reg>
    inline std::uint16_t GetRegister()
    {
        if constexpr (Reg == Registers::AF) {
            return registers_[0];
        } else if constexpr (Reg == Registers::A) {
            return registers_[0] >> 8;
        } else if constexpr (Reg == Registers::F) {
            return registers_[0] & 0xFF;
        } else if constexpr (Reg == Registers::BC) {
            return registers_[1];
        } else if constexpr (Reg == Registers::B) {
            return registers_[1] >> 8;
        } else if constexpr (Reg == Registers::C) {
            return registers_[1] & 0xFF;
        } else if constexpr (Reg == Registers::DE) {
            return registers_[2];
        } else if constexpr (Reg == Registers::D) {
            return registers_[2] >> 8;
        } else if constexpr (Reg == Registers::E) {
            return registers_[2] & 0xFF;
        } else if constexpr (Reg == Registers::HL) {
            return registers_[3];
        } else if constexpr (Reg == Registers::H) {
            return registers_[3] >> 8;
        } else if constexpr (Reg == Registers::L) {
            return registers_[3] & 0xFF;
        } else if constexpr (Reg == Registers::SP) {
            return registers_[4];
        } else if constexpr (Reg == Registers::PC) {
            return registers_[5];
        } else {
            std::unreachable();
        }
    }

    template<Registers Reg>
    inline void SetRegister(std::uint16_t value)
    {
        if constexpr (Reg == Registers::AF) {
            registers_[0] = value;
        } else if constexpr (Reg == Registers::A) {
            registers_[0] = (registers_[0] & 0x00FF) | ((value & 0xFF) << 8);
        } else if constexpr (Reg == Registers::A) {
            registers_[0] = (registers_[0] & 0xFF00) | (value & 0xFF);
        } else if constexpr (Reg == Registers::BC) {
            registers_[1] = value;
        } else if constexpr (Reg == Registers::B) {
            registers_[1] = (registers_[1] & 0x00FF) | ((value & 0xFF) << 8);
        } else if constexpr (Reg == Registers::C) {
            registers_[1] = (registers_[1] & 0xFF00) | (value & 0xFF);
        } else if constexpr (Reg == Registers::DE) {
            registers_[2] = value;
        } else if constexpr (Reg == Registers::D) {
            registers_[2] = (registers_[2] & 0x00FF) | ((value & 0xFF) << 8);
        } else if constexpr (Reg == Registers::E) {
            registers_[2] = (registers_[2] & 0xFF00) | (value & 0xFF);
        } else if constexpr (Reg == Registers::HL) {
            registers_[3] = value;
        } else if constexpr (Reg == Registers::H) {
            registers_[3] = (registers_[3] & 0x00FF) | ((value & 0xFF) << 8);
        } else if constexpr (Reg == Registers::L) {
            registers_[3] = (registers_[3] & 0xFF00) | (value & 0xFF);
        } else if constexpr (Reg == Registers::SP) {
            registers_[4] = value;
        } else if constexpr (Reg == Registers::PC) {
            registers_[5] = value;
        } else {
            std::unreachable();
        }
    }

    template<Registers Reg>
    inline void AddRegister(std::uint16_t value)
    {
        if constexpr (Reg == Registers::AF) {
            registers_[0] += value;
        } else if constexpr (Reg == Registers::A) {
            registers_[0] += (registers_[0] & 0x00FF) | ((value & 0xFF) << 8);
        } else if constexpr (Reg == Registers::BC) {
            registers_[1] += value;
        } else if constexpr (Reg == Registers::B) {
            registers_[1] += (registers_[1] & 0x00FF) | ((value & 0xFF) << 8);
        } else if constexpr (Reg == Registers::C) {
            registers_[1] += (registers_[1] & 0xFF00) | (value & 0xFF);
        } else if constexpr (Reg == Registers::DE) {
            registers_[2] += value;
        } else if constexpr (Reg == Registers::D) {
            registers_[2] += (registers_[2] & 0x00FF) | ((value & 0xFF) << 8);
        } else if constexpr (Reg == Registers::E) {
            registers_[2] += (registers_[2] & 0xFF00) | (value & 0xFF);
        } else if constexpr (Reg == Registers::HL) {
            registers_[3] += value;
        } else if constexpr (Reg == Registers::H) {
            registers_[3] += (registers_[3] & 0x00FF) | ((value & 0xFF) << 8);
        } else if constexpr (Reg == Registers::L) {
            registers_[3] += (registers_[3] & 0xFF00) | (value & 0xFF);
        } else if constexpr (Reg == Registers::SP) {
            registers_[4] += value;
        } else if constexpr (Reg == Registers::PC) {
            registers_[5] += value;
        } else {
            std::unreachable();
        }
    }

    template<Registers Reg>
    inline void SubRegister(std::uint16_t value)
    {
        if constexpr (Reg == Registers::AF) {
            registers_[0] -= value;
        } else if constexpr (Reg == Registers::A) {
            registers_[0] -= (registers_[0] & 0x00FF) | ((value & 0xFF) << 8);
        } else if constexpr (Reg == Registers::BC) {
            registers_[1] -= value;
        } else if constexpr (Reg == Registers::B) {
            registers_[1] -= (registers_[1] & 0x00FF) | ((value & 0xFF) << 8);
        } else if constexpr (Reg == Registers::C) {
            registers_[1] -= (registers_[1] & 0xFF00) | (value & 0xFF);
        } else if constexpr (Reg == Registers::DE) {
            registers_[2] -= value;
        } else if constexpr (Reg == Registers::D) {
            registers_[2] -= (registers_[2] & 0x00FF) | ((value & 0xFF) << 8);
        } else if constexpr (Reg == Registers::E) {
            registers_[2] -= (registers_[2] & 0xFF00) | (value & 0xFF);
        } else if constexpr (Reg == Registers::HL) {
            registers_[3] -= value;
        } else if constexpr (Reg == Registers::H) {
            registers_[3] -= (registers_[3] & 0x00FF) | ((value & 0xFF) << 8);
        } else if constexpr (Reg == Registers::L) {
            registers_[3] -= (registers_[3] & 0xFF00) | (value & 0xFF);
        } else if constexpr (Reg == Registers::SP) {
            registers_[4] -= value;
        } else if constexpr (Reg == Registers::PC) {
            registers_[5] -= value;
        } else {
            std::unreachable();
        }
    }

    template<Flags Flag>
    inline bool GetFlag()
    {
        if constexpr (Flag == Flags::Z) {
            return (registers_[0] >> 7) & 0x1;
        } else if constexpr (Flag == Flags::N) {
            return (registers_[0] >> 6) & 0x1;
        } else if constexpr (Flag == Flags::H) {
            return (registers_[0] >> 5) & 0x1;
        } else if constexpr (Flag == Flags::C) {
            return (registers_[0] >> 4) & 0x1;
        } else {
            std::unreachable();
        }
    }

    template<Flags Flag>
    inline void SetFlag(bool value)
    {
        std::uint16_t mask;
        if constexpr (Flag == Flags::Z) {
            mask = 1 << 7;
        } else if constexpr (Flag == Flags::N) {
            mask = 1 << 6;
        } else if constexpr (Flag == Flags::H) {
            mask = 1 << 5;
        } else if constexpr (Flag == Flags::C) {
            mask = 1 << 4;
        } else {
            std::unreachable();
        }

        if (value) {
            registers_[0] |= mask;
        } else {
            registers_[0] &= ~mask;
        }
    }

private:
    using MicroCode = std::function<void(CPU*)>;
    std::array<MicroCode, 32> microcode_;
    size_t microcodeStackLength_;

    std::array<std::uint16_t, 6> registers_;

    void PushMicrocode(MicroCode code);
    void DecodeOpcode(std::uint8_t opcode);

    template <Registers reg>
    MicroCode GenerateRLC_RRC(bool shiftLeft)
    {
        if (shiftLeft) {
            return [](CPU* cpu) {
                std::uint8_t val = static_cast<std::uint8_t>(cpu->GetRegister<reg>());
                int carry = (val >> 7) & 0x1;

                val = (val << 1) | carry;
                
                cpu->SetRegister<reg>(val);
                cpu->SetFlag<Flags::Z>(val == 0);
                cpu->SetFlag<Flags::H>(false);
                cpu->SetFlag<Flags::N>(false);
                cpu->SetFlag<Flags::C>(carry);
            };
        } else {
            return [](CPU* cpu) {
                std::uint8_t val = static_cast<std::uint8_t>(cpu->GetRegister<reg>());
                int carry = val & 0x1;

                val = (val >> 1) | (carry << 7);

                cpu->SetRegister<reg>(val);
                cpu->SetFlag<Flags::Z>(val == 0);
                cpu->SetFlag<Flags::H>(false);
                cpu->SetFlag<Flags::N>(false);
                cpu->SetFlag<Flags::C>(carry);
            };
        }
    }

    template <Registers reg>
    MicroCode GenerateRL_RR(bool shiftLeft)
    {
        if (shiftLeft) {
            return [](CPU* cpu) {
                int carry = cpu->GetFlag<Flags::C>();
                std::uint8_t val = static_cast<std::uint8_t>(cpu->GetRegister<reg>());

                cpu->SetFlag<Flags::C>(val >> 7);
                val <<= 1;
                val |= carry;

                cpu->SetRegister<reg>(val);
                cpu->SetFlag<Flags::Z>(val == 0);
                cpu->SetFlag<Flags::H>(false);
                cpu->SetFlag<Flags::N>(false);
            };
        } else {
            return [](CPU* cpu) {
                int carry = cpu->GetFlag<Flags::C>();
                std::uint8_t val = static_cast<std::uint8_t>(cpu->GetRegister<reg>());

                cpu->SetFlag<Flags::C>(val & 0x1);
                val >>= 1;
                val |= carry << 7;

                cpu->SetRegister<reg>(val);
                cpu->SetFlag<Flags::Z>(val == 0);
                cpu->SetFlag<Flags::H>(false);
                cpu->SetFlag<Flags::N>(false);
            };
        }
    }

    template <Registers reg>
    MicroCode GenerateBit8(int bit)
    {
        return [bit](CPU* cpu) {
            auto val = cpu->GetRegister<reg>();
            cpu->SetFlag<Flags::Z>(!((val >> bit) & 0x1));
            cpu->SetFlag<Flags::N>(0);
            cpu->SetFlag<Flags::H>(1);
        };
    }

    void DecodeCBOpcode();

public:
    CPU();
    CPU(const CPU& other);

    ~CPU();

    void ReceiveTick() override;

    void PowerOn() noexcept override;
    void PowerOff() noexcept override;

    void AttachToBus(component::Bus* bus) override;

    void LogStacktrace() noexcept override;

    // NoOp IO Instructions
    void WriteUInt8(std::size_t address, std::uint8_t value) override
    {
        if (address == 0xFF50) {
            // Disable boot ROM if value != 0
            auto system = bus_->GetBoundSystem();
            bus_->RemoveComponent(system->GetComponent(kBootROMName));
        }
    }

    void WriteInt8(std::size_t address, std::int8_t value) override
    {
        return;
    }

    void WriteUInt16(std::size_t address, std::uint16_t value) override
    {
        return;
    }

    void WriteInt16(std::size_t address, std::int16_t value) override
    {
        return;
    }

    void WriteUInt32(std::size_t address, std::uint32_t value) override
    {
        return;
    }

    void WriteInt32(std::size_t address, std::int32_t value) override
    {
        return;
    }

    std::uint8_t ReadUInt8(std::size_t address) override
    {
        return 0;
    }

    std::int8_t ReadInt8(std::size_t address) override
    {
        return 0;
    }

    std::uint16_t ReadUInt16(std::size_t address) override
    {
        return 0;
    }

    std::int16_t ReadInt16(std::size_t address) override
    {
        return 0;
    }

    std::uint32_t ReadUInt32(std::size_t address) override
    {
        return 0;
    }

    std::int32_t ReadInt32(std::size_t address) override
    {
        return 0;
    }
};

}; // namespace emulator::gameboy
