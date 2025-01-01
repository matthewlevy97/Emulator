#include <gtest/gtest.h>

#include <emulator.h>
#include <components/multimappedmemory.h>
#include "cpu.h"

using emulator::component::MultiMappedMemory;
using emulator::component::MemoryType;
using emulator::gameboy::CPU;

// TODO: Add tests for all PUSH and POP instructions
// TODO: Add test for all XOR instructions
// TODO: Add test for all ADD instructions
// TODO: Add test for all SUB instructions
// TODO: Add test for all ADC instructions
// TODO: Add test for all SBC instructions
// TODO: Add test for all CP instructions
// TODO: Add test for all AND instructions
// TODO: Add test for all OR instructions
// TODO: Add test for all LD instructions
// TODO: Add test for all Single-Byte instructions

class GameBoyCPUDecode : public ::testing::Test {
protected:
    emulator::component::System* system_;
    CPU* cpu_;
    MultiMappedMemory<MemoryType::ReadWrite>* internalMem_;

    GameBoyCPUDecode()
    {}

    virtual ~GameBoyCPUDecode()
    {}

    virtual void SetUp()
    {
        system_ = CreateSystem();
        cpu_ = reinterpret_cast<CPU*>(system_->GetComponent("CPU"));
        internalMem_ = reinterpret_cast<MultiMappedMemory<MemoryType::ReadWrite>*>(
            system_->GetComponent("Internal8KiBRAM")
        );

        cpu_->SetRegister<CPU::Registers::PC>(0xC000);
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

    void SaveCPUState(CPU& ctx)
    {
        ctx = *cpu_;
    }

    bool ValidateCPUState(CPU& ctx)
    {
        return (
            ctx.GetRegister<CPU::Registers::AF>() == cpu_->GetRegister<CPU::Registers::AF>() &&
            ctx.GetRegister<CPU::Registers::BC>() == cpu_->GetRegister<CPU::Registers::BC>() &&
            ctx.GetRegister<CPU::Registers::DE>() == cpu_->GetRegister<CPU::Registers::DE>() &&
            ctx.GetRegister<CPU::Registers::HL>() == cpu_->GetRegister<CPU::Registers::HL>() &&
            ctx.GetRegister<CPU::Registers::PC>() == cpu_->GetRegister<CPU::Registers::PC>() &&
            ctx.GetRegister<CPU::Registers::SP>() == cpu_->GetRegister<CPU::Registers::SP>()
        );
    }
};

TEST_F(GameBoyCPUDecode, DecodeNOOP)
{
    std::vector<std::uint8_t> opcodes = {0x0, 0x0, 0x0, 0x0};
    LoadData(opcodes);

    CPU state;
    SaveCPUState(state);

    for (int i = 0; i < opcodes.size(); i++) {
        cpu_->ReceiveTick();
        state.AddRegister<CPU::Registers::PC>(1);
    }

    ASSERT_TRUE(ValidateCPUState(state));
}

#pragma region DecodeIncDec16Cycle
#define DecodeIncDec8Cycle(name, opcode, targetReg, val)    \
    TEST_F(GameBoyCPUDecode, Decode##name)                  \
    {                                                       \
        std::vector<std::uint8_t> opcodes = {opcode};       \
        LoadData(opcodes);                                  \
                                                            \
        CPU state;                                          \
        SaveCPUState(state);                                \
                                                            \
        cpu_->ReceiveTick();                                \
        ASSERT_TRUE(ValidateCPUState(state));               \
                                                            \
        cpu_->ReceiveTick();                                \
        state.AddRegister<CPU::Registers::PC>(1);           \
        state.SetRegister<targetReg>(val);                  \
        ASSERT_TRUE(ValidateCPUState(state));               \
    }

DecodeIncDec8Cycle(INC_BC, 0x03, CPU::Registers::BC, 1)
DecodeIncDec8Cycle(DEC_BC, 0x0B, CPU::Registers::BC, -1)
DecodeIncDec8Cycle(INC_DE, 0x13, CPU::Registers::DE, 1)
DecodeIncDec8Cycle(DEC_DE, 0x1B, CPU::Registers::DE, -1)
DecodeIncDec8Cycle(INC_HL, 0x23, CPU::Registers::HL, 1)
DecodeIncDec8Cycle(DEC_HL, 0x2B, CPU::Registers::HL, -1)
DecodeIncDec8Cycle(INC_SP, 0x33, CPU::Registers::SP, 1)
DecodeIncDec8Cycle(DEC_SP, 0x3B, CPU::Registers::SP, -1)

#undef DecodeIncDec8Cycle
#pragma endregion DecodeIncDec16Cycle

#pragma region DecodeIncDec4Cycle
#define DecodeIncDec4Cycle(name, opcode, targetReg, val)    \
    TEST_F(GameBoyCPUDecode, Decode##name)                  \
    {                                                       \
        std::vector<std::uint8_t> opcodes = {opcode};       \
        LoadData(opcodes);                                  \
                                                            \
        CPU state;                                          \
        SaveCPUState(state);                                \
                                                            \
        cpu_->ReceiveTick();                                \
        state.AddRegister<CPU::Registers::PC>(1);           \
        state.SetRegister<targetReg>(val);                  \
        state.SetFlag<CPU::Flags::Z>(false);                \
        state.SetFlag<CPU::Flags::N>(val < 0);              \
        state.SetFlag<CPU::Flags::H>(false);                \
        ASSERT_TRUE(ValidateCPUState(state));               \
                                                            \
        /* Reset state to test setting Z flag */            \
        cpu_->SetRegister<targetReg>(-val);                 \
        cpu_->AddRegister<CPU::Registers::PC>(-1);          \
                                                            \
        cpu_->ReceiveTick();                                \
        state.SetRegister<targetReg>(0);                    \
        state.SetFlag<CPU::Flags::Z>(true);                 \
        state.SetFlag<CPU::Flags::N>(val < 0);              \
        state.SetFlag<CPU::Flags::H>(false);                \
        ASSERT_TRUE(ValidateCPUState(state));               \
                                                            \
        /* Reset state to test setting H flag */            \
        if constexpr (val > 0)                              \
            cpu_->SetRegister<targetReg>(0xF);              \
        else                                                \
            cpu_->SetRegister<targetReg>(0x0);              \
        cpu_->AddRegister<CPU::Registers::PC>(-1);          \
                                                            \
        cpu_->ReceiveTick();                                \
        state.SetRegister<targetReg>(0);                    \
        state.SetFlag<CPU::Flags::Z>(false);                \
        state.SetFlag<CPU::Flags::N>(val < 0);              \
        state.SetFlag<CPU::Flags::H>(false);                \
        ASSERT_TRUE(ValidateCPUState(state));               \
    }

DecodeIncDec4Cycle(INC_B, 0x04, CPU::Registers::B, 1)
DecodeIncDec4Cycle(DEC_B, 0x05, CPU::Registers::B, -1)
DecodeIncDec4Cycle(INC_C, 0x0C, CPU::Registers::C, 1)
DecodeIncDec4Cycle(DEC_C, 0x0D, CPU::Registers::C, -1)
DecodeIncDec4Cycle(INC_D, 0x14, CPU::Registers::D, 1)
DecodeIncDec4Cycle(DEC_D, 0x15, CPU::Registers::D, -1)
DecodeIncDec4Cycle(INC_E, 0x1C, CPU::Registers::E, 1)
DecodeIncDec4Cycle(DEC_E, 0x1D, CPU::Registers::E, -1)
DecodeIncDec4Cycle(INC_H, 0x24, CPU::Registers::H, 1)
DecodeIncDec4Cycle(DEC_H, 0x25, CPU::Registers::H, -1)
DecodeIncDec4Cycle(INC_L, 0x2C, CPU::Registers::L, 1)
DecodeIncDec4Cycle(DEC_L, 0x2D, CPU::Registers::L, -1)
DecodeIncDec4Cycle(INC_A, 0x3C, CPU::Registers::A, 1)
DecodeIncDec4Cycle(DEC_A, 0x3D, CPU::Registers::A, -1)

#undef DecodeIncDec4Cycle
#pragma endregion DecodeIncDec4Cycle
