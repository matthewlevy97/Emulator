#include <gtest/gtest.h>

#include <emulator.h>
#include <components/multimappedmemory.h>
#include "cpu.h"

using emulator::component::MultiMappedMemory;
using emulator::component::MemoryType;

class GameBoyCPUDecode : public ::testing::Test {
protected:
    emulator::component::System* system_;
    emulator::gameboy::CPU* cpu_;
    MultiMappedMemory<MemoryType::ReadWrite>* internalMem_;

    GameBoyCPUDecode()
    {}

    virtual ~GameBoyCPUDecode()
    {}

    virtual void SetUp()
    {
        system_ = CreateSystem();
        cpu_ = reinterpret_cast<emulator::gameboy::CPU*>(system_->GetComponent("CPU"));
        internalMem_ = reinterpret_cast<MultiMappedMemory<MemoryType::ReadWrite>*>(
            system_->GetComponent("Internal8KiBRAM")
        );

        cpu_->SetRegister<emulator::gameboy::CPU::Registers::PC>(0xC000);
    }

    virtual void TearDown()
    {
        delete system_;
    }

    void LoadData(std::vector<std::uint8_t> data)
    {
        for (std::size_t i = 0; i < data.size(); i++) {
            internalMem_->WriteUInt8(0xC000 + i, data[i]);
        }

        // Fetch first opcode
        cpu_->ReceiveTick();
    }

    std::array<std::uint16_t, 6> SaveCPUState()
    {
        return {
            cpu_->GetRegister<emulator::gameboy::CPU::Registers::AF>(),
            cpu_->GetRegister<emulator::gameboy::CPU::Registers::BC>(),
            cpu_->GetRegister<emulator::gameboy::CPU::Registers::DE>(),
            cpu_->GetRegister<emulator::gameboy::CPU::Registers::HL>(),
            cpu_->GetRegister<emulator::gameboy::CPU::Registers::PC>(),
            cpu_->GetRegister<emulator::gameboy::CPU::Registers::SP>(),
        };
    }

    bool ValidateCPUState(std::array<std::uint16_t, 6> state)
    {
        return (
            state[0] == cpu_->GetRegister<emulator::gameboy::CPU::Registers::AF>() &&
            state[1] == cpu_->GetRegister<emulator::gameboy::CPU::Registers::BC>() &&
            state[2] == cpu_->GetRegister<emulator::gameboy::CPU::Registers::DE>() &&
            state[3] == cpu_->GetRegister<emulator::gameboy::CPU::Registers::HL>() &&
            state[4] == cpu_->GetRegister<emulator::gameboy::CPU::Registers::PC>() &&
            state[5] == cpu_->GetRegister<emulator::gameboy::CPU::Registers::SP>()
        );
    }
};

TEST_F(GameBoyCPUDecode, DecodeNOOP)
{
    std::vector<std::uint8_t> opcodes = {0x0, 0x0, 0x0, 0x0};
    LoadData(opcodes);

    auto state = SaveCPUState();

    for (int i = 0; i < opcodes.size(); i++) {
        cpu_->ReceiveTick();
        state[4]++;
    }

    ASSERT_TRUE(ValidateCPUState(state));
}