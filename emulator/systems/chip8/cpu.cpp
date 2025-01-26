#include <spdlog/spdlog.h>

#include "cpu.h"

#include <components/bus.h>
#include <utils.h>

namespace emulator::chip8
{

CPU::CPU()
{
    registers_.fill(0);
    pc_ = 0;
    sp_ = 0;
    stack_.fill(0);
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
    auto opcode = bus_->Read<std::uint16_t>(pc_);
    pc_ += sizeof(opcode);

    std::uint8_t reg = 0;
    std::uint8_t reg2 = 0;

    // Decode and Execute Opcode
    switch (opcode >> 24) {
    case 0x0:
        switch (opcode & 0x00FF) {
        case 0xE0:
            // TODO: Clear the screen
            break;
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
                spdlog::critical("Unknown CB Opcode 0x{:04X} @ 0x{:04X}", opcode, pc_ - sizeof(pc_));
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
        // TODO: Set I = nnn
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
        // TODO: Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision
        break;
    case 0xE:
        switch (opcode & 0x00FF) {
        case 0x9E:
            // TODO: Skip next instruction if key with the value of Vx is pressed
            break;
        case 0xA1:
            // TODO: Skip next instruction if key with the value of Vx is not pressed
            break;
        default:
            spdlog::critical("Unknown CB Opcode 0x{:04X} @ 0x{:04X}", opcode, pc_ - sizeof(pc_));
            throw std::runtime_error("Unknown opcode");
        }
        break;
    case 0xF:
        DecodeFOpcodes(opcode);
        break;
    default:
        spdlog::critical("Unknown CB Opcode 0x{:04X} @ 0x{:04X}", opcode, pc_ - sizeof(pc_));
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
    case 0x07:
        // TODO: Set Vx = delay timer value
        break;
    case 0x0A:
        // TODO: Wait for a key press, store the value of the key in Vx
        break;
    case 0x15:
        // TODO: Set delay timer = Vx
        break;
    case 0x18:
        // TODO: Set sound timer = Vx
        break;
    case 0x1E:
        // TODO: Set I = I + Vx
        break;
    case 0x29:
        // TODO: Set I = location of sprite for digit Vx
        break;
    case 0x33:
        // TODO: Store BCD representation of Vx in memory locations I, I+1, and I+2
        break;
    case 0x55:
        // TODO: Store registers V0 through Vx in memory starting at location I
        break;
    case 0x65:
        // TODO: Read registers V0 through Vx from memory starting at location I
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

}; // namespace emulator::chip8