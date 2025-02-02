#pragma once

#include <array>
#include <functional>
#include <type_traits>
#include <utility>

#include <components/bus.h>
#include <components/cpu.h>
#include <components/display.h>

namespace emulator::chip8
{

class CPU : public emulator::component::CPU
{
public:
    constexpr static std::size_t kFontSetBaseAddress = 0x50;
    constexpr static std::size_t kSpriteLength = 5;

    emulator::component::Display::Pixel pixelOn_;
    emulator::component::Display::Pixel pixelOff_;

private:
    std::unordered_map<std::uint8_t, std::uint8_t> keymap_;

    std::array<std::uint8_t, 16> registers_;
    std::uint16_t I_;
    std::uint16_t pc_;
    std::uint8_t sp_;
    std::array<std::uint16_t, 16> stack_;

    bool waitingForKeyChange_{false};

    bool enableSysAddrOpcode_{false}; // 0x0NNN

    void Decode8Opcodes(std::uint16_t opcode);
    void DecodeFOpcodes(std::uint16_t opcode);
    void DisplaySprite(std::uint16_t opcode);

public:
    CPU();
    CPU(const CPU& other);

    ~CPU();

    void ReceiveTick() override;

    void PowerOn() noexcept override {};
    void PowerOff() noexcept override {};

    void LogStacktrace() noexcept override;

    // Map Chip8 keycodes to the real keyoard keycodes
    void LoadKeymap(const std::uint8_t keycodes[])
    {
        keymap_[0x1] = keycodes[0];
        keymap_[0x2] = keycodes[1];
        keymap_[0x3] = keycodes[2];
        keymap_[0xC] = keycodes[3];
        keymap_[0x4] = keycodes[4];
        keymap_[0x5] = keycodes[5];
        keymap_[0x6] = keycodes[6];
        keymap_[0xD] = keycodes[7];
        keymap_[0x7] = keycodes[8];
        keymap_[0x8] = keycodes[9];
        keymap_[0x9] = keycodes[10];
        keymap_[0xE] = keycodes[11];
        keymap_[0xA] = keycodes[12];
        keymap_[0x0] = keycodes[13];
        keymap_[0xB] = keycodes[14];
        keymap_[0xF] = keycodes[15];
    }
};

}; // namespace emulator::chip8
