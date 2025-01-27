#pragma once

#include <array>
#include <functional>
#include <type_traits>
#include <utility>

#include <components/bus.h>
#include <components/cpu.h>

namespace emulator::chip8
{

class CPU : public emulator::component::CPU
{
private:
    std::array<std::uint8_t, 16> registers_;
    std::uint16_t pc_;
    std::uint8_t sp_;
    std::array<std::uint16_t, 16> stack_;

    bool enableSysAddrOpcode_{false}; // 0x0NNN

    void Decode8Opcodes(std::uint16_t opcode);
    void DecodeFOpcodes(std::uint16_t opcode);

public:
    CPU();
    CPU(const CPU& other);

    ~CPU();

    void ReceiveTick() override;

    void PowerOn() noexcept override {};
    void PowerOff() noexcept override {};

    void LogStacktrace() noexcept override;
};

}; // namespace emulator::chip8
