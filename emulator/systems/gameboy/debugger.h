#pragma once

#include "cpu.h"

#include <debugger/sysdebugger.h>
#include <components/system.h>

#include <array>

namespace emulator::gameboy {

class Debugger : public emulator::debugger::ISystemDebugger {
private:
    std::array<emulator::debugger::RegisterInfo, 1> kDebugRegisters;

    emulator::component::System* system_;
    emulator::gameboy::CPU* cpu_;

public:
    Debugger(emulator::gameboy::CPU* cpu) : cpu_(cpu), system_(nullptr), emulator::debugger::ISystemDebugger("gameboy")
    {
        // TODO: Complete all registers
        kDebugRegisters[0].name = "AF";
        kDebugRegisters[0].altName = "af";
        kDebugRegisters[0].bitSize = 16;
        kDebugRegisters[0].offset = 0;
        kDebugRegisters[0].group = "General Purpose Registers";
    }

    void SetSystem(emulator::component::System* system) noexcept
    {
        system_ = system;
    }

    constexpr std::uint32_t GetPtrSize() const noexcept
    {
        return sizeof(std::uint32_t);
    }

    const emulator::debugger::RegisterInfo* GetRegisterInfo(std::size_t regNum) const noexcept
    {
        if (regNum >= kDebugRegisters.size()) {
            return nullptr;
        }
        return &kDebugRegisters[regNum];
    }

    void HandleSignal(std::uint8_t signal) const noexcept
    {
        stopped_ = true;
    }

    bool GetRegister(std::string name, std::uint64_t& value) const noexcept
    {
        if (name == "AF" || name == "af") {
            value = cpu_->GetRegister<emulator::gameboy::CPU::Registers::AF>();
        } else if (name == "A" || name == "a") {
            value = cpu_->GetRegister<emulator::gameboy::CPU::Registers::A>();
        } else if (name == "F" || name == "f") {
            value = cpu_->GetRegister<emulator::gameboy::CPU::Registers::F>();
        } else if (name == "BC" || name == "bc") {
            value = cpu_->GetRegister<emulator::gameboy::CPU::Registers::BC>();
        } else if (name == "B" || name == "b") {
            value = cpu_->GetRegister<emulator::gameboy::CPU::Registers::B>();
        } else if (name == "C" || name == "c") {
            value = cpu_->GetRegister<emulator::gameboy::CPU::Registers::C>();
        } else if (name == "DE" || name == "de") {
            value = cpu_->GetRegister<emulator::gameboy::CPU::Registers::DE>();
        } else if (name == "D" || name == "d") {
            value = cpu_->GetRegister<emulator::gameboy::CPU::Registers::D>();
        } else if (name == "E" || name == "e") {
            value = cpu_->GetRegister<emulator::gameboy::CPU::Registers::E>();
        } else if (name == "HL" || name == "hl") {
            value = cpu_->GetRegister<emulator::gameboy::CPU::Registers::HL>();
        } else if (name == "H" || name == "h") {
            value = cpu_->GetRegister<emulator::gameboy::CPU::Registers::H>();
        } else if (name == "L" || name == "l") {
            value = cpu_->GetRegister<emulator::gameboy::CPU::Registers::L>();
        } else if (name == "PC" || name == "pc") {
            value = cpu_->GetRegister<emulator::gameboy::CPU::Registers::PC>();
        } else if (name == "SP" || name == "sp") {
            value = cpu_->GetRegister<emulator::gameboy::CPU::Registers::SP>();
        } else {
            return false;
        }
        return true;
    }

    bool SetRegister(std::string name, std::uint64_t _value) noexcept
    {
        auto value = static_cast<std::uint16_t>(_value & 0xFFFF);
        if (name == "AF" || name == "af") {
            cpu_->SetRegister<emulator::gameboy::CPU::Registers::AF>(value);
        } else if (name == "A" || name == "a") {
            cpu_->SetRegister<emulator::gameboy::CPU::Registers::A>(value);
        } else if (name == "F" || name == "f") {
            cpu_->SetRegister<emulator::gameboy::CPU::Registers::F>(value);
        } else if (name == "BC" || name == "bc") {
            cpu_->SetRegister<emulator::gameboy::CPU::Registers::BC>(value);
        } else if (name == "B" || name == "b") {
            cpu_->SetRegister<emulator::gameboy::CPU::Registers::B>(value);
        } else if (name == "C" || name == "c") {
            cpu_->SetRegister<emulator::gameboy::CPU::Registers::C>(value);
        } else if (name == "DE" || name == "de") {
            cpu_->SetRegister<emulator::gameboy::CPU::Registers::DE>(value);
        } else if (name == "D" || name == "d") {
            cpu_->SetRegister<emulator::gameboy::CPU::Registers::D>(value);
        } else if (name == "E" || name == "e") {
            cpu_->SetRegister<emulator::gameboy::CPU::Registers::E>(value);
        } else if (name == "HL" || name == "hl") {
            cpu_->SetRegister<emulator::gameboy::CPU::Registers::HL>(value);
        } else if (name == "H" || name == "h") {
            cpu_->SetRegister<emulator::gameboy::CPU::Registers::H>(value);
        } else if (name == "L" || name == "l") {
            cpu_->SetRegister<emulator::gameboy::CPU::Registers::L>(value);
        } else if (name == "PC" || name == "pc") {
            cpu_->SetRegister<emulator::gameboy::CPU::Registers::PC>(value);
        } else if (name == "SP" || name == "sp") {
            cpu_->SetRegister<emulator::gameboy::CPU::Registers::SP>(value);
        } else {
            return false;
        }
        return true;

        // TODO: Repeat for all registers
    }

    std::uint8_t* ReadMemory(emulator::debugger::Address addr, std::size_t& bytes) const noexcept
    {
        auto bus = system_->GetBus();

        auto buf = new std::uint8_t[bytes];
        if (buf == nullptr) {
            bytes = 0;
            return nullptr;
        }

        for (auto start = addr, end = addr + bytes; start < end; start++) {
            try {
                buf[start - addr] = bus.Read<std::uint8_t>(start);
            } catch (emulator::component::InvalidAddress) {
                bytes = start - addr;
                return buf;
            }
        }
        return buf;
    }

    bool WriteMemory(emulator::debugger::Address, void*, std::size_t) noexcept
    {
        return false;
    }
};

};
