#include <spdlog/spdlog.h>

#include <components/input.h>
#include <components/timer.h>

#include "cpu.h"

#include <utils.h>

// TODO: Fix failing Not Released case

namespace emulator::chip8
{

CPU::CPU()
{
    registers_.fill(0);
    pc_ = 0x200;
    sp_ = 0;
    stack_.fill(0);

    pixelOn_ = emulator::component::Display::Pixel(128, 255, 128, 255);
}

CPU::CPU(const CPU& other)
{
    registers_ = other.registers_;
    pc_ = other.pc_;
    sp_ = other.sp_;
    stack_ = other.stack_;
}

CPU::~CPU()
{
}

void CPU::ReceiveTick()
{
    if (waitingForKeyChange_) {
        return;
    }

    // Read Big Endian not Little Endian
    std::uint16_t opcode = std::uint16_t(bus_->Read<std::uint8_t>(pc_) << 8) | bus_->Read<std::uint8_t>(pc_ + 1);
    pc_ += sizeof(opcode);

    std::uint8_t reg = 0;
    std::uint8_t reg2 = 0;

    // Decode and Execute Opcode
    switch (opcode >> 12) {
    case 0x0:
        switch (opcode & 0x00FF) {
        case 0xE0: {
            auto display = bus_->GetBoundSystem()->GetFirstComponentByType<emulator::component::Display>(emulator::component::IComponent::ComponentType::Display);
            if (!display) {
                spdlog::critical("Display component not found");
                throw std::runtime_error("Display component not found");
            }
            display->ClearScreen();
            break;
        }
        case 0xEE:
            // Return from subroutine
            if (sp_ >= stack_.size()) {
                spdlog::critical("Stack overflow");
                throw std::runtime_error("Stack overflow");
            }
            pc_ = stack_[sp_];
            sp_--;
            break;
        default:
            if (enableSysAddrOpcode_) {
                // Jump to machine code routine at nnn
                pc_ = opcode & 0x0FFF;
            } else {
                spdlog::critical("Unknown Opcode 0x{:04X} @ 0x{:04X}", opcode, pc_ - sizeof(pc_));
                throw std::runtime_error("Unknown opcode");
            }
        }
        break;
    case 0x1:
        // Jump to address nnn
        pc_ = opcode & 0x0FFF;
        break;
    case 0x2:
        // Call subroutine at nnn
        sp_++;

        if (sp_ >= stack_.size()) {
            spdlog::critical("Stack overflow");
            throw std::runtime_error("Stack overflow");
        }
        stack_[sp_] = pc_;
        pc_ = opcode & 0x0FFF;
        break;
    case 0x3:
        // Skip next instruction if Vx == kk
        reg = (opcode & 0x0F00) >> 8;
        if (reg >= registers_.size()) {
            spdlog::critical("Unknown register V{:X} @ 0x{:04X}", reg, pc_ - sizeof(pc_));
            throw std::runtime_error("Unknown register");
        }
        if (registers_[reg] == (opcode & 0x00FF)) {
            pc_ += 2;
        }
        break;
    case 0x4:
        // Skip next instruction if Vx != kk
        reg = (opcode & 0x0F00) >> 8;
        if (reg >= registers_.size()) {
            spdlog::critical("Unknown register V{:X} @ 0x{:04X}", reg, pc_ - sizeof(pc_));
            throw std::runtime_error("Unknown register");
        }
        if (registers_[reg] != (opcode & 0x00FF)) {
            pc_ += 2;
        }
        break;
    case 0x5:
        // Skip next instruction if Vx == Vy
        reg = (opcode & 0x0F00) >> 8;
        reg2 = (opcode & 0x00F0) >> 4;
        if (reg >= registers_.size() || reg2 >= registers_.size()) {
            spdlog::critical("Unknown register V{:X} or V{:X} @ 0x{:04X}", reg, reg2, pc_ - sizeof(pc_));
            throw std::runtime_error("Unknown register");
        }
        if (registers_[reg] == registers_[reg2]) {
            pc_ += 2;
        }
        break;
    case 0x6:
        // Set Vx = kk
        reg = (opcode & 0x0F00) >> 8;
        if (reg >= registers_.size()) {
            spdlog::critical("Unknown register V{:X} @ 0x{:04X}", reg, pc_ - sizeof(pc_));
            throw std::runtime_error("Unknown register");
        }
        registers_[reg] = opcode & 0x00FF;
        break;
    case 0x7:
        // Set Vx = Vx + kk
        reg = (opcode & 0x0F00) >> 8;
        if (reg >= registers_.size()) {
            spdlog::critical("Unknown register V{:X} @ 0x{:04X}", reg, pc_ - sizeof(pc_));
            throw std::runtime_error("Unknown register");
        }
        registers_[reg] += opcode & 0x00FF;
        break;
    case 0x8:
        Decode8Opcodes(opcode);
        break;
    case 0x9:
        // Skip next instruction if Vx != Vy
        reg = (opcode & 0x0F00) >> 8;
        reg2 = (opcode & 0x00F0) >> 4;
        if (reg >= registers_.size() || reg2 >= registers_.size()) {
            spdlog::critical("Unknown register V{:X} or V{:X} @ 0x{:04X}", reg, reg2, pc_ - sizeof(pc_));
            throw std::runtime_error("Unknown register");
        }
        if (registers_[reg] != registers_[reg2]) {
            pc_ += 2;
        }
        break;
    case 0xA:
        // Set I = nnn
        I_ = opcode & 0x0FFF;
        break;
    case 0xB:
        // Jump to location nnn + V0
        pc_ = (opcode & 0x0FFF) + registers_[0];
        break;
    case 0xC:
        // Set Vx = random byte AND kk
        reg = (opcode & 0x0F00) >> 8;
        if (reg >= registers_.size()) {
            spdlog::critical("Unknown register V{:X} @ 0x{:04X}", reg, pc_ - sizeof(pc_));
            throw std::runtime_error("Unknown register");
        }
        registers_[reg] = emulator::GenerateRandom<std::uint8_t>() & (opcode & 0x00FF);
        break;
    case 0xD:
        DisplaySprite(opcode);
        break;
    case 0xE:
        switch (opcode & 0x00FF) {
        case 0x9E: {
            // Skip next instruction if key with the value of Vx is pressed
            reg = (opcode & 0x0F00) >> 8;
            if (reg >= registers_.size()) {
                spdlog::critical("Unknown register V{:X} @ 0x{:04X}", reg, pc_ - sizeof(pc_));
                throw std::runtime_error("Unknown register");
            }
            auto key = bus_->GetBoundSystem()->GetFirstComponentByType<emulator::component::Input>(emulator::component::IComponent::ComponentType::Input);
            if (!key) {
                spdlog::critical("Keyboard component not found");
                throw std::runtime_error("Keyboard component not found");
            }

            if (key->IsPressed(keymap_[registers_[reg]])) {
                pc_ += 2;
            }
        } break;
        case 0xA1: {
            // Skip next instruction if key with the value of Vx is not pressed
            reg = (opcode & 0x0F00) >> 8;
            if (reg >= registers_.size()) {
                spdlog::critical("Unknown register V{:X} @ 0x{:04X}", reg, pc_ - sizeof(pc_));
                throw std::runtime_error("Unknown register");
            }
            auto key = bus_->GetBoundSystem()->GetFirstComponentByType<emulator::component::Input>(emulator::component::IComponent::ComponentType::Input);
            if (!key) {
                spdlog::critical("Keyboard component not found");
                throw std::runtime_error("Keyboard component not found");
            }

            if (!key->IsPressed(keymap_[registers_[reg]])) {
                pc_ += 2;
            }
        } break;
        default:
            spdlog::critical("Unknown Opcode 0x{:04X} @ 0x{:04X}", opcode, pc_ - sizeof(pc_));
            throw std::runtime_error("Unknown opcode");
        }
        break;
    case 0xF:
        DecodeFOpcodes(opcode);
        break;
    default:
        spdlog::critical("Unknown Opcode 0x{:04X} @ 0x{:04X}", opcode, pc_ - sizeof(pc_));
        throw std::runtime_error("Unknown opcode");
    }
}

void CPU::Decode8Opcodes(std::uint16_t opcode)
{
    std::uint8_t reg = 0;
    std::uint8_t reg2 = 0;

    int sum = 0;

    switch (opcode & 0x000F) {
    case 0x0:
        // Set Vx = Vy
        reg = (opcode & 0x0F00) >> 8;
        reg2 = (opcode & 0x00F0) >> 4;
        if (reg >= registers_.size() || reg2 >= registers_.size()) {
            spdlog::critical("Unknown register V{:X} or V{:X} @ 0x{:04X}", reg, reg2, pc_ - sizeof(pc_));
            throw std::runtime_error("Unknown register");
        }
        registers_[reg] = registers_[reg2];
        break;
    case 0x1:
        // Set Vx = Vx OR Vy
        reg = (opcode & 0x0F00) >> 8;
        reg2 = (opcode & 0x00F0) >> 4;
        if (reg >= registers_.size() || reg2 >= registers_.size()) {
            spdlog::critical("Unknown register V{:X} or V{:X} @ 0x{:04X}", reg, reg2, pc_ - sizeof(pc_));
            throw std::runtime_error("Unknown register");
        }
        registers_[reg] |= registers_[reg2];
        break;
    case 0x2:
        // Set Vx = Vx AND Vy
        reg = (opcode & 0x0F00) >> 8;
        reg2 = (opcode & 0x00F0) >> 4;
        if (reg >= registers_.size() || reg2 >= registers_.size()) {
            spdlog::critical("Unknown register V{:X} or V{:X} @ 0x{:04X}", reg, reg2, pc_ - sizeof(pc_));
            throw std::runtime_error("Unknown register");
        }
        registers_[reg] &= registers_[reg2];
        break;
    case 0x3:
        // Set Vx = Vx XOR Vy
        reg = (opcode & 0x0F00) >> 8;
        reg2 = (opcode & 0x00F0) >> 4;
        if (reg >= registers_.size() || reg2 >= registers_.size()) {
            spdlog::critical("Unknown register V{:X} or V{:X} @ 0x{:04X}", reg, reg2, pc_ - sizeof(pc_));
            throw std::runtime_error("Unknown register");
        }
        registers_[reg] ^= registers_[reg2];
        break;
    case 0x4:
        // Set Vx = Vx + Vy, set VF = carry
        reg = (opcode & 0x0F00) >> 8;
        reg2 = (opcode & 0x00F0) >> 4;
        if (reg >= registers_.size() || reg2 >= registers_.size()) {
            spdlog::critical("Unknown register V{:X} or V{:X} @ 0x{:04X}", reg, reg2, pc_ - sizeof(pc_));
            throw std::runtime_error("Unknown register");
        }
        sum = registers_[reg] + registers_[reg2];
        registers_[0xF] = sum > 0xFF;
        registers_[reg] = sum & 0xFF;
        break;
    case 0x5:
        // Set Vx = Vx - Vy, set VF = NOT borrow
        reg = (opcode & 0x0F00) >> 8;
        reg2 = (opcode & 0x00F0) >> 4;
        if (reg >= registers_.size() || reg2 >= registers_.size()) {
            spdlog::critical("Unknown register V{:X} or V{:X} @ 0x{:04X}", reg, reg2, pc_ - sizeof(pc_));
            throw std::runtime_error("Unknown register");
        }
        registers_[0xF] = registers_[reg] > registers_[reg2];
        registers_[reg] -= registers_[reg2];
        break;
    case 0x6:
        // Set Vx = Vx SHR 1
        reg = (opcode & 0x0F00) >> 8;
        if (reg >= registers_.size()) {
            spdlog::critical("Unknown register V{:X} @ 0x{:04X}", reg, pc_ - sizeof(pc_));
            throw std::runtime_error("Unknown register");
        }
        registers_[0xF] = registers_[reg] & 0x1;
        registers_[reg] >>= 1;
        break;
    case 0x7:
        // Set Vx = Vy - Vx, set VF = NOT borrow
        reg = (opcode & 0x0F00) >> 8;
        reg2 = (opcode & 0x00F0) >> 4;
        if (reg >= registers_.size() || reg2 >= registers_.size()) {
            spdlog::critical("Unknown register V{:X} or V{:X} @ 0x{:04X}", reg, reg2, pc_ - sizeof(pc_));
            throw std::runtime_error("Unknown register");
        }
        registers_[0xF] = registers_[reg2] > registers_[reg];
        registers_[reg] = registers_[reg2] - registers_[reg];
        break;
    case 0xE:
        // Set Vx = Vx SHL 1
        reg = (opcode & 0x0F00) >> 8;
        if (reg >= registers_.size()) {
            spdlog::critical("Unknown register V{:X} @ 0x{:04X}", reg, pc_ - sizeof(pc_));
            throw std::runtime_error("Unknown register");
        }
        registers_[0xF] = registers_[reg] & 0x80;
        registers_[reg] <<= 1;
        break;
    default:
        spdlog::critical("Unknown Opcode 0x{:04X} @ 0x{:04X}", opcode, pc_ - sizeof(pc_));
        throw std::runtime_error("Unknown opcode");
    }
}

void CPU::DecodeFOpcodes(std::uint16_t opcode)
{
    switch (opcode & 0x00FF) {
    case 0x07: {
        // Set Vx = delay timer value
        auto timer = bus_->GetBoundSystem()->GetComponentsByType<emulator::component::Timer>(emulator::component::IComponent::ComponentType::Timer);
        for (auto& t : timer) {
            if (t->GetName() == "Delay") {
                registers_[(opcode & 0x0F00) >> 8] = t->GetCounter();
                break;
            }
        }
    } break;
    case 0x0A: {
        // Wait for a key press, store the value of the key in Vx
        auto keyboard = bus_->GetBoundSystem()->GetFirstComponentByType<emulator::component::Input>(emulator::component::IComponent::ComponentType::Input);
        if (!keyboard) {
            spdlog::critical("Keyboard component not found");
            throw std::runtime_error("Keyboard component not found");
        }

        std::vector<std::uint8_t> currentlyPressedKeys;
        for (auto& [k, v] : keymap_) {
            if (keyboard->IsPressed(v)) {
                currentlyPressedKeys.push_back(k);
            }
        }

        keyboard->SetKeyPressHandlers(
            nullptr,
            [this, currentlyPressedKeys, opcode, keyboard](std::uint8_t key) {
                for (auto& [k, v] : keymap_) {
                    bool consider = true;
                    for (auto& c : currentlyPressedKeys) {
                        if (c == k) {
                            consider = false;
                            break;
                        }
                    }

                    if (consider && v == key) {
                        registers_[(opcode & 0x0F00) >> 8] = k;
                        waitingForKeyChange_ = false;
                        keyboard->SetKeyPressHandlers(nullptr, nullptr);
                        break;
                    }
                }
            });
        waitingForKeyChange_ = true;

    } break;
    case 0x15: {
        // Set delay timer = Vx
        auto timer = bus_->GetBoundSystem()->GetComponentsByType<emulator::component::Timer>(emulator::component::IComponent::ComponentType::Timer);
        for (auto& t : timer) {
            if (t->GetName() == "Delay") {
                t->SetCounter(registers_[(opcode & 0x0F00) >> 8]);
                break;
            }
        }
    } break;
    case 0x18: {
        // Set sound timer = Vx
        auto timer = bus_->GetBoundSystem()->GetComponentsByType<emulator::component::Timer>(emulator::component::IComponent::ComponentType::Timer);
        for (auto& t : timer) {
            if (t->GetName() == "Sound") {
                t->SetCounter(registers_[(opcode & 0x0F00) >> 8]);
                break;
            }
        }
    } break;
    case 0x1E:
        // Set I = I + Vx
        I_ += registers_[(opcode & 0x0F00) >> 8];
        break;
    case 0x29:
        // Set I = location of sprite for digit Vx
        I_ = registers_[(opcode & 0x0F00) >> 8] * kSpriteLength;
        I_ += kFontSetBaseAddress;
        break;
    case 0x33:
        // Store BCD representation of Vx in memory locations I, I+1, and I+2
        bus_->Write(I_, registers_[(opcode & 0x0F00) >> 8] / 100);
        bus_->Write(I_ + 1, (registers_[(opcode & 0x0F00) >> 8] / 10) % 10);
        bus_->Write(I_ + 2, registers_[(opcode & 0x0F00) >> 8] % 10);
        break;
    case 0x55:
        // Store registers V0 through Vx in memory starting at location I
        for (std::size_t i = 0; i <= ((opcode & 0x0F00) >> 8); i++) {
            bus_->Write(I_ + i, registers_[i]);
        }
        break;
    case 0x65:
        // Read registers V0 through Vx from memory starting at location I
        for (std::size_t i = 0; i <= ((opcode & 0x0F00) >> 8); i++) {
            registers_[i] = bus_->Read<std::uint8_t>(I_ + i);
        }
        break;
    default:
        spdlog::critical("Unknown Opcode 0x{:04X} @ 0x{:04X}", opcode, pc_ - sizeof(pc_));
        throw std::runtime_error("Unknown opcode");
    }
}

void CPU::LogStacktrace() noexcept
{
    spdlog::debug("[CPU] V0: {:02X}   V1: {:02X}   V2: {:02X}   V3: {:02X}",
                  registers_[0], registers_[1], registers_[2], registers_[3]);
    spdlog::debug("[CPU] V4: {:02X}   V5: {:02X}   V6: {:02X}   V7: {:02X}",
                  registers_[4], registers_[5], registers_[6], registers_[7]);
    spdlog::debug("[CPU] V8: {:02X}   V9: {:02X}   VA: {:02X}   VB: {:02X}",
                  registers_[8], registers_[9], registers_[10], registers_[11]);
    spdlog::debug("[CPU] VC: {:02X}   VD: {:02X}   VE: {:02X}   VF: {:02X}",
                  registers_[12], registers_[13], registers_[14], registers_[15]);
    spdlog::debug("[CPU] PC: {:04X}   SP: {:02X}", pc_, sp_);
}

void CPU::DisplaySprite(std::uint16_t opcode)
{
    // Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision
    std::uint8_t reg = (opcode & 0x0F00) >> 8;
    std::uint8_t reg2 = (opcode & 0x00F0) >> 4;
    if (reg >= registers_.size() || reg2 >= registers_.size()) {
        spdlog::critical("Unknown register V{:X} or V{:X} @ 0x{:04X}", reg, reg2, pc_ - sizeof(pc_));
        throw std::runtime_error("Unknown register");
    }

    auto display = bus_->GetBoundSystem()->GetFirstComponentByType<emulator::component::Display>(emulator::component::IComponent::ComponentType::Display);
    if (!display) {
        spdlog::critical("Display component not found");
        throw std::runtime_error("Display component not found");
    }

    auto x = registers_[reg];
    auto y = registers_[reg2];
    registers_[0xF] = 0;

    for (std::size_t row = 0; row < (opcode & 0x000F); ++row) {
        auto sprite_byte = bus_->Read<std::uint8_t>(I_ + row);

        for (std::size_t j = 0; j < 8; ++j) {
            auto b = (sprite_byte & 0x80) >> 7;
            auto col = (x + j) % display->GetWidth();

            auto previousPixel = display->GetPixel(col, y + row);
            if (b) {
                if (previousPixel != display->clearColor_) {
                    display->SetPixel(col, y + row, display->clearColor_);
                    registers_[0xF] = 1;
                } else {
                    display->SetPixel(col, y + row, pixelOn_);
                }
            }
            sprite_byte <<= 1;
        }
    }
}

}; // namespace emulator::chip8
