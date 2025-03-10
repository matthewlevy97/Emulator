#include <gtest/gtest.h>

#include <emulator.h>

#include <components/cpu.h>
#include <components/memory.h>
#include <components/multimappedmemory.h>

#include "cpu.h"
#include "names.h"

// TODO: Finish memory layout of GB

using emulator::component::Memory;
using emulator::component::MemoryType;
using emulator::component::MultiMappedMemory;
using emulator::gameboy::CPU;

class GameBoyCPUDecode : public ::testing::Test
{
protected:
    emulator::component::System* system_;
    CPU* cpu_;
    MultiMappedMemory<MemoryType::ReadWrite>* internalMem_;
    Memory<MemoryType::ReadWrite>* upperInternalMem_;

    std::size_t validationCount_;

    GameBoyCPUDecode()
    {
    }

    virtual ~GameBoyCPUDecode()
    {
    }

    virtual void SetUp()
    {
        system_ = CreateSystem();
        cpu_ = reinterpret_cast<CPU*>(system_->GetComponent(emulator::gameboy::kCPUName));
        internalMem_ = reinterpret_cast<MultiMappedMemory<MemoryType::ReadWrite>*>(
            system_->GetComponent(emulator::gameboy::kInternal8KiBRAMName));
        upperInternalMem_ = reinterpret_cast<Memory<MemoryType::ReadWrite>*>(
            system_->GetComponent(emulator::gameboy::kUpperInternalRAMName));

        cpu_->SetRegister<CPU::Registers::PC>(0xC000);

        validationCount_ = 0;
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
        DoMCycle();
    }

    void DoMCycle()
    {
        for (int i = 0; i < CPU::TCycleToMCycle; i++)
            cpu_->ReceiveTick();
    }

    void SaveCPUState(CPU& ctx)
    {
        ctx = *cpu_;
    }

    void ValidateCPUState(CPU& ctx)
    {
        validationCount_++;

        ASSERT_EQ(
            ctx.GetRegister<CPU::Registers::A>(),
            cpu_->GetRegister<CPU::Registers::A>())
            << "Validation Count: " << validationCount_;
        ASSERT_EQ(
            ctx.GetRegister<CPU::Registers::BC>(),
            cpu_->GetRegister<CPU::Registers::BC>())
            << "Validation Count: " << validationCount_;
        ASSERT_EQ(
            ctx.GetRegister<CPU::Registers::DE>(),
            cpu_->GetRegister<CPU::Registers::DE>())
            << "Validation Count: " << validationCount_;
        ASSERT_EQ(
            ctx.GetRegister<CPU::Registers::HL>(),
            cpu_->GetRegister<CPU::Registers::HL>())
            << "Validation Count: " << validationCount_;
        ASSERT_EQ(
            ctx.GetRegister<CPU::Registers::PC>(),
            cpu_->GetRegister<CPU::Registers::PC>())
            << "Validation Count: " << validationCount_;
        ASSERT_EQ(
            ctx.GetRegister<CPU::Registers::SP>(),
            cpu_->GetRegister<CPU::Registers::SP>())
            << "Validation Count: " << validationCount_;

        ASSERT_EQ(
            ctx.GetFlag<CPU::Flags::Z>(),
            cpu_->GetFlag<CPU::Flags::Z>())
            << "Validation Count: " << validationCount_;
        ASSERT_EQ(
            ctx.GetFlag<CPU::Flags::N>(),
            cpu_->GetFlag<CPU::Flags::N>())
            << "Validation Count: " << validationCount_;
        ASSERT_EQ(
            ctx.GetFlag<CPU::Flags::H>(),
            cpu_->GetFlag<CPU::Flags::H>())
            << "Validation Count: " << validationCount_;
        ASSERT_EQ(
            ctx.GetFlag<CPU::Flags::C>(),
            cpu_->GetFlag<CPU::Flags::C>())
            << "Validation Count: " << validationCount_;
    }
};

TEST_F(GameBoyCPUDecode, DecodeNOOP)
{
    std::vector<std::uint8_t> opcodes = {0x0, 0x0, 0x0, 0x0};
    LoadData(opcodes);

    CPU state;
    SaveCPUState(state);

    for (int i = 0; i < opcodes.size(); i++) {
        DoMCycle();
        state.AddRegister<CPU::Registers::PC>(1);
    }

    ValidateCPUState(state);
}

#pragma region DecodeJump

#define DecodeJumpConditional(opcode, name, flag, flagValue) \
    TEST_F(GameBoyCPUDecode, Decode##name##_DoJumpForward)   \
    {                                                        \
        std::vector<std::uint8_t> opcodes = {opcode, 0x5};   \
        LoadData(opcodes);                                   \
                                                             \
        cpu_->SetFlag<flag>(flagValue);                      \
                                                             \
        CPU state;                                           \
        SaveCPUState(state);                                 \
                                                             \
        /* Get r8 and increment PC */                        \
        DoMCycle();                                          \
        state.AddRegister<CPU::Registers::PC>(1);            \
        ValidateCPUState(state);                             \
                                                             \
        /* Calculate Jump */                                 \
        DoMCycle();                                          \
        ValidateCPUState(state);                             \
                                                             \
        /* Validate Jump */                                  \
        DoMCycle();                                          \
        state.AddRegister<CPU::Registers::PC>(0x5);          \
        /* Fetch Next */                                     \
        state.AddRegister<CPU::Registers::PC>(1);            \
        ValidateCPUState(state);                             \
    }                                                        \
                                                             \
    TEST_F(GameBoyCPUDecode, Decode##name##_DoJumpBackward)  \
    {                                                        \
        static constexpr std::uint8_t NOP = 0x00;            \
        /* 0xFB == -5 */                                     \
        std::vector<std::uint8_t> opcodes = {                \
            NOP, NOP, NOP, NOP, NOP, NOP,                    \
            opcode, 0xFB};                                   \
        LoadData(opcodes);                                   \
                                                             \
        cpu_->SetFlag<flag>(flagValue);                      \
                                                             \
        CPU state;                                           \
        SaveCPUState(state);                                 \
                                                             \
        /* Move past NOP */                                  \
        for (int i = 0; i < 6; i++) {                        \
            DoMCycle();                                      \
            state.AddRegister<CPU::Registers::PC>(1);        \
            ValidateCPUState(state);                         \
        };                                                   \
                                                             \
        /* Get r8 and increment PC */                        \
        DoMCycle();                                          \
        state.AddRegister<CPU::Registers::PC>(1);            \
        ValidateCPUState(state);                             \
                                                             \
        /* Calculate Jump */                                 \
        DoMCycle();                                          \
        ValidateCPUState(state);                             \
                                                             \
        /* Validate Jump */                                  \
        DoMCycle();                                          \
        state.SubRegister<CPU::Registers::PC>(0x5);          \
        /* Fetch Next */                                     \
        state.AddRegister<CPU::Registers::PC>(1);            \
        ValidateCPUState(state);                             \
    }

#define DecodeNoJumpConditional(opcode, name, flag, flagValue) \
    TEST_F(GameBoyCPUDecode, Decode##name##_NoJump)            \
    {                                                          \
        std::vector<std::uint8_t> opcodes = {opcode, 0x5};     \
        LoadData(opcodes);                                     \
                                                               \
        cpu_->SetFlag<flag>(!flagValue);                       \
                                                               \
        CPU state;                                             \
        SaveCPUState(state);                                   \
                                                               \
        /* Get r8 and increment PC */                          \
        DoMCycle();                                            \
        state.AddRegister<CPU::Registers::PC>(1);              \
        ValidateCPUState(state);                               \
                                                               \
        /* Determine no jump */                                \
        DoMCycle();                                            \
        state.AddRegister<CPU::Registers::PC>(1);              \
        ValidateCPUState(state);                               \
    }

// Flags don't matter for JR r8
DecodeJumpConditional(0x18, JR, CPU::Flags::Z, false);

DecodeJumpConditional(0x20, JR_NZ, CPU::Flags::Z, false);
DecodeNoJumpConditional(0x20, JR_NZ, CPU::Flags::Z, false);
DecodeJumpConditional(0x28, JR_Z, CPU::Flags::Z, true);
DecodeNoJumpConditional(0x28, JR_Z, CPU::Flags::Z, true);

DecodeJumpConditional(0x30, JR_NC, CPU::Flags::C, false);
DecodeNoJumpConditional(0x30, JR_NC, CPU::Flags::C, false);
DecodeJumpConditional(0x38, JR_C, CPU::Flags::C, true);
DecodeNoJumpConditional(0x38, JR_C, CPU::Flags::C, true);

#undef DecodeJumpConditional
#undef DecodeNoJumpConditional

TEST_F(GameBoyCPUDecode, DecodeJP_Addr)
{
    auto newPCAddress = cpu_->GetRegister<CPU::Registers::PC>();
    newPCAddress += 20;
    std::vector<std::uint8_t> opcodes = {
        0xC3,
        std::uint8_t(newPCAddress & 0xFF),
        std::uint8_t(newPCAddress >> 8)};
    LoadData(opcodes);

    CPU state;
    SaveCPUState(state);

    /* Process Opcode and read lower 8bit */
    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);

    /* Read upper 8bit */
    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);

    /* Set PC */
    DoMCycle();
    state.SetRegister<CPU::Registers::PC>(newPCAddress);
    ValidateCPUState(state);

    /* Cycle NoOp for pipeline refresh */
    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);
}

#define DecodeJumpAbsConditional(opcode, condition, flag, flagValue) \
    TEST_F(GameBoyCPUDecode, DecodeJP_##condition##_Jump)            \
    {                                                                \
        auto newPCAddress = cpu_->GetRegister<CPU::Registers::PC>(); \
        newPCAddress += 20;                                          \
        std::vector<std::uint8_t> opcodes = {                        \
            opcode,                                                  \
            std::uint8_t(newPCAddress & 0xFF),                       \
            std::uint8_t(newPCAddress >> 8)};                        \
        LoadData(opcodes);                                           \
                                                                     \
        cpu_->SetFlag<flag>(flagValue);                              \
                                                                     \
        CPU state;                                                   \
        SaveCPUState(state);                                         \
                                                                     \
        /* Process Opcode and read lower 8bit */                     \
        DoMCycle();                                                  \
        state.AddRegister<CPU::Registers::PC>(1);                    \
        ValidateCPUState(state);                                     \
                                                                     \
        /* Read upper 8bit */                                        \
        DoMCycle();                                                  \
        state.AddRegister<CPU::Registers::PC>(1);                    \
        ValidateCPUState(state);                                     \
                                                                     \
        /* Set PC */                                                 \
        DoMCycle();                                                  \
        state.SetRegister<CPU::Registers::PC>(newPCAddress);         \
        ValidateCPUState(state);                                     \
                                                                     \
        /* Cycle NoOp for pipeline refresh */                        \
        DoMCycle();                                                  \
        state.AddRegister<CPU::Registers::PC>(1);                    \
        ValidateCPUState(state);                                     \
    }                                                                \
                                                                     \
    TEST_F(GameBoyCPUDecode, DecodeJP_##condition##_NoJump)          \
    {                                                                \
        auto newPCAddress = cpu_->GetRegister<CPU::Registers::PC>(); \
        newPCAddress += 20;                                          \
        std::vector<std::uint8_t> opcodes = {                        \
            opcode,                                                  \
            std::uint8_t(newPCAddress & 0xFF),                       \
            std::uint8_t(newPCAddress >> 8)};                        \
        LoadData(opcodes);                                           \
                                                                     \
        cpu_->SetFlag<flag>(!flagValue);                             \
                                                                     \
        CPU state;                                                   \
        SaveCPUState(state);                                         \
                                                                     \
        /* Process Opcode and read lower 8bit */                     \
        DoMCycle();                                                  \
        state.AddRegister<CPU::Registers::PC>(1);                    \
        ValidateCPUState(state);                                     \
                                                                     \
        /* Read upper 8bit */                                        \
        DoMCycle();                                                  \
        state.AddRegister<CPU::Registers::PC>(1);                    \
        ValidateCPUState(state);                                     \
                                                                     \
        /* Cycle NoOp for pipeline refresh */                        \
        DoMCycle();                                                  \
        state.AddRegister<CPU::Registers::PC>(1);                    \
        ValidateCPUState(state);                                     \
    }

DecodeJumpAbsConditional(0xC2, NZ, CPU::Flags::Z, false);
DecodeJumpAbsConditional(0xCA, Z, CPU::Flags::Z, true);
DecodeJumpAbsConditional(0xD2, NC, CPU::Flags::C, false);
DecodeJumpAbsConditional(0xDA, C, CPU::Flags::C, true);

#undef DecodeJumpAbsConditional

TEST_F(GameBoyCPUDecode, DecodeJP_HLAddr)
{
    auto newPCAddress = cpu_->GetRegister<CPU::Registers::PC>();
    newPCAddress += 20;
    std::vector<std::uint8_t> opcodes = {0xE9};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::HL>(newPCAddress);

    CPU state;
    SaveCPUState(state);

    /* Set PC */
    DoMCycle();
    state.SetRegister<CPU::Registers::PC>(newPCAddress);
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);
}

#pragma endregion DecodeJump

#pragma region DecodeIncDec16Cycle
#define DecodeIncDec8Cycle(name, opcode, targetReg, val) \
    TEST_F(GameBoyCPUDecode, Decode##name)               \
    {                                                    \
        std::vector<std::uint8_t> opcodes = {opcode};    \
        LoadData(opcodes);                               \
                                                         \
        CPU state;                                       \
        SaveCPUState(state);                             \
                                                         \
        DoMCycle();                                      \
        ValidateCPUState(state);                         \
                                                         \
        DoMCycle();                                      \
        state.AddRegister<CPU::Registers::PC>(1);        \
        state.SetRegister<targetReg>(val);               \
        ValidateCPUState(state);                         \
    }

DecodeIncDec8Cycle(INC_BC, 0x03, CPU::Registers::BC, 1);
DecodeIncDec8Cycle(DEC_BC, 0x0B, CPU::Registers::BC, -1);
DecodeIncDec8Cycle(INC_DE, 0x13, CPU::Registers::DE, 1);
DecodeIncDec8Cycle(DEC_DE, 0x1B, CPU::Registers::DE, -1);
DecodeIncDec8Cycle(INC_HL, 0x23, CPU::Registers::HL, 1);
DecodeIncDec8Cycle(DEC_HL, 0x2B, CPU::Registers::HL, -1);
DecodeIncDec8Cycle(INC_SP, 0x33, CPU::Registers::SP, 1);
DecodeIncDec8Cycle(DEC_SP, 0x3B, CPU::Registers::SP, -1);

#undef DecodeIncDec8Cycle

#pragma endregion DecodeIncDec16Cycle

#pragma region DecodeIncDecHL

TEST_F(GameBoyCPUDecode, DecodeINC_HLAddr)
{
    std::vector<std::uint8_t> opcodes = {0x34};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::HL>(cpu_->GetRegister<CPU::Registers::PC>() + 10);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0x01);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0x03); // Something to invalidate if not read already
    ValidateCPUState(state);

    DoMCycle();
    ASSERT_EQ(internalMem_->ReadUInt8(cpu_->GetRegister<CPU::Registers::HL>()), 0x02);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeINC_HLAddr_ZFlag)
{
    std::vector<std::uint8_t> opcodes = {0x34};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::HL>(cpu_->GetRegister<CPU::Registers::PC>() + 10);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0xFF);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0x03); // Something to invalidate if not read already
    ValidateCPUState(state);

    DoMCycle();
    ASSERT_EQ(internalMem_->ReadUInt8(cpu_->GetRegister<CPU::Registers::HL>()), 0x00);
    state.SetFlag<CPU::Flags::Z>(true);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(true);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeINC_HLAddr_HFlag)
{
    std::vector<std::uint8_t> opcodes = {0x34};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::HL>(cpu_->GetRegister<CPU::Registers::PC>() + 10);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0x0F);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0x03); // Something to invalidate if not read already
    ValidateCPUState(state);

    DoMCycle();
    ASSERT_EQ(internalMem_->ReadUInt8(cpu_->GetRegister<CPU::Registers::HL>()), 0x10);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(true);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeDEC_HLAddr)
{
    std::vector<std::uint8_t> opcodes = {0x35};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::HL>(cpu_->GetRegister<CPU::Registers::PC>() + 10);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0x02);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0x03); // Something to invalidate if not read already
    ValidateCPUState(state);

    DoMCycle();
    ASSERT_EQ(internalMem_->ReadUInt8(cpu_->GetRegister<CPU::Registers::HL>()), 0x01);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeDEC_HLAddr_ZFlag)
{
    std::vector<std::uint8_t> opcodes = {0x35};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::HL>(cpu_->GetRegister<CPU::Registers::PC>() + 10);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0x01);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0x03); // Something to invalidate if not read already
    ValidateCPUState(state);

    DoMCycle();
    ASSERT_EQ(internalMem_->ReadUInt8(cpu_->GetRegister<CPU::Registers::HL>()), 0x00);
    state.SetFlag<CPU::Flags::Z>(true);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeDEC_HLAddr_HFlag)
{
    std::vector<std::uint8_t> opcodes = {0x35};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::HL>(cpu_->GetRegister<CPU::Registers::PC>() + 10);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0x10);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0x03); // Something to invalidate if not read already
    ValidateCPUState(state);

    DoMCycle();
    ASSERT_EQ(internalMem_->ReadUInt8(cpu_->GetRegister<CPU::Registers::HL>()), 0xF);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(true);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);
}

#pragma endregion DecodeIncDecHL

#pragma region DecodeCall

#define DecodeCallConditional(opcode, condition, flag, flagValue)             \
    TEST_F(GameBoyCPUDecode, DecodeCall_##condition##_Jump)                   \
    {                                                                         \
        auto newPCAddress = cpu_->GetRegister<CPU::Registers::PC>();          \
        newPCAddress += 20;                                                   \
        std::vector<std::uint8_t> opcodes = {                                 \
            opcode,                                                           \
            std::uint8_t(newPCAddress & 0xFF),                                \
            std::uint8_t(newPCAddress >> 8)};                                 \
        LoadData(opcodes);                                                    \
                                                                              \
        cpu_->SetRegister<CPU::Registers::SP>(newPCAddress + 50);             \
        cpu_->SetFlag<flag>(flagValue);                                       \
                                                                              \
        CPU state;                                                            \
        SaveCPUState(state);                                                  \
                                                                              \
        /* Process Opcode and read lower 8bit */                              \
        DoMCycle();                                                           \
        state.AddRegister<CPU::Registers::PC>(1);                             \
        ValidateCPUState(state);                                              \
                                                                              \
        /* Read upper 8bit */                                                 \
        DoMCycle();                                                           \
        state.AddRegister<CPU::Registers::PC>(1);                             \
        ValidateCPUState(state);                                              \
                                                                              \
        /* Dec SP */                                                          \
        DoMCycle();                                                           \
        state.SubRegister<CPU::Registers::SP>(1);                             \
        ValidateCPUState(state);                                              \
                                                                              \
        auto currentPC = state.GetRegister<CPU::Registers::PC>();             \
                                                                              \
        /* Write PC msb and dec SP */                                         \
        DoMCycle();                                                           \
        ASSERT_EQ(                                                            \
            internalMem_->ReadUInt8(state.GetRegister<CPU::Registers::SP>()), \
            currentPC >> 8);                                                  \
        state.SubRegister<CPU::Registers::SP>(1);                             \
        ValidateCPUState(state);                                              \
                                                                              \
        /* Write PC lsb and SP = WZ */                                        \
        DoMCycle();                                                           \
        ASSERT_EQ(                                                            \
            internalMem_->ReadUInt8(state.GetRegister<CPU::Registers::SP>()), \
            currentPC & 0xFF);                                                \
        state.SetRegister<CPU::Registers::PC>(newPCAddress);                  \
        ValidateCPUState(state);                                              \
                                                                              \
        /* Cycle NoOp for pipeline refresh */                                 \
        DoMCycle();                                                           \
        state.AddRegister<CPU::Registers::PC>(1);                             \
        ValidateCPUState(state);                                              \
    }                                                                         \
                                                                              \
    TEST_F(GameBoyCPUDecode, DecodeCall_##condition##_NoJump)                 \
    {                                                                         \
        auto newPCAddress = cpu_->GetRegister<CPU::Registers::PC>();          \
        newPCAddress += 20;                                                   \
        std::vector<std::uint8_t> opcodes = {                                 \
            opcode,                                                           \
            std::uint8_t(newPCAddress & 0xFF),                                \
            std::uint8_t(newPCAddress >> 8)};                                 \
        LoadData(opcodes);                                                    \
                                                                              \
        cpu_->SetFlag<flag>(!flagValue);                                      \
                                                                              \
        CPU state;                                                            \
        SaveCPUState(state);                                                  \
                                                                              \
        /* Process Opcode and read lower 8bit */                              \
        DoMCycle();                                                           \
        state.AddRegister<CPU::Registers::PC>(1);                             \
        ValidateCPUState(state);                                              \
                                                                              \
        /* Read upper 8bit */                                                 \
        DoMCycle();                                                           \
        state.AddRegister<CPU::Registers::PC>(1);                             \
        ValidateCPUState(state);                                              \
                                                                              \
        /* Cycle NoOp for pipeline refresh */                                 \
        DoMCycle();                                                           \
        state.AddRegister<CPU::Registers::PC>(1);                             \
        ValidateCPUState(state);                                              \
    }

DecodeCallConditional(0xC4, NZ, CPU::Flags::Z, false);
DecodeCallConditional(0xCC, Z, CPU::Flags::Z, true);
DecodeCallConditional(0xD4, NC, CPU::Flags::C, false);
DecodeCallConditional(0xDC, C, CPU::Flags::C, true);

#undef DecodeCallConditional

#pragma endregion DecodeCall

#pragma region DecodeRET

TEST_F(GameBoyCPUDecode, DecodeRET)
{
    std::vector<std::uint8_t> opcodes = {0xC9};
    LoadData(opcodes);

    std::uint16_t newPC = cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 2;

    cpu_->SetRegister<CPU::Registers::SP>(
        cpu_->GetRegister<CPU::Registers::PC>() + 50);
    internalMem_->WriteUInt16(
        cpu_->GetRegister<CPU::Registers::SP>(),
        newPC);

    CPU state;
    SaveCPUState(state);

    /* Read Z */
    DoMCycle();
    state.AddRegister<CPU::Registers::SP>(1);
    ValidateCPUState(state);

    /* Read W */
    DoMCycle();
    state.AddRegister<CPU::Registers::SP>(1);
    ValidateCPUState(state);

    /* PC = WZ */
    DoMCycle();
    state.SetRegister<CPU::Registers::PC>(newPC);
    ValidateCPUState(state);

    /* Fetch NoOp */
    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);
}

#define DecodeRETConditional(opcode, name, flag, flagValue)                                 \
    TEST_F(GameBoyCPUDecode, DecodeRET_##name##_Pass)                                       \
    {                                                                                       \
        std::vector<std::uint8_t> opcodes = {opcode};                                       \
        LoadData(opcodes);                                                                  \
                                                                                            \
        std::uint16_t newPC = cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 2; \
                                                                                            \
        cpu_->SetFlag<flag>(flagValue);                                                     \
        cpu_->SetRegister<CPU::Registers::SP>(                                              \
            cpu_->GetRegister<CPU::Registers::PC>() + 50);                                  \
        internalMem_->WriteUInt16(                                                          \
            cpu_->GetRegister<CPU::Registers::SP>(),                                        \
            newPC);                                                                         \
                                                                                            \
        CPU state;                                                                          \
        SaveCPUState(state);                                                                \
                                                                                            \
        /* Pass Check */                                                                    \
        DoMCycle();                                                                         \
        ValidateCPUState(state);                                                            \
                                                                                            \
        /* Read Z */                                                                        \
        DoMCycle();                                                                         \
        state.AddRegister<CPU::Registers::SP>(1);                                           \
        ValidateCPUState(state);                                                            \
                                                                                            \
        /* Read W */                                                                        \
        DoMCycle();                                                                         \
        state.AddRegister<CPU::Registers::SP>(1);                                           \
        ValidateCPUState(state);                                                            \
                                                                                            \
        /* PC = WZ */                                                                       \
        DoMCycle();                                                                         \
        state.SetRegister<CPU::Registers::PC>(newPC);                                       \
        ValidateCPUState(state);                                                            \
                                                                                            \
        /* Fetch NoOp */                                                                    \
        DoMCycle();                                                                         \
        state.AddRegister<CPU::Registers::PC>(1);                                           \
        ValidateCPUState(state);                                                            \
    }                                                                                       \
                                                                                            \
    TEST_F(GameBoyCPUDecode, DecodeRET_##name##_NoRet)                                      \
    {                                                                                       \
        std::vector<std::uint8_t> opcodes = {opcode};                                       \
        LoadData(opcodes);                                                                  \
                                                                                            \
        cpu_->SetFlag<flag>(!flagValue);                                                    \
                                                                                            \
        CPU state;                                                                          \
        SaveCPUState(state);                                                                \
                                                                                            \
        /* Fail Check */                                                                    \
        DoMCycle();                                                                         \
        ValidateCPUState(state);                                                            \
                                                                                            \
        /* Fetch NoOp */                                                                    \
        DoMCycle();                                                                         \
        state.AddRegister<CPU::Registers::PC>(1);                                           \
        ValidateCPUState(state);                                                            \
    }

DecodeRETConditional(0xC0, NZ, CPU::Flags::Z, false);
DecodeRETConditional(0xC8, N, CPU::Flags::Z, true);

DecodeRETConditional(0xD0, NC, CPU::Flags::C, false);
DecodeRETConditional(0xD8, C, CPU::Flags::C, true);

#undef DecodeRETConditional

#pragma endregion DecodeRET

#pragma region DecodeIncDec4Cycle
#define DecodeIncDec4CycleZ(name, opcode, targetReg, val) \
    TEST_F(GameBoyCPUDecode, Decode##name##_ZFlag)        \
    {                                                     \
        std::vector<std::uint8_t> opcodes = {opcode};     \
        LoadData(opcodes);                                \
                                                          \
        CPU state;                                        \
        SaveCPUState(state);                              \
                                                          \
        if constexpr (val > 0)                            \
            cpu_->SetRegister<targetReg>(0xFF);           \
        else                                              \
            cpu_->SetRegister<targetReg>(0x01);           \
                                                          \
        DoMCycle();                                       \
        state.AddRegister<CPU::Registers::PC>(1);         \
        state.SetRegister<targetReg>(0);                  \
        state.SetFlag<CPU::Flags::Z>(true);               \
        state.SetFlag<CPU::Flags::N>(val < 0);            \
        if constexpr (val > 0)                            \
            state.SetFlag<CPU::Flags::H>(true);           \
        else                                              \
            state.SetFlag<CPU::Flags::H>(false);          \
        ValidateCPUState(state);                          \
    }

#define DecodeIncDec4CycleH(name, opcode, targetReg, val) \
    TEST_F(GameBoyCPUDecode, Decode##name##_HFlag)        \
    {                                                     \
        std::vector<std::uint8_t> opcodes = {opcode};     \
        LoadData(opcodes);                                \
                                                          \
        CPU state;                                        \
        SaveCPUState(state);                              \
                                                          \
        if constexpr (val > 0)                            \
            cpu_->SetRegister<targetReg>(0x0F);           \
        else                                              \
            cpu_->SetRegister<targetReg>(0x10);           \
                                                          \
        DoMCycle();                                       \
        state.AddRegister<CPU::Registers::PC>(1);         \
        if constexpr (val > 0)                            \
            state.SetRegister<targetReg>(0x10);           \
        else                                              \
            state.SetRegister<targetReg>(0x0F);           \
        state.SetFlag<CPU::Flags::Z>(false);              \
        state.SetFlag<CPU::Flags::N>(val < 0);            \
        state.SetFlag<CPU::Flags::H>(true);               \
        ValidateCPUState(state);                          \
    }

#define DecodeIncDec4Cycle(name, opcode, targetReg, val)    \
    TEST_F(GameBoyCPUDecode, Decode##name)                  \
    {                                                       \
        std::vector<std::uint8_t> opcodes = {opcode};       \
        LoadData(opcodes);                                  \
                                                            \
        CPU state;                                          \
        SaveCPUState(state);                                \
                                                            \
        cpu_->SetRegister<targetReg>(0x3);                  \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        state.SetRegister<targetReg>(0x3 + val);            \
        state.SetFlag<CPU::Flags::Z>(false); /* val != 0 */ \
        state.SetFlag<CPU::Flags::N>(val < 0);              \
        state.SetFlag<CPU::Flags::H>(false);                \
        ValidateCPUState(state);                            \
    }                                                       \
    DecodeIncDec4CycleZ(name, opcode, targetReg, val);      \
    DecodeIncDec4CycleH(name, opcode, targetReg, val);

DecodeIncDec4Cycle(INC_B, 0x04, CPU::Registers::B, 1);
DecodeIncDec4Cycle(DEC_B, 0x05, CPU::Registers::B, -1);
DecodeIncDec4Cycle(INC_C, 0x0C, CPU::Registers::C, 1);
DecodeIncDec4Cycle(DEC_C, 0x0D, CPU::Registers::C, -1);
DecodeIncDec4Cycle(INC_D, 0x14, CPU::Registers::D, 1);
DecodeIncDec4Cycle(DEC_D, 0x15, CPU::Registers::D, -1);
DecodeIncDec4Cycle(INC_E, 0x1C, CPU::Registers::E, 1);
DecodeIncDec4Cycle(DEC_E, 0x1D, CPU::Registers::E, -1);
DecodeIncDec4Cycle(INC_H, 0x24, CPU::Registers::H, 1);
DecodeIncDec4Cycle(DEC_H, 0x25, CPU::Registers::H, -1);
DecodeIncDec4Cycle(INC_L, 0x2C, CPU::Registers::L, 1);
DecodeIncDec4Cycle(DEC_L, 0x2D, CPU::Registers::L, -1);
DecodeIncDec4Cycle(INC_A, 0x3C, CPU::Registers::A, 1);
DecodeIncDec4Cycle(DEC_A, 0x3D, CPU::Registers::A, -1);

#undef DecodeIncDec4CycleZ
#undef DecodeIncDec4CycleH
#undef DecodeIncDec4Cycle
#pragma endregion DecodeIncDec4Cycle

#pragma region DecodePushPop

#define DecodePop(name, opcode, targetReg)                                           \
    TEST_F(GameBoyCPUDecode, DecodePop##name)                                        \
    {                                                                                \
        std::vector<std::uint8_t> opcodes = {opcode};                                \
        LoadData(opcodes);                                                           \
                                                                                     \
        /* Set value to pop from stack */                                            \
        cpu_->SetRegister<CPU::Registers::SP>(                                       \
            cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 2);           \
        internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::SP>(), 0x34);     \
        internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::SP>() + 1, 0x12); \
                                                                                     \
        CPU state;                                                                   \
        SaveCPUState(state);                                                         \
                                                                                     \
        /* Need 12 cycles to complete (3 machine cycles) */                          \
        DoMCycle();                                                                  \
        state.AddRegister<CPU::Registers::SP>(1);                                    \
        ValidateCPUState(state);                                                     \
                                                                                     \
        DoMCycle();                                                                  \
        state.AddRegister<CPU::Registers::SP>(1);                                    \
        ValidateCPUState(state);                                                     \
                                                                                     \
        DoMCycle();                                                                  \
        state.AddRegister<CPU::Registers::PC>(1);                                    \
        state.SetRegister<targetReg>(0x1234);                                        \
                                                                                     \
        ValidateCPUState(state);                                                     \
    }

DecodePop(BC, 0xC1, CPU::Registers::BC);
DecodePop(DE, 0xD1, CPU::Registers::DE);
DecodePop(HL, 0xE1, CPU::Registers::HL);
DecodePop(AF, 0xF1, CPU::Registers::AF);

#undef DecodePop

#define DecodePush(name, opcode, targetReg)                                                                  \
    TEST_F(GameBoyCPUDecode, DecodePush##name)                                                               \
    {                                                                                                        \
        std::vector<std::uint8_t> opcodes = {opcode};                                                        \
        LoadData(opcodes);                                                                                   \
                                                                                                             \
        /* Set compare value on stack */                                                                     \
        cpu_->SetRegister<CPU::Registers::SP>(cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 4); \
        internalMem_->WriteUInt16(cpu_->GetRegister<CPU::Registers::SP>() - 2, 0x1234);                      \
                                                                                                             \
        cpu_->SetRegister<targetReg>(0xCAFE);                                                                \
                                                                                                             \
        CPU state;                                                                                           \
        SaveCPUState(state);                                                                                 \
                                                                                                             \
        /* Need 16 cycles to complete (4 machine cycles) */                                                  \
        DoMCycle();                                                                                          \
        ASSERT_EQ(internalMem_->ReadUInt8(cpu_->GetRegister<CPU::Registers::SP>()), 0x12);                   \
        state.SubRegister<CPU::Registers::SP>(1);                                                            \
        ValidateCPUState(state);                                                                             \
                                                                                                             \
        DoMCycle();                                                                                          \
        ASSERT_EQ(internalMem_->ReadUInt8(cpu_->GetRegister<CPU::Registers::SP>() + 1), 0xCA);               \
        state.SubRegister<CPU::Registers::SP>(1);                                                            \
        ValidateCPUState(state);                                                                             \
                                                                                                             \
        DoMCycle();                                                                                          \
        ASSERT_EQ(internalMem_->ReadUInt8(cpu_->GetRegister<CPU::Registers::SP>() + 1), 0xCA);               \
        ASSERT_EQ(internalMem_->ReadUInt8(cpu_->GetRegister<CPU::Registers::SP>()), 0xFE);                   \
        ValidateCPUState(state);                                                                             \
                                                                                                             \
        DoMCycle();                                                                                          \
        state.AddRegister<CPU::Registers::PC>(1);                                                            \
        ASSERT_EQ(internalMem_->ReadUInt8(cpu_->GetRegister<CPU::Registers::SP>() + 1), 0xCA);               \
        ASSERT_EQ(internalMem_->ReadUInt8(cpu_->GetRegister<CPU::Registers::SP>()), 0xFE);                   \
                                                                                                             \
        ValidateCPUState(state);                                                                             \
    }

DecodePush(BC, 0xC5, CPU::Registers::BC);
DecodePush(DE, 0xD5, CPU::Registers::DE);
DecodePush(HL, 0xE5, CPU::Registers::HL);
DecodePush(AF, 0xF5, CPU::Registers::AF);

#undef DecodePush

#define DecodePushPop(name, pushOp, popOp, targetReg)                                                        \
    TEST_F(GameBoyCPUDecode, DecodePushPop##name)                                                            \
    {                                                                                                        \
        std::vector<std::uint8_t> opcodes = {pushOp, popOp};                                                 \
        LoadData(opcodes);                                                                                   \
                                                                                                             \
        /* Set value to pop from stack */                                                                    \
        cpu_->SetRegister<CPU::Registers::SP>(cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 4); \
        cpu_->SetRegister<targetReg>(0xCAFE);                                                                \
                                                                                                             \
        CPU state;                                                                                           \
        SaveCPUState(state);                                                                                 \
                                                                                                             \
        for (int i = 4 + 4; --i;) {                                                                          \
            DoMCycle();                                                                                      \
        }                                                                                                    \
        state.AddRegister<CPU::Registers::PC>(2);                                                            \
        ValidateCPUState(state);                                                                             \
    }

DecodePushPop(BC, 0xC5, 0xC1, CPU::Registers::BC);
DecodePushPop(DE, 0xD5, 0xD1, CPU::Registers::DE);
DecodePushPop(HL, 0xE5, 0xE1, CPU::Registers::HL);
DecodePushPop(AF, 0xF5, 0xF1, CPU::Registers::AF);

#undef DecodePushPop

#pragma endregion DecodePushPop

#pragma region DecodeAND

#define DecodeANDZero(name, targetRegister, opcode)   \
    TEST_F(GameBoyCPUDecode, DecodeAND##name##Zero)   \
    {                                                 \
        std::vector<std::uint8_t> opcodes = {opcode}; \
        LoadData(opcodes);                            \
                                                      \
        /* 0b10 AND 0b01 = 0b00 */                    \
        cpu_->SetRegister<CPU::Registers::A>(0b10);   \
        cpu_->SetRegister<targetRegister>(0b01);      \
                                                      \
        CPU state;                                    \
        SaveCPUState(state);                          \
                                                      \
        DoMCycle();                                   \
        state.AddRegister<CPU::Registers::PC>(1);     \
        state.SetRegister<CPU::Registers::A>(0);      \
        state.SetFlag<CPU::Flags::Z>(true);           \
        state.SetFlag<CPU::Flags::N>(false);          \
        state.SetFlag<CPU::Flags::H>(true);           \
        state.SetFlag<CPU::Flags::C>(false);          \
        ValidateCPUState(state);                      \
    }

#define DecodeANDNotZero(name, targetRegister, opcode) \
    TEST_F(GameBoyCPUDecode, DecodeAND##name##NotZero) \
    {                                                  \
        std::vector<std::uint8_t> opcodes = {opcode};  \
        LoadData(opcodes);                             \
                                                       \
        /* 0b10 AND 0b10 = 0b10 */                     \
        cpu_->SetRegister<CPU::Registers::A>(0b10);    \
        cpu_->SetRegister<targetRegister>(0b10);       \
                                                       \
        CPU state;                                     \
        SaveCPUState(state);                           \
                                                       \
        DoMCycle();                                    \
        state.AddRegister<CPU::Registers::PC>(1);      \
        state.SetRegister<CPU::Registers::A>(0b10);    \
        state.SetFlag<CPU::Flags::Z>(false);           \
        state.SetFlag<CPU::Flags::N>(false);           \
        state.SetFlag<CPU::Flags::H>(true);            \
        state.SetFlag<CPU::Flags::C>(false);           \
        ValidateCPUState(state);                       \
    }

#define DecodeAND(name, targetRegister, opcode)  \
    DecodeANDZero(name, targetRegister, opcode); \
    DecodeANDNotZero(name, targetRegister, opcode);

DecodeAND(B, CPU::Registers::B, 0xA0);
DecodeAND(C, CPU::Registers::C, 0xA1);
DecodeAND(D, CPU::Registers::D, 0xA2);
DecodeAND(E, CPU::Registers::E, 0xA3);
DecodeAND(H, CPU::Registers::H, 0xA4);
DecodeAND(L, CPU::Registers::L, 0xA5);

TEST_F(GameBoyCPUDecode, DecodeANDAZero)
{
    std::vector<std::uint8_t> opcodes = {0xA7};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0);
    state.SetFlag<CPU::Flags::Z>(true);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(true);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);
}
DecodeANDNotZero(A, CPU::Registers::A, 0xA7);

#undef DecodeANDZero
#undef DecodeANDNotZero
#undef DecodeAND

TEST_F(GameBoyCPUDecode, DecodeANDN)
{
    std::vector<std::uint8_t> opcodes = {0xE6, 0x1};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0x3);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0x1);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(true);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeANDN_ZFlag)
{
    std::vector<std::uint8_t> opcodes = {0xE6, 0x00};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0x0);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0);
    state.SetFlag<CPU::Flags::Z>(true);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(true);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeANDHLAddr_Zero)
{
    std::vector<std::uint8_t> opcodes = {0xA6};
    LoadData(opcodes);

    /* 0b10 AND 0b01 = 0b00 */
    cpu_->SetRegister<CPU::Registers::A>(0b10);
    cpu_->SetRegister<CPU::Registers::HL>(cpu_->GetRegister<CPU::Registers::PC>() + 10);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0b01);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0);
    state.SetFlag<CPU::Flags::Z>(true);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(true);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeANDHLAddr_NotZero)
{
    std::vector<std::uint8_t> opcodes = {0xA6};
    LoadData(opcodes);

    /* 0b10 AND 0b10 = 0b10 */
    cpu_->SetRegister<CPU::Registers::A>(0b10);
    cpu_->SetRegister<CPU::Registers::HL>(cpu_->GetRegister<CPU::Registers::PC>() + 10);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0b10);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0b10);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(true);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);
}

#pragma endregion DecodeAND

#pragma region DecodeXOR

#define DecodeXORZero(name, targetRegister, opcode)   \
    TEST_F(GameBoyCPUDecode, DecodeXOR##name##Zero)   \
    {                                                 \
        std::vector<std::uint8_t> opcodes = {opcode}; \
        LoadData(opcodes);                            \
                                                      \
        /* 0b11 XOR 0b11 = 0b00 */                    \
        cpu_->SetRegister<CPU::Registers::A>(0b11);   \
        cpu_->SetRegister<targetRegister>(0b11);      \
                                                      \
        CPU state;                                    \
        SaveCPUState(state);                          \
                                                      \
        DoMCycle();                                   \
        state.AddRegister<CPU::Registers::PC>(1);     \
        state.SetRegister<CPU::Registers::A>(0);      \
        state.SetFlag<CPU::Flags::Z>(true);           \
        state.SetFlag<CPU::Flags::N>(false);          \
        state.SetFlag<CPU::Flags::H>(false);          \
        state.SetFlag<CPU::Flags::C>(false);          \
        ValidateCPUState(state);                      \
    }

#define DecodeXORNotZero(name, targetRegister, opcode) \
    TEST_F(GameBoyCPUDecode, DecodeXOR##name##NotZero) \
    {                                                  \
        std::vector<std::uint8_t> opcodes = {opcode};  \
        LoadData(opcodes);                             \
                                                       \
        /* 0b01 XOR 0b10 = 0b11 */                     \
        cpu_->SetRegister<CPU::Registers::A>(0b01);    \
        cpu_->SetRegister<targetRegister>(0b10);       \
                                                       \
        CPU state;                                     \
        SaveCPUState(state);                           \
                                                       \
        DoMCycle();                                    \
        state.AddRegister<CPU::Registers::PC>(1);      \
        state.SetRegister<CPU::Registers::A>(0b11);    \
        state.SetFlag<CPU::Flags::Z>(false);           \
        state.SetFlag<CPU::Flags::N>(false);           \
        state.SetFlag<CPU::Flags::H>(false);           \
        state.SetFlag<CPU::Flags::C>(false);           \
        ValidateCPUState(state);                       \
    }

#define DecodeXOR(name, targetRegister, opcode)  \
    DecodeXORZero(name, targetRegister, opcode); \
    DecodeXORNotZero(name, targetRegister, opcode);

DecodeXOR(XOR_B, CPU::Registers::B, 0xA8);
DecodeXOR(XOR_C, CPU::Registers::C, 0xA9);
DecodeXOR(XOR_D, CPU::Registers::D, 0xAA);
DecodeXOR(XOR_E, CPU::Registers::E, 0xAB);
DecodeXOR(XOR_H, CPU::Registers::H, 0xAC);
DecodeXOR(XOR_L, CPU::Registers::L, 0xAD);
DecodeXORZero(XOR_A, CPU::Registers::A, 0xAF);

#undef DecodeXORZero
#undef DecodeXORNotZero
#undef DecodeXOR

TEST_F(GameBoyCPUDecode, DecodeXORN_Zero)
{
    std::vector<std::uint8_t> opcodes = {0xEE, 0x03};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(3);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0);
    state.SetFlag<CPU::Flags::Z>(true);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeXORN_NotZero)
{
    std::vector<std::uint8_t> opcodes = {0xEE, 0x03};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0b01);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0b10);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeXOR_HLAddrZero)
{
    std::vector<std::uint8_t> opcodes = {0xAE};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(3);
    cpu_->SetRegister<CPU::Registers::HL>(cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 2);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 3);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0);
    state.SetFlag<CPU::Flags::Z>(true);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeXOR_HLAddrNotZero)
{
    std::vector<std::uint8_t> opcodes = {0xAE};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0b01);
    cpu_->SetRegister<CPU::Registers::HL>(cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 2);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0b10);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0b11);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);
}

#pragma endregion DecodeXOR

#pragma region DecodeXOR

#define DecodeORZero(name, targetRegister, opcode)    \
    TEST_F(GameBoyCPUDecode, DecodeOR##name##Zero)    \
    {                                                 \
        std::vector<std::uint8_t> opcodes = {opcode}; \
        LoadData(opcodes);                            \
                                                      \
        /* 0b00 OR 0b00 = 0b00 */                     \
        cpu_->SetRegister<CPU::Registers::A>(0b00);   \
        cpu_->SetRegister<targetRegister>(0b00);      \
                                                      \
        CPU state;                                    \
        SaveCPUState(state);                          \
                                                      \
        DoMCycle();                                   \
        state.AddRegister<CPU::Registers::PC>(1);     \
        state.SetRegister<CPU::Registers::A>(0);      \
        state.SetFlag<CPU::Flags::Z>(true);           \
        state.SetFlag<CPU::Flags::N>(false);          \
        state.SetFlag<CPU::Flags::H>(false);          \
        state.SetFlag<CPU::Flags::C>(false);          \
        ValidateCPUState(state);                      \
    }

#define DecodeORNotZero(name, targetRegister, opcode) \
    TEST_F(GameBoyCPUDecode, DecodeOR##name##NotZero) \
    {                                                 \
        std::vector<std::uint8_t> opcodes = {opcode}; \
        LoadData(opcodes);                            \
                                                      \
        /* 0b11 OR 0b10 = 0b11 */                     \
        cpu_->SetRegister<CPU::Registers::A>(0b11);   \
        cpu_->SetRegister<targetRegister>(0b10);      \
                                                      \
        CPU state;                                    \
        SaveCPUState(state);                          \
                                                      \
        DoMCycle();                                   \
        state.AddRegister<CPU::Registers::PC>(1);     \
        state.SetRegister<CPU::Registers::A>(0b11);   \
        state.SetFlag<CPU::Flags::Z>(false);          \
        state.SetFlag<CPU::Flags::N>(false);          \
        state.SetFlag<CPU::Flags::H>(false);          \
        state.SetFlag<CPU::Flags::C>(false);          \
        ValidateCPUState(state);                      \
    }

#define DecodeOR(name, targetRegister, opcode)  \
    DecodeORZero(name, targetRegister, opcode); \
    DecodeORNotZero(name, targetRegister, opcode);

DecodeOR(B, CPU::Registers::B, 0xB0);
DecodeOR(C, CPU::Registers::C, 0xB1);
DecodeOR(D, CPU::Registers::D, 0xB2);
DecodeOR(E, CPU::Registers::E, 0xB3);
DecodeOR(H, CPU::Registers::H, 0xB4);
DecodeOR(L, CPU::Registers::L, 0xB5);
DecodeORZero(A, CPU::Registers::A, 0xB7);

#undef DecodeORZero
#undef DecodeORNotZero
#undef DecodeOR

TEST_F(GameBoyCPUDecode, DecodeORN_Zero)
{
    std::vector<std::uint8_t> opcodes = {0xF6, 0b00};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0b00);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0);
    state.SetFlag<CPU::Flags::Z>(true);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeORN_NotZero)
{
    std::vector<std::uint8_t> opcodes = {0xF6, 0b11};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0b01);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0b11);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeOR_HLAddrZero)
{
    std::vector<std::uint8_t> opcodes = {0xB6};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0);
    cpu_->SetRegister<CPU::Registers::HL>(cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 2);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0);
    state.SetFlag<CPU::Flags::Z>(true);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeOR_HLAddrNotZero)
{
    std::vector<std::uint8_t> opcodes = {0xB6};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0b101);
    cpu_->SetRegister<CPU::Registers::HL>(cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 2);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0b011);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0b111);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);
}

#pragma endregion DecodeOR

#pragma region DecodeADD

#define DecodeADD(name, opcode, targetReg)            \
    TEST_F(GameBoyCPUDecode, DecodeAdd##name)         \
    {                                                 \
        std::vector<std::uint8_t> opcodes = {opcode}; \
        LoadData(opcodes);                            \
                                                      \
        cpu_->SetRegister<CPU::Registers::A>(4);      \
        cpu_->SetRegister<targetReg>(4);              \
                                                      \
        CPU state;                                    \
        SaveCPUState(state);                          \
                                                      \
        DoMCycle();                                   \
        state.AddRegister<CPU::Registers::PC>(1);     \
        state.SetRegister<CPU::Registers::A>(8);      \
        state.SetFlag<CPU::Flags::Z>(false);          \
        state.SetFlag<CPU::Flags::N>(false);          \
        state.SetFlag<CPU::Flags::C>(false);          \
        state.SetFlag<CPU::Flags::H>(false);          \
        ValidateCPUState(state);                      \
    }                                                 \
                                                      \
    TEST_F(GameBoyCPUDecode, DecodeAdd##name##_ZFlag) \
    {                                                 \
        std::vector<std::uint8_t> opcodes = {opcode}; \
        LoadData(opcodes);                            \
                                                      \
        cpu_->SetRegister<CPU::Registers::A>(0x80);   \
        cpu_->SetRegister<targetReg>(0x80);           \
                                                      \
        CPU state;                                    \
        SaveCPUState(state);                          \
                                                      \
        DoMCycle();                                   \
        state.AddRegister<CPU::Registers::PC>(1);     \
        state.SetRegister<CPU::Registers::A>(0x00);   \
        state.SetFlag<CPU::Flags::Z>(true);           \
        state.SetFlag<CPU::Flags::N>(false);          \
        state.SetFlag<CPU::Flags::C>(true);           \
        state.SetFlag<CPU::Flags::H>(false);          \
        ValidateCPUState(state);                      \
    }                                                 \
                                                      \
    TEST_F(GameBoyCPUDecode, DecodeAdd##name##_CFlag) \
    {                                                 \
        std::vector<std::uint8_t> opcodes = {opcode}; \
        LoadData(opcodes);                            \
                                                      \
        cpu_->SetRegister<CPU::Registers::A>(0xF0);   \
        cpu_->SetRegister<targetReg>(0xF0);           \
                                                      \
        CPU state;                                    \
        SaveCPUState(state);                          \
                                                      \
        DoMCycle();                                   \
        state.AddRegister<CPU::Registers::PC>(1);     \
        state.SetRegister<CPU::Registers::A>(0xE0);   \
        state.SetFlag<CPU::Flags::Z>(false);          \
        state.SetFlag<CPU::Flags::N>(false);          \
        state.SetFlag<CPU::Flags::C>(true);           \
        state.SetFlag<CPU::Flags::H>(false);          \
        ValidateCPUState(state);                      \
    }                                                 \
                                                      \
    TEST_F(GameBoyCPUDecode, DecodeAdd##name##_HFlag) \
    {                                                 \
        std::vector<std::uint8_t> opcodes = {opcode}; \
        LoadData(opcodes);                            \
                                                      \
        cpu_->SetRegister<CPU::Registers::A>(0x0F);   \
        cpu_->SetRegister<targetReg>(0x0F);           \
                                                      \
        CPU state;                                    \
        SaveCPUState(state);                          \
                                                      \
        DoMCycle();                                   \
        state.AddRegister<CPU::Registers::PC>(1);     \
        state.SetRegister<CPU::Registers::A>(0x1E);   \
        state.SetFlag<CPU::Flags::Z>(false);          \
        state.SetFlag<CPU::Flags::N>(false);          \
        state.SetFlag<CPU::Flags::C>(false);          \
        state.SetFlag<CPU::Flags::H>(true);           \
        ValidateCPUState(state);                      \
    }

DecodeADD(B, 0x80, CPU::Registers::B);
DecodeADD(C, 0x81, CPU::Registers::C);
DecodeADD(D, 0x82, CPU::Registers::D);
DecodeADD(E, 0x83, CPU::Registers::E);
DecodeADD(H, 0x84, CPU::Registers::H);
DecodeADD(L, 0x85, CPU::Registers::L);
DecodeADD(A, 0x87, CPU::Registers::A);

#undef DecodeADD

TEST_F(GameBoyCPUDecode, DecodeADDN)
{
    std::vector<std::uint8_t> opcodes = {0xC6, 0x02};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0x3);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(5);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeADDN_ZFlag)
{
    std::vector<std::uint8_t> opcodes = {0xC6, 0x01};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0xFF);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0);
    state.SetFlag<CPU::Flags::Z>(true);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(true);
    state.SetFlag<CPU::Flags::C>(true);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeADDN_HFlag)
{
    std::vector<std::uint8_t> opcodes = {0xC6, 0x0F};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0x01);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0x10);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(true);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeADDN_CFlag)
{
    std::vector<std::uint8_t> opcodes = {0xC6, 0x05};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0xFF);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0x04);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(true);
    state.SetFlag<CPU::Flags::C>(true);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeADDHLAddress)
{
    std::vector<std::uint8_t> opcodes = {0x86};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::HL>(cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 2);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 4);

    cpu_->SetRegister<CPU::Registers::A>(4);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(8);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::C>(false);
    state.SetFlag<CPU::Flags::H>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeADDHLAddress_ZFlag)
{
    std::vector<std::uint8_t> opcodes = {0x86};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::HL>(cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 2);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0x80);

    cpu_->SetRegister<CPU::Registers::A>(0x80);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0x00);
    state.SetFlag<CPU::Flags::Z>(true);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::C>(true);
    state.SetFlag<CPU::Flags::H>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeADDHLAddress_CFlag)
{
    std::vector<std::uint8_t> opcodes = {0x86};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::HL>(cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 2);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0xF0);

    cpu_->SetRegister<CPU::Registers::A>(0xF0);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0xE0);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::C>(true);
    state.SetFlag<CPU::Flags::H>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeADDHLAddress_HFlag)
{
    std::vector<std::uint8_t> opcodes = {0x86};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::HL>(cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 2);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0x0F);

    cpu_->SetRegister<CPU::Registers::A>(0x0F);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0x1E);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::C>(false);
    state.SetFlag<CPU::Flags::H>(true);
    ValidateCPUState(state);
}

#define DecodeADDHL(name, opcode, targetReg)               \
    TEST_F(GameBoyCPUDecode, DecodeAdd##name##_LowerCFlag) \
    {                                                      \
        std::vector<std::uint8_t> opcodes = {opcode};      \
        LoadData(opcodes);                                 \
                                                           \
        cpu_->SetRegister<CPU::Registers::HL>(0x00F0);     \
        cpu_->SetRegister<targetReg>(0x00F0);              \
                                                           \
        CPU state;                                         \
        SaveCPUState(state);                               \
                                                           \
        DoMCycle();                                        \
        state.SetRegister<CPU::Registers::L>(0xE0);        \
        state.SetFlag<CPU::Flags::C>(true);                \
        state.SetFlag<CPU::Flags::H>(false);               \
        ValidateCPUState(state);                           \
                                                           \
        DoMCycle();                                        \
        state.AddRegister<CPU::Registers::PC>(1);          \
        state.SetRegister<CPU::Registers::HL>(0x01E0);     \
        state.SetFlag<CPU::Flags::Z>(false);               \
        state.SetFlag<CPU::Flags::N>(false);               \
        state.SetFlag<CPU::Flags::C>(false);               \
        state.SetFlag<CPU::Flags::H>(false);               \
        ValidateCPUState(state);                           \
    }                                                      \
                                                           \
    TEST_F(GameBoyCPUDecode, DecodeAdd##name##_LowerHFlag) \
    {                                                      \
        std::vector<std::uint8_t> opcodes = {opcode};      \
        LoadData(opcodes);                                 \
                                                           \
        cpu_->SetRegister<CPU::Registers::HL>(0x000F);     \
        cpu_->SetRegister<targetReg>(0x000F);              \
                                                           \
        CPU state;                                         \
        SaveCPUState(state);                               \
                                                           \
        DoMCycle();                                        \
        state.SetRegister<CPU::Registers::L>(0x1E);        \
        state.SetFlag<CPU::Flags::C>(false);               \
        state.SetFlag<CPU::Flags::H>(true);                \
        ValidateCPUState(state);                           \
                                                           \
        DoMCycle();                                        \
        state.AddRegister<CPU::Registers::PC>(1);          \
        state.SetRegister<CPU::Registers::HL>(0x001E);     \
        state.SetFlag<CPU::Flags::Z>(false);               \
        state.SetFlag<CPU::Flags::N>(false);               \
        state.SetFlag<CPU::Flags::C>(false);               \
        state.SetFlag<CPU::Flags::H>(false);               \
        ValidateCPUState(state);                           \
    }                                                      \
                                                           \
    TEST_F(GameBoyCPUDecode, DecodeAdd##name##_UpperCFlag) \
    {                                                      \
        std::vector<std::uint8_t> opcodes = {opcode};      \
        LoadData(opcodes);                                 \
                                                           \
        cpu_->SetRegister<CPU::Registers::HL>(0xF000);     \
        cpu_->SetRegister<targetReg>(0xF000);              \
                                                           \
        CPU state;                                         \
        SaveCPUState(state);                               \
                                                           \
        DoMCycle();                                        \
        ValidateCPUState(state);                           \
                                                           \
        DoMCycle();                                        \
        state.AddRegister<CPU::Registers::PC>(1);          \
        state.SetRegister<CPU::Registers::HL>(0xE000);     \
        state.SetFlag<CPU::Flags::Z>(false);               \
        state.SetFlag<CPU::Flags::N>(false);               \
        state.SetFlag<CPU::Flags::C>(true);                \
        state.SetFlag<CPU::Flags::H>(false);               \
        ValidateCPUState(state);                           \
    }                                                      \
                                                           \
    TEST_F(GameBoyCPUDecode, DecodeAdd##name##_UpperHFlag) \
    {                                                      \
        std::vector<std::uint8_t> opcodes = {opcode};      \
        LoadData(opcodes);                                 \
                                                           \
        cpu_->SetRegister<CPU::Registers::HL>(0x0F00);     \
        cpu_->SetRegister<targetReg>(0x0F00);              \
                                                           \
        CPU state;                                         \
        SaveCPUState(state);                               \
                                                           \
        DoMCycle();                                        \
        ValidateCPUState(state);                           \
                                                           \
        DoMCycle();                                        \
        state.AddRegister<CPU::Registers::PC>(1);          \
        state.SetRegister<CPU::Registers::HL>(0x1E00);     \
        state.SetFlag<CPU::Flags::Z>(false);               \
        state.SetFlag<CPU::Flags::N>(false);               \
        state.SetFlag<CPU::Flags::C>(false);               \
        state.SetFlag<CPU::Flags::H>(true);                \
        ValidateCPUState(state);                           \
    }

DecodeADDHL(BC, 0x09, CPU::Registers::BC);
DecodeADDHL(DE, 0x19, CPU::Registers::DE);
DecodeADDHL(HL, 0x29, CPU::Registers::HL);
DecodeADDHL(SP, 0x39, CPU::Registers::SP);

#undef DecodeADDHL

#pragma endregion DecodeADD

#pragma region DecodeSUB

#define DecodeSUB(opcode, name, targetReg)            \
    TEST_F(GameBoyCPUDecode, DecodeSub##name)         \
    {                                                 \
        std::vector<std::uint8_t> opcodes = {opcode}; \
        LoadData(opcodes);                            \
                                                      \
        cpu_->SetRegister<CPU::Registers::A>(8);      \
        cpu_->SetRegister<targetReg>(4);              \
                                                      \
        CPU state;                                    \
        SaveCPUState(state);                          \
                                                      \
        DoMCycle();                                   \
        state.AddRegister<CPU::Registers::PC>(1);     \
        state.SetRegister<CPU::Registers::A>(4);      \
        state.SetFlag<CPU::Flags::Z>(false);          \
        state.SetFlag<CPU::Flags::N>(true);           \
        state.SetFlag<CPU::Flags::C>(false);          \
        state.SetFlag<CPU::Flags::H>(false);          \
        ValidateCPUState(state);                      \
    }                                                 \
                                                      \
    TEST_F(GameBoyCPUDecode, DecodeSub##name##_ZFlag) \
    {                                                 \
        std::vector<std::uint8_t> opcodes = {opcode}; \
        LoadData(opcodes);                            \
                                                      \
        cpu_->SetRegister<CPU::Registers::A>(0x8);    \
        cpu_->SetRegister<targetReg>(0x8);            \
                                                      \
        CPU state;                                    \
        SaveCPUState(state);                          \
                                                      \
        DoMCycle();                                   \
        state.AddRegister<CPU::Registers::PC>(1);     \
        state.SetRegister<CPU::Registers::A>(0x00);   \
        state.SetFlag<CPU::Flags::Z>(true);           \
        state.SetFlag<CPU::Flags::N>(true);           \
        state.SetFlag<CPU::Flags::C>(false);          \
        state.SetFlag<CPU::Flags::H>(false);          \
        ValidateCPUState(state);                      \
    }                                                 \
                                                      \
    TEST_F(GameBoyCPUDecode, DecodeSub##name##_CFlag) \
    {                                                 \
        std::vector<std::uint8_t> opcodes = {opcode}; \
        LoadData(opcodes);                            \
                                                      \
        cpu_->SetRegister<CPU::Registers::A>(0x00);   \
        cpu_->SetRegister<targetReg>(0x01);           \
                                                      \
        CPU state;                                    \
        SaveCPUState(state);                          \
                                                      \
        DoMCycle();                                   \
        state.AddRegister<CPU::Registers::PC>(1);     \
        state.SetRegister<CPU::Registers::A>(0xFF);   \
        state.SetFlag<CPU::Flags::Z>(false);          \
        state.SetFlag<CPU::Flags::N>(true);           \
        state.SetFlag<CPU::Flags::C>(true);           \
        state.SetFlag<CPU::Flags::H>(true);           \
        ValidateCPUState(state);                      \
    }                                                 \
                                                      \
    TEST_F(GameBoyCPUDecode, DecodeSub##name##_HFlag) \
    {                                                 \
        std::vector<std::uint8_t> opcodes = {opcode}; \
        LoadData(opcodes);                            \
                                                      \
        cpu_->SetRegister<CPU::Registers::A>(0x10);   \
        cpu_->SetRegister<targetReg>(0x0F);           \
                                                      \
        CPU state;                                    \
        SaveCPUState(state);                          \
                                                      \
        DoMCycle();                                   \
        state.AddRegister<CPU::Registers::PC>(1);     \
        state.SetRegister<CPU::Registers::A>(0x01);   \
        state.SetFlag<CPU::Flags::Z>(false);          \
        state.SetFlag<CPU::Flags::N>(true);           \
        state.SetFlag<CPU::Flags::C>(false);          \
        state.SetFlag<CPU::Flags::H>(true);           \
        ValidateCPUState(state);                      \
    }

DecodeSUB(0x90, B, CPU::Registers::B);
DecodeSUB(0x91, C, CPU::Registers::C);
DecodeSUB(0x92, D, CPU::Registers::D);
DecodeSUB(0x93, E, CPU::Registers::E);
DecodeSUB(0x94, H, CPU::Registers::H);
DecodeSUB(0x95, L, CPU::Registers::L);

#undef DecodeSUB

TEST_F(GameBoyCPUDecode, DecodeSUBN)
{
    std::vector<std::uint8_t> opcodes = {0xD6, 0x03};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0x5);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0x2);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(true);
    state.SetFlag<CPU::Flags::H>(false);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeSUBN_ZFlag)
{
    std::vector<std::uint8_t> opcodes = {0xD6, 0x02};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0x2);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0);
    state.SetFlag<CPU::Flags::Z>(true);
    state.SetFlag<CPU::Flags::N>(true);
    state.SetFlag<CPU::Flags::H>(false);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeSUBN_HFlag)
{
    std::vector<std::uint8_t> opcodes = {0xD6, 0x01};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0x10);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0x0F);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(true);
    state.SetFlag<CPU::Flags::H>(true);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeSUBN_CFlag)
{
    std::vector<std::uint8_t> opcodes = {0xD6, 0x01};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0x00);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0xFF);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(true);
    state.SetFlag<CPU::Flags::H>(true);
    state.SetFlag<CPU::Flags::C>(true);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeSubHLAddr)
{
    std::vector<std::uint8_t> opcodes = {0x96};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0x03);
    cpu_->SetRegister<CPU::Registers::HL>(cpu_->GetRegister<CPU::Registers::PC>() + 10);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0x02);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0x05);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0x01);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(true);
    state.SetFlag<CPU::Flags::C>(false);
    state.SetFlag<CPU::Flags::H>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeSubHLAddr_ZFlag)
{
    std::vector<std::uint8_t> opcodes = {0x96};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0x02);
    cpu_->SetRegister<CPU::Registers::HL>(cpu_->GetRegister<CPU::Registers::PC>() + 10);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0x02);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0x05);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0x00);
    state.SetFlag<CPU::Flags::Z>(true);
    state.SetFlag<CPU::Flags::N>(true);
    state.SetFlag<CPU::Flags::C>(false);
    state.SetFlag<CPU::Flags::H>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeSubHLAddr_CFlag)
{
    std::vector<std::uint8_t> opcodes = {0x96};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0x00);
    cpu_->SetRegister<CPU::Registers::HL>(cpu_->GetRegister<CPU::Registers::PC>() + 10);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0x01);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0x05);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0xFF);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(true);
    state.SetFlag<CPU::Flags::C>(true);
    state.SetFlag<CPU::Flags::H>(true);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeSubHLAddr_HFlag)
{
    std::vector<std::uint8_t> opcodes = {0x96};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0x10);
    cpu_->SetRegister<CPU::Registers::HL>(cpu_->GetRegister<CPU::Registers::PC>() + 10);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0x01);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0x05);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0x0F);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(true);
    state.SetFlag<CPU::Flags::C>(false);
    state.SetFlag<CPU::Flags::H>(true);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeSubA)
{
    std::vector<std::uint8_t> opcodes = {0x97};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0xF0);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0);
    state.SetFlag<CPU::Flags::Z>(true);
    state.SetFlag<CPU::Flags::N>(true);
    state.SetFlag<CPU::Flags::C>(false);
    state.SetFlag<CPU::Flags::H>(false);
    ValidateCPUState(state);
}

#pragma endregion DecodeSUB

#pragma region DecodeLD

TEST_F(GameBoyCPUDecode, DecodeLD_u16_SP)
{
    std::uint16_t addr = cpu_->GetRegister<CPU::Registers::PC>() + 10;
    std::vector<std::uint8_t> opcodes = {
        0x08,
        static_cast<std::uint8_t>(addr & 0xFF),
        static_cast<std::uint8_t>(addr >> 8)};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::SP>(0x1234);

    CPU state;
    SaveCPUState(state);

    // Read Z
    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ASSERT_EQ(internalMem_->ReadUInt16(addr), 0x0000);
    ValidateCPUState(state);

    // Read W
    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ASSERT_EQ(internalMem_->ReadUInt16(addr), 0x0000);
    ValidateCPUState(state);

    // Write LSB(SP)
    DoMCycle();
    ASSERT_EQ(internalMem_->ReadUInt8(addr), 0x34);
    ASSERT_EQ(internalMem_->ReadUInt8(addr + 1), 0x00);
    ValidateCPUState(state);

    // Write MSB(SP)
    DoMCycle();
    ASSERT_EQ(internalMem_->ReadUInt8(addr), 0x34);
    ASSERT_EQ(internalMem_->ReadUInt8(addr + 1), 0x12);
    ValidateCPUState(state);

    // Fetch next instruction
    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);
}

#define DecodeLoadD8(opcode, name, targetReg)               \
    TEST_F(GameBoyCPUDecode, DecodeLD_##name##_d8)          \
    {                                                       \
        std::vector<std::uint8_t> opcodes = {opcode, 0xCA}; \
        LoadData(opcodes);                                  \
                                                            \
        CPU state;                                          \
        SaveCPUState(state);                                \
                                                            \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        ValidateCPUState(state);                            \
                                                            \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        state.SetRegister<targetReg>(0xCA);                 \
        ValidateCPUState(state);                            \
    }

DecodeLoadD8(0x06, B, CPU::Registers::B);
DecodeLoadD8(0x0E, C, CPU::Registers::C);
DecodeLoadD8(0x16, D, CPU::Registers::D);
DecodeLoadD8(0x1E, E, CPU::Registers::E);
DecodeLoadD8(0x26, H, CPU::Registers::H);
DecodeLoadD8(0x2E, L, CPU::Registers::L);
DecodeLoadD8(0x3E, A, CPU::Registers::A);

#undef DecodeLoadD8

TEST_F(GameBoyCPUDecode, DecodeLD_HLAddr_d8)
{
    std::vector<std::uint8_t> opcodes = {0x36, 0xCA};
    LoadData(opcodes);

    std::uint16_t hlAddr = cpu_->GetRegister<CPU::Registers::PC>() + 10;
    cpu_->SetRegister<CPU::Registers::HL>(hlAddr);
    internalMem_->WriteUInt8(hlAddr, 0xDE);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);

    DoMCycle();
    ASSERT_EQ(internalMem_->ReadUInt8(hlAddr), 0xCA);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);
}

#define DecodeLoad(opcode, dstName, dstReg, srcName, srcReg) \
    TEST_F(GameBoyCPUDecode, DecodeLD_##dstName##_##srcName) \
    {                                                        \
        std::vector<std::uint8_t> opcodes = {opcode};        \
        LoadData(opcodes);                                   \
                                                             \
        cpu_->SetRegister<srcReg>(0xCA);                     \
                                                             \
        CPU state;                                           \
        SaveCPUState(state);                                 \
                                                             \
        DoMCycle();                                          \
        state.AddRegister<CPU::Registers::PC>(1);            \
        state.SetRegister<dstReg>(0xCA);                     \
        state.SetFlag<CPU::Flags::Z>(false);                 \
        state.SetFlag<CPU::Flags::N>(false);                 \
        state.SetFlag<CPU::Flags::C>(false);                 \
        state.SetFlag<CPU::Flags::H>(false);                 \
        ValidateCPUState(state);                             \
    }

#define DecodeLoadHL(opcode, dstName, dstReg)                                            \
    TEST_F(GameBoyCPUDecode, DecodeLD_##dstName##_##HLAddress)                           \
    {                                                                                    \
        std::vector<std::uint8_t> opcodes = {opcode};                                    \
        LoadData(opcodes);                                                               \
                                                                                         \
        std::uint16_t hl = cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 2; \
        cpu_->SetRegister<CPU::Registers::HL>(hl);                                       \
        internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0xCA);         \
                                                                                         \
        CPU state;                                                                       \
        SaveCPUState(state);                                                             \
                                                                                         \
        DoMCycle();                                                                      \
        ValidateCPUState(state);                                                         \
                                                                                         \
        DoMCycle();                                                                      \
        state.AddRegister<CPU::Registers::PC>(1);                                        \
        state.SetRegister<dstReg>(0xCA);                                                 \
        state.SetFlag<CPU::Flags::Z>(false);                                             \
        state.SetFlag<CPU::Flags::N>(false);                                             \
        state.SetFlag<CPU::Flags::C>(false);                                             \
        state.SetFlag<CPU::Flags::H>(false);                                             \
        ValidateCPUState(state);                                                         \
    }

DecodeLoad(0x40, B, CPU::Registers::B, B, CPU::Registers::B);
DecodeLoad(0x41, B, CPU::Registers::B, C, CPU::Registers::C);
DecodeLoad(0x42, B, CPU::Registers::B, D, CPU::Registers::D);
DecodeLoad(0x43, B, CPU::Registers::B, E, CPU::Registers::E);
DecodeLoad(0x44, B, CPU::Registers::B, H, CPU::Registers::H);
DecodeLoad(0x45, B, CPU::Registers::B, L, CPU::Registers::L);
DecodeLoadHL(0x46, B, CPU::Registers::B);
DecodeLoad(0x47, B, CPU::Registers::B, A, CPU::Registers::A);

DecodeLoad(0x48, C, CPU::Registers::C, B, CPU::Registers::B);
DecodeLoad(0x49, C, CPU::Registers::C, C, CPU::Registers::C);
DecodeLoad(0x4A, C, CPU::Registers::C, D, CPU::Registers::D);
DecodeLoad(0x4B, C, CPU::Registers::C, E, CPU::Registers::E);
DecodeLoad(0x4C, C, CPU::Registers::C, H, CPU::Registers::H);
DecodeLoad(0x4D, C, CPU::Registers::C, L, CPU::Registers::L);
DecodeLoadHL(0x4E, C, CPU::Registers::C);
DecodeLoad(0x4F, C, CPU::Registers::C, A, CPU::Registers::A);

DecodeLoad(0x50, D, CPU::Registers::D, B, CPU::Registers::B);
DecodeLoad(0x51, D, CPU::Registers::D, C, CPU::Registers::C);
DecodeLoad(0x52, D, CPU::Registers::D, D, CPU::Registers::D);
DecodeLoad(0x53, D, CPU::Registers::D, E, CPU::Registers::E);
DecodeLoad(0x54, D, CPU::Registers::D, H, CPU::Registers::H);
DecodeLoad(0x55, D, CPU::Registers::D, L, CPU::Registers::L);
DecodeLoadHL(0x56, D, CPU::Registers::D);
DecodeLoad(0x57, D, CPU::Registers::D, A, CPU::Registers::A);

DecodeLoad(0x58, E, CPU::Registers::E, B, CPU::Registers::B);
DecodeLoad(0x59, E, CPU::Registers::E, C, CPU::Registers::C);
DecodeLoad(0x5A, E, CPU::Registers::E, D, CPU::Registers::D);
DecodeLoad(0x5B, E, CPU::Registers::E, E, CPU::Registers::E);
DecodeLoad(0x5C, E, CPU::Registers::E, H, CPU::Registers::H);
DecodeLoad(0x5D, E, CPU::Registers::E, L, CPU::Registers::L);
DecodeLoadHL(0x5E, E, CPU::Registers::E);
DecodeLoad(0x5F, E, CPU::Registers::E, A, CPU::Registers::A);

DecodeLoad(0x60, H, CPU::Registers::H, B, CPU::Registers::B);
DecodeLoad(0x61, H, CPU::Registers::H, C, CPU::Registers::C);
DecodeLoad(0x62, H, CPU::Registers::H, D, CPU::Registers::D);
DecodeLoad(0x63, H, CPU::Registers::H, E, CPU::Registers::E);
DecodeLoad(0x64, H, CPU::Registers::H, H, CPU::Registers::H);
DecodeLoad(0x65, H, CPU::Registers::H, L, CPU::Registers::L);
DecodeLoadHL(0x66, H, CPU::Registers::H);
DecodeLoad(0x67, H, CPU::Registers::H, A, CPU::Registers::A);

DecodeLoad(0x68, L, CPU::Registers::L, B, CPU::Registers::B);
DecodeLoad(0x69, L, CPU::Registers::L, C, CPU::Registers::C);
DecodeLoad(0x6A, L, CPU::Registers::L, D, CPU::Registers::D);
DecodeLoad(0x6B, L, CPU::Registers::L, E, CPU::Registers::E);
DecodeLoad(0x6C, L, CPU::Registers::L, H, CPU::Registers::H);
DecodeLoad(0x6D, L, CPU::Registers::L, L, CPU::Registers::L);
DecodeLoadHL(0x6E, L, CPU::Registers::L);
DecodeLoad(0x6F, L, CPU::Registers::L, A, CPU::Registers::A);

DecodeLoad(0x78, A, CPU::Registers::A, B, CPU::Registers::B);
DecodeLoad(0x79, A, CPU::Registers::A, C, CPU::Registers::C);
DecodeLoad(0x7A, A, CPU::Registers::A, D, CPU::Registers::D);
DecodeLoad(0x7B, A, CPU::Registers::A, E, CPU::Registers::E);
DecodeLoad(0x7C, A, CPU::Registers::A, H, CPU::Registers::H);
DecodeLoad(0x7D, A, CPU::Registers::A, L, CPU::Registers::L);
DecodeLoadHL(0x7E, A, CPU::Registers::A);
DecodeLoad(0x7F, A, CPU::Registers::A, A, CPU::Registers::A);

#undef DecodeLoadHL
#undef DecodeLoad

#define DecodeLoadHLAddr(opcode, name, targetReg)                                          \
    TEST_F(GameBoyCPUDecode, DecodeLD_HLAddr_##name)                                       \
    {                                                                                      \
        std::vector<std::uint8_t> opcodes = {opcode};                                      \
        LoadData(opcodes);                                                                 \
                                                                                           \
        auto hl = cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 2;            \
        cpu_->SetRegister<CPU::Registers::HL>(hl);                                         \
        internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0xDE);           \
                                                                                           \
        cpu_->SetRegister<targetReg>(0xCA);                                                \
                                                                                           \
        CPU state;                                                                         \
        SaveCPUState(state);                                                               \
                                                                                           \
        DoMCycle();                                                                        \
        ValidateCPUState(state);                                                           \
                                                                                           \
        DoMCycle();                                                                        \
        state.AddRegister<CPU::Registers::PC>(1);                                          \
        state.SetFlag<CPU::Flags::Z>(false);                                               \
        state.SetFlag<CPU::Flags::N>(false);                                               \
        state.SetFlag<CPU::Flags::H>(false);                                               \
        state.SetFlag<CPU::Flags::C>(false);                                               \
        ASSERT_EQ(internalMem_->ReadUInt8(cpu_->GetRegister<CPU::Registers::HL>()), 0xCA); \
        ValidateCPUState(state);                                                           \
    }

DecodeLoadHLAddr(0x70, B, CPU::Registers::B);
DecodeLoadHLAddr(0x71, C, CPU::Registers::C);
DecodeLoadHLAddr(0x72, D, CPU::Registers::D);
DecodeLoadHLAddr(0x73, E, CPU::Registers::E);
DecodeLoadHLAddr(0x74, H, CPU::Registers::H);
DecodeLoadHLAddr(0x75, L, CPU::Registers::L);
DecodeLoadHLAddr(0x77, A, CPU::Registers::A);

#undef DecodeLoadHLAddr

TEST_F(GameBoyCPUDecode, DecodeLD_FromFF00_d8)
{
    static constexpr std::uint16_t kOffset = 0xDE;
    std::vector<std::uint8_t> opcodes = {0xF0, kOffset};
    LoadData(opcodes);

    upperInternalMem_->WriteUInt8(0xFF00 | kOffset, 0xCA);

    CPU state;
    SaveCPUState(state);

    // Read the offset d8
    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);

    // Read value 0xFF00 + d8
    DoMCycle();
    ValidateCPUState(state);

    // Set A
    DoMCycle();
    state.SetRegister<CPU::Registers::A>(0xCA);
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeLD_ToFF00_d8)
{
    static constexpr std::uint16_t kOffset = 0xDE;
    std::vector<std::uint8_t> opcodes = {0xE0, kOffset};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0xCA);
    upperInternalMem_->WriteUInt8(0xFF00 | kOffset, 0xFF);

    CPU state;
    SaveCPUState(state);

    // Read the offset d8
    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ASSERT_EQ(upperInternalMem_->ReadUInt8(0xFF00 | kOffset), 0xFF);
    ValidateCPUState(state);

    // Write the value in A
    DoMCycle();
    ASSERT_EQ(upperInternalMem_->ReadUInt8(0xFF00 | kOffset), 0xCA);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ASSERT_EQ(upperInternalMem_->ReadUInt8(0xFF00 | kOffset), 0xCA);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeLD_ToFF00_C)
{
    std::vector<std::uint8_t> opcodes = {0xE2};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0xCA);
    cpu_->SetRegister<CPU::Registers::C>(0xDE);
    upperInternalMem_->WriteUInt8(0xFF00 | cpu_->GetRegister<CPU::Registers::C>(), 0xFF);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    ASSERT_EQ(upperInternalMem_->ReadUInt8(0xFF00 | cpu_->GetRegister<CPU::Registers::C>()), 0xCA);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ASSERT_EQ(upperInternalMem_->ReadUInt8(0xFF00 | cpu_->GetRegister<CPU::Registers::C>()), 0xCA);
    ValidateCPUState(state);
}

#define DecodeLoadAddr(opcode, name, targetReg)                                   \
    TEST_F(GameBoyCPUDecode, DecodeLD_##name##Address_A)                          \
    {                                                                             \
        std::vector<std::uint8_t> opcodes = {opcode};                             \
        LoadData(opcodes);                                                        \
                                                                                  \
        auto mem = cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 2;  \
        cpu_->SetRegister<targetReg>(mem);                                        \
        internalMem_->WriteUInt8(cpu_->GetRegister<targetReg>(), 0xDE);           \
                                                                                  \
        cpu_->SetRegister<CPU::Registers::A>(0xCA);                               \
                                                                                  \
        CPU state;                                                                \
        SaveCPUState(state);                                                      \
                                                                                  \
        DoMCycle();                                                               \
        ValidateCPUState(state);                                                  \
                                                                                  \
        DoMCycle();                                                               \
        state.AddRegister<CPU::Registers::PC>(1);                                 \
        state.SetFlag<CPU::Flags::Z>(false);                                      \
        state.SetFlag<CPU::Flags::N>(false);                                      \
        state.SetFlag<CPU::Flags::H>(false);                                      \
        state.SetFlag<CPU::Flags::C>(false);                                      \
        ASSERT_EQ(internalMem_->ReadUInt8(cpu_->GetRegister<targetReg>()), 0xCA); \
        ValidateCPUState(state);                                                  \
    }

DecodeLoadAddr(0x02, BC, CPU::Registers::BC);
DecodeLoadAddr(0x12, DE, CPU::Registers::DE);

TEST_F(GameBoyCPUDecode, DecodeLD_ToHLAddressInc)
{
    std::vector<std::uint8_t> opcodes = {0x22};
    LoadData(opcodes);

    auto mem = cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 2;
    cpu_->SetRegister<CPU::Registers::HL>(mem);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0xDE);

    cpu_->SetRegister<CPU::Registers::A>(0xCA);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::HL>(1);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    state.SetFlag<CPU::Flags::C>(false);
    ASSERT_EQ(internalMem_->ReadUInt8(cpu_->GetRegister<CPU::Registers::HL>() - 1), 0xCA);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeLD_ToHLAddressDec)
{
    std::vector<std::uint8_t> opcodes = {0x32};
    LoadData(opcodes);

    auto mem = cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 2;
    cpu_->SetRegister<CPU::Registers::HL>(mem);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0xDE);

    cpu_->SetRegister<CPU::Registers::A>(0xCA);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    state.SubRegister<CPU::Registers::HL>(1);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    state.SetFlag<CPU::Flags::C>(false);
    ASSERT_EQ(internalMem_->ReadUInt8(cpu_->GetRegister<CPU::Registers::HL>() + 1), 0xCA);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeLD_FromBCAddress)
{
    std::vector<std::uint8_t> opcodes = {0x0A};
    LoadData(opcodes);

    auto mem = cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 2;
    cpu_->SetRegister<CPU::Registers::BC>(mem);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::BC>(), 0xCA);

    cpu_->SetRegister<CPU::Registers::A>(0xCA);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0xCA);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeLD_FromDEAddress)
{
    std::vector<std::uint8_t> opcodes = {0x1A};
    LoadData(opcodes);

    auto mem = cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 2;
    cpu_->SetRegister<CPU::Registers::DE>(mem);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::DE>(), 0xCA);

    cpu_->SetRegister<CPU::Registers::A>(0xCA);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0xCA);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeLD_FromHLAddressAsc)
{
    std::vector<std::uint8_t> opcodes = {0x2A};
    LoadData(opcodes);

    auto mem = cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 2;
    cpu_->SetRegister<CPU::Registers::HL>(mem);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0xDE);

    cpu_->SetRegister<CPU::Registers::A>(0xCA);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::HL>(1);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0xDE);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeLD_FromHLAddressDec)
{
    std::vector<std::uint8_t> opcodes = {0x3A};
    LoadData(opcodes);

    auto mem = cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 2;
    cpu_->SetRegister<CPU::Registers::HL>(mem);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0xDE);

    cpu_->SetRegister<CPU::Registers::A>(0xCA);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    state.SubRegister<CPU::Registers::HL>(1);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0xDE);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);
}

#undef DecodeLoadAddr

#define DecodeLoadFromImmediate(opcode, name, targetReg)          \
    TEST_F(GameBoyCPUDecode, DecodeLD##name##_FromImmediate)      \
    {                                                             \
        std::vector<std::uint8_t> opcodes = {opcode, 0xFE, 0xCA}; \
        LoadData(opcodes);                                        \
                                                                  \
        CPU state;                                                \
        SaveCPUState(state);                                      \
                                                                  \
        DoMCycle();                                               \
        state.AddRegister<CPU::Registers::PC>(1);                 \
        ValidateCPUState(state);                                  \
                                                                  \
        DoMCycle();                                               \
        state.AddRegister<CPU::Registers::PC>(1);                 \
        ValidateCPUState(state);                                  \
                                                                  \
        DoMCycle();                                               \
        state.AddRegister<CPU::Registers::PC>(1);                 \
        state.SetRegister<targetReg>(0xCAFE);                     \
        state.SetFlag<CPU::Flags::Z>(false);                      \
        state.SetFlag<CPU::Flags::N>(false);                      \
        state.SetFlag<CPU::Flags::H>(false);                      \
        state.SetFlag<CPU::Flags::C>(false);                      \
        ValidateCPUState(state);                                  \
    }

DecodeLoadFromImmediate(0x01, BC, CPU::Registers::BC);
DecodeLoadFromImmediate(0x11, DE, CPU::Registers::DE);
DecodeLoadFromImmediate(0x21, HL, CPU::Registers::HL);
DecodeLoadFromImmediate(0x31, SP, CPU::Registers::SP);

#undef DecodeLoadFromImmediate

TEST_F(GameBoyCPUDecode, DecodeLD_u16_A)
{
    std::uint16_t writeAddr = cpu_->GetRegister<CPU::Registers::PC>() + 10;
    std::vector<std::uint8_t> opcodes = {
        0xEA,
        static_cast<std::uint8_t>(writeAddr & 0xFF),
        static_cast<std::uint8_t>(writeAddr >> 8)};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0xCA);

    CPU state;
    SaveCPUState(state);

    // Read Z
    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ASSERT_EQ(internalMem_->ReadUInt8(writeAddr), 0x00);
    ValidateCPUState(state);

    // Read W
    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ASSERT_EQ(internalMem_->ReadUInt8(writeAddr), 0x00);
    ValidateCPUState(state);

    // Write Memory
    DoMCycle();
    ASSERT_EQ(internalMem_->ReadUInt8(writeAddr), 0xCA);
    ValidateCPUState(state);

    // Fetch
    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeLD_A_u16)
{
    std::uint16_t writeAddr = cpu_->GetRegister<CPU::Registers::PC>() + 10;
    std::vector<std::uint8_t> opcodes = {
        0xFA,
        static_cast<std::uint8_t>(writeAddr & 0xFF),
        static_cast<std::uint8_t>(writeAddr >> 8)};
    LoadData(opcodes);

    internalMem_->WriteUInt8(writeAddr, 0xCA);

    CPU state;
    SaveCPUState(state);

    // Read Z
    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);

    // Read W
    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);

    // Read Memory
    DoMCycle();
    ValidateCPUState(state);

    // Fetch and Set
    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.AddRegister<CPU::Registers::A>(0xCA);
    ValidateCPUState(state);
}

#pragma endregion DecodeLD

#pragma region DecodeCP

#define DecodeCP(opcode, name, targetReg)                 \
    TEST_F(GameBoyCPUDecode, DecodeCP_##name##_ZFlag)     \
    {                                                     \
        std::vector<std::uint8_t> opcodes = {opcode};     \
        LoadData(opcodes);                                \
                                                          \
        cpu_->SetRegister<CPU::Registers::A>(1);          \
        cpu_->SetRegister<targetReg>(1);                  \
                                                          \
        CPU state;                                        \
        SaveCPUState(state);                              \
                                                          \
        DoMCycle();                                       \
        state.AddRegister<CPU::Registers::PC>(1);         \
        state.SetFlag<CPU::Flags::Z>(true);               \
        state.SetFlag<CPU::Flags::N>(true);               \
        state.SetFlag<CPU::Flags::H>(false);              \
        state.SetFlag<CPU::Flags::C>(false);              \
        ValidateCPUState(state);                          \
    }                                                     \
                                                          \
    TEST_F(GameBoyCPUDecode, DecodeCP_##name##_HFlag)     \
    {                                                     \
        std::vector<std::uint8_t> opcodes = {opcode};     \
        LoadData(opcodes);                                \
                                                          \
        cpu_->SetRegister<CPU::Registers::A>(0b00010000); \
        cpu_->SetRegister<targetReg>(0b00001000);         \
                                                          \
        CPU state;                                        \
        SaveCPUState(state);                              \
                                                          \
        DoMCycle();                                       \
        state.AddRegister<CPU::Registers::PC>(1);         \
        state.SetFlag<CPU::Flags::Z>(false);              \
        state.SetFlag<CPU::Flags::N>(true);               \
        state.SetFlag<CPU::Flags::H>(true);               \
        state.SetFlag<CPU::Flags::C>(false);              \
        ValidateCPUState(state);                          \
    }                                                     \
                                                          \
    TEST_F(GameBoyCPUDecode, DecodeCP_##name##_CFlag)     \
    {                                                     \
        std::vector<std::uint8_t> opcodes = {opcode};     \
        LoadData(opcodes);                                \
                                                          \
        cpu_->SetRegister<CPU::Registers::A>(0x10);       \
        cpu_->SetRegister<targetReg>(0x20);               \
                                                          \
        CPU state;                                        \
        SaveCPUState(state);                              \
                                                          \
        DoMCycle();                                       \
        state.AddRegister<CPU::Registers::PC>(1);         \
        state.SetFlag<CPU::Flags::Z>(false);              \
        state.SetFlag<CPU::Flags::N>(true);               \
        state.SetFlag<CPU::Flags::H>(false);              \
        state.SetFlag<CPU::Flags::C>(true);               \
        ValidateCPUState(state);                          \
    }

DecodeCP(0xB8, B, CPU::Registers::B);
DecodeCP(0xB9, C, CPU::Registers::C);
DecodeCP(0xBA, D, CPU::Registers::D);
DecodeCP(0xBB, E, CPU::Registers::E);
DecodeCP(0xBC, H, CPU::Registers::H);
DecodeCP(0xBD, L, CPU::Registers::L);

#undef DecodeCP

TEST_F(GameBoyCPUDecode, DecodeCP_A)
{
    std::vector<std::uint8_t> opcodes = {0xBF};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0x10);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetFlag<CPU::Flags::Z>(true);
    state.SetFlag<CPU::Flags::N>(true);
    state.SetFlag<CPU::Flags::H>(false);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeCP_HL_ZFlag)
{
    std::vector<std::uint8_t> opcodes = {0xBE};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(1);
    cpu_->SetRegister<CPU::Registers::HL>(cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 2);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 1);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetFlag<CPU::Flags::Z>(true);
    state.SetFlag<CPU::Flags::N>(true);
    state.SetFlag<CPU::Flags::H>(false);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeCP_HL_HFlag)
{
    std::vector<std::uint8_t> opcodes = {0xBE};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0b00010000);
    cpu_->SetRegister<CPU::Registers::HL>(cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 2);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0b00001000);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(true);
    state.SetFlag<CPU::Flags::H>(true);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeCP_HL_CFlag)
{
    std::vector<std::uint8_t> opcodes = {0xBE};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0x10);
    cpu_->SetRegister<CPU::Registers::HL>(cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 2);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 0x20);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(true);
    state.SetFlag<CPU::Flags::H>(false);
    state.SetFlag<CPU::Flags::C>(true);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeCP_u8_ZFlag)
{
    std::vector<std::uint8_t> opcodes = {0xFE, 0x01};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(1);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetFlag<CPU::Flags::Z>(true);
    state.SetFlag<CPU::Flags::N>(true);
    state.SetFlag<CPU::Flags::H>(false);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeCP_u8_HFlag)
{
    std::vector<std::uint8_t> opcodes = {0xFE, 0b00001000};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0b00010000);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(true);
    state.SetFlag<CPU::Flags::H>(true);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeCP_u8_CFlag)
{
    std::vector<std::uint8_t> opcodes = {0xFE, 0x20};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0x10);

    CPU state;
    SaveCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);

    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(true);
    state.SetFlag<CPU::Flags::H>(false);
    state.SetFlag<CPU::Flags::C>(true);
    ValidateCPUState(state);
}

#pragma endregion DecodeCP

#pragma region CB_Bit

#define CB_DecodeBit(opcode, name, targetReg, bit)          \
    TEST_F(GameBoyCPUDecode, CB_BIT_##bit##_##name##_SET)   \
    {                                                       \
        std::vector<std::uint8_t> opcodes = {0xCB, opcode}; \
        LoadData(opcodes);                                  \
                                                            \
        cpu_->SetRegister<targetReg>(1 << (bit));           \
                                                            \
        CPU state;                                          \
        SaveCPUState(state);                                \
                                                            \
        /* Process CB prefix */                             \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        ValidateCPUState(state);                            \
                                                            \
        /* Process CB opcode */                             \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        state.SetFlag<CPU::Flags::Z>(false);                \
        state.SetFlag<CPU::Flags::N>(false);                \
        state.SetFlag<CPU::Flags::H>(true);                 \
        ValidateCPUState(state);                            \
    }                                                       \
                                                            \
    TEST_F(GameBoyCPUDecode, CB_BIT_##bit##_##name##_UNSET) \
    {                                                       \
        std::vector<std::uint8_t> opcodes = {0xCB, opcode}; \
        LoadData(opcodes);                                  \
                                                            \
        cpu_->SetRegister<targetReg>(~(1 << (bit)));        \
                                                            \
        CPU state;                                          \
        SaveCPUState(state);                                \
                                                            \
        /* Process CB prefix */                             \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        ValidateCPUState(state);                            \
                                                            \
        /* Process CB opcode */                             \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        state.SetFlag<CPU::Flags::Z>(true);                 \
        state.SetFlag<CPU::Flags::N>(false);                \
        state.SetFlag<CPU::Flags::H>(true);                 \
        ValidateCPUState(state);                            \
    }

#define CB_DecodeBits(opcodeBase, name, targetReg)       \
    CB_DecodeBit(opcodeBase, name, targetReg, 0);        \
    CB_DecodeBit(opcodeBase + 0x8, name, targetReg, 1);  \
    CB_DecodeBit(opcodeBase + 0x10, name, targetReg, 2); \
    CB_DecodeBit(opcodeBase + 0x18, name, targetReg, 3); \
    CB_DecodeBit(opcodeBase + 0x20, name, targetReg, 4); \
    CB_DecodeBit(opcodeBase + 0x28, name, targetReg, 5); \
    CB_DecodeBit(opcodeBase + 0x30, name, targetReg, 6); \
    CB_DecodeBit(opcodeBase + 0x38, name, targetReg, 7);

CB_DecodeBits(0x40, B, CPU::Registers::B);
CB_DecodeBits(0x41, C, CPU::Registers::C);
CB_DecodeBits(0x42, D, CPU::Registers::D);
CB_DecodeBits(0x43, E, CPU::Registers::E);
CB_DecodeBits(0x44, H, CPU::Registers::H);
CB_DecodeBits(0x45, L, CPU::Registers::L);
CB_DecodeBits(0x47, A, CPU::Registers::A);

#undef CB_DecodeBits
#undef CB_DecodeBit

#define CB_DecodeBit_HLAddr(bit)                                                 \
    TEST_F(GameBoyCPUDecode, CB_BIT_##bit##_##HLAddr##_SET)                      \
    {                                                                            \
        std::vector<std::uint8_t> opcodes = {0xCB, 0x46 + (0x8 * bit)};          \
        LoadData(opcodes);                                                       \
                                                                                 \
        auto mem = cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 2; \
        cpu_->SetRegister<CPU::Registers::HL>(mem);                              \
        internalMem_->WriteUInt8(                                                \
            cpu_->GetRegister<CPU::Registers::HL>(), 1 << (bit));                \
                                                                                 \
        CPU state;                                                               \
        SaveCPUState(state);                                                     \
                                                                                 \
        /* Process CB prefix */                                                  \
        DoMCycle();                                                              \
        state.AddRegister<CPU::Registers::PC>(1);                                \
        ValidateCPUState(state);                                                 \
                                                                                 \
        /* Read HL Address */                                                    \
        DoMCycle();                                                              \
        ValidateCPUState(state);                                                 \
                                                                                 \
        /* Process CB opcode */                                                  \
        DoMCycle();                                                              \
        state.AddRegister<CPU::Registers::PC>(1);                                \
        state.SetFlag<CPU::Flags::Z>(true);                                      \
        state.SetFlag<CPU::Flags::N>(false);                                     \
        state.SetFlag<CPU::Flags::H>(true);                                      \
        ValidateCPUState(state);                                                 \
    }                                                                            \
                                                                                 \
    TEST_F(GameBoyCPUDecode, CB_BIT_##bit##_##HLAddr##_UNSET)                    \
    {                                                                            \
        std::vector<std::uint8_t> opcodes = {0xCB, 0x46 + (0x8 * bit)};          \
        LoadData(opcodes);                                                       \
                                                                                 \
        auto mem = cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 2; \
        cpu_->SetRegister<CPU::Registers::HL>(mem);                              \
        std::uint8_t val = std::uint8_t(~(1 << (bit)));                          \
        internalMem_->WriteUInt8(                                                \
            cpu_->GetRegister<CPU::Registers::HL>(), val);                       \
                                                                                 \
        CPU state;                                                               \
        SaveCPUState(state);                                                     \
                                                                                 \
        /* Process CB prefix */                                                  \
        DoMCycle();                                                              \
        state.AddRegister<CPU::Registers::PC>(1);                                \
        ValidateCPUState(state);                                                 \
                                                                                 \
        /* Read HL Address */                                                    \
        DoMCycle();                                                              \
        ValidateCPUState(state);                                                 \
                                                                                 \
        /* Process CB opcode */                                                  \
        DoMCycle();                                                              \
        state.AddRegister<CPU::Registers::PC>(1);                                \
        state.SetFlag<CPU::Flags::Z>(false);                                     \
        state.SetFlag<CPU::Flags::N>(false);                                     \
        state.SetFlag<CPU::Flags::H>(true);                                      \
        ValidateCPUState(state);                                                 \
    }

CB_DecodeBit_HLAddr(0);
CB_DecodeBit_HLAddr(1);
CB_DecodeBit_HLAddr(2);
CB_DecodeBit_HLAddr(3);
CB_DecodeBit_HLAddr(4);
CB_DecodeBit_HLAddr(5);
CB_DecodeBit_HLAddr(6);
CB_DecodeBit_HLAddr(7);

#undef CB_DecodeBit_HLAddr

#pragma endregion CB_Bit

#pragma region DecodeRLCA

TEST_F(GameBoyCPUDecode, DecodeRLCA_MSBZero)
{
    std::vector<std::uint8_t> opcodes = {0x07};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0b01111111);
    cpu_->SetFlag<CPU::Flags::C>(true);

    CPU state;
    SaveCPUState(state);

    /* Process RLCA */
    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0b11111110);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::C>(false);

    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeRLCA_LSBZero)
{
    std::vector<std::uint8_t> opcodes = {0x07};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0b01111110);
    cpu_->SetFlag<CPU::Flags::C>(true);

    CPU state;
    SaveCPUState(state);

    /* Process RLCA */
    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0b11111100);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::C>(false);

    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeRLCA_ZFlag)
{
    std::vector<std::uint8_t> opcodes = {0x07};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0b00000000);
    cpu_->SetFlag<CPU::Flags::C>(true);

    CPU state;
    SaveCPUState(state);

    /* Process RLCA */
    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::C>(false);

    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    ValidateCPUState(state);
}

#pragma endregion DecodeRLCA

#pragma region CB_RLC

#define CB_DecodeRLC(opcode, name, targetReg)               \
    TEST_F(GameBoyCPUDecode, CB_RCL_##name##_MSBZero)       \
    {                                                       \
        std::vector<std::uint8_t> opcodes = {0xCB, opcode}; \
        LoadData(opcodes);                                  \
                                                            \
        cpu_->SetRegister<targetReg>(0b01111111);           \
        cpu_->SetFlag<CPU::Flags::C>(true);                 \
                                                            \
        CPU state;                                          \
        SaveCPUState(state);                                \
                                                            \
        /* Process CB */                                    \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        ValidateCPUState(state);                            \
                                                            \
        /* Process RL */                                    \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        state.SetRegister<targetReg>(0b11111110);           \
        state.SetFlag<CPU::Flags::Z>(false);                \
        state.SetFlag<CPU::Flags::C>(false);                \
                                                            \
        state.SetFlag<CPU::Flags::N>(false);                \
        state.SetFlag<CPU::Flags::H>(false);                \
        ValidateCPUState(state);                            \
    }                                                       \
                                                            \
    TEST_F(GameBoyCPUDecode, CB_RLC_##name##_LSBZero)       \
    {                                                       \
        std::vector<std::uint8_t> opcodes = {0xCB, opcode}; \
        LoadData(opcodes);                                  \
                                                            \
        cpu_->SetRegister<targetReg>(0b01111110);           \
        cpu_->SetFlag<CPU::Flags::C>(true);                 \
                                                            \
        CPU state;                                          \
        SaveCPUState(state);                                \
                                                            \
        /* Process CB */                                    \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        ValidateCPUState(state);                            \
                                                            \
        /* Process RL */                                    \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        state.SetRegister<targetReg>(0b11111100);           \
        state.SetFlag<CPU::Flags::Z>(false);                \
        state.SetFlag<CPU::Flags::C>(false);                \
                                                            \
        state.SetFlag<CPU::Flags::N>(false);                \
        state.SetFlag<CPU::Flags::H>(false);                \
        ValidateCPUState(state);                            \
    }                                                       \
                                                            \
    TEST_F(GameBoyCPUDecode, CB_RLC_##name##_ZFlag)         \
    {                                                       \
        std::vector<std::uint8_t> opcodes = {0xCB, opcode}; \
        LoadData(opcodes);                                  \
                                                            \
        cpu_->SetRegister<targetReg>(0b00000000);           \
        cpu_->SetFlag<CPU::Flags::C>(true);                 \
                                                            \
        CPU state;                                          \
        SaveCPUState(state);                                \
                                                            \
        /* Process CB */                                    \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        ValidateCPUState(state);                            \
                                                            \
        /* Process RL */                                    \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        state.SetRegister<targetReg>(0);                    \
        state.SetFlag<CPU::Flags::Z>(true);                 \
        state.SetFlag<CPU::Flags::C>(false);                \
                                                            \
        state.SetFlag<CPU::Flags::N>(false);                \
        state.SetFlag<CPU::Flags::H>(false);                \
        ValidateCPUState(state);                            \
    }

CB_DecodeRLC(0x00, B, CPU::Registers::B);
CB_DecodeRLC(0x01, C, CPU::Registers::C);
CB_DecodeRLC(0x02, D, CPU::Registers::D);
CB_DecodeRLC(0x03, E, CPU::Registers::E);
CB_DecodeRLC(0x04, H, CPU::Registers::H);
CB_DecodeRLC(0x05, L, CPU::Registers::L);
// TODO: RL (HL)
CB_DecodeRLC(0x07, A, CPU::Registers::A);

#undef CB_DecodeRLC

#pragma endregion CB_RLC

#pragma region DecodeRRC

TEST_F(GameBoyCPUDecode, DecodeRRCA_MSBZero)
{
    std::vector<std::uint8_t> opcodes = {0x0F};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0b01111111);
    cpu_->SetFlag<CPU::Flags::C>(false);

    CPU state;
    SaveCPUState(state);

    /* Process RRCA */
    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0b10111111);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::C>(true);

    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeRRCA_LSBZero)
{
    std::vector<std::uint8_t> opcodes = {0x0F};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0b11111110);
    cpu_->SetFlag<CPU::Flags::C>(true);

    CPU state;
    SaveCPUState(state);

    /* Process RRCA */
    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0b01111111);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::C>(false);

    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeRRCA_ZFlag)
{
    std::vector<std::uint8_t> opcodes = {0x0F};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0b00000000);
    cpu_->SetFlag<CPU::Flags::C>(true);

    CPU state;
    SaveCPUState(state);

    /* Process RRCA */
    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::C>(false);

    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    ValidateCPUState(state);
}

#pragma endregion DecodeRRC

#pragma region CB_RRC

#define CB_DecodeRRC(opcode, name, targetReg)               \
    TEST_F(GameBoyCPUDecode, CB_RRC_##name##_MSBZero)       \
    {                                                       \
        std::vector<std::uint8_t> opcodes = {0xCB, opcode}; \
        LoadData(opcodes);                                  \
                                                            \
        cpu_->SetRegister<targetReg>(0b01111111);           \
        cpu_->SetFlag<CPU::Flags::C>(false);                \
                                                            \
        CPU state;                                          \
        SaveCPUState(state);                                \
                                                            \
        /* Process CB */                                    \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        ValidateCPUState(state);                            \
                                                            \
        /* Process RL */                                    \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        state.SetRegister<targetReg>(0b10111111);           \
        state.SetFlag<CPU::Flags::Z>(false);                \
        state.SetFlag<CPU::Flags::C>(true);                 \
                                                            \
        state.SetFlag<CPU::Flags::N>(false);                \
        state.SetFlag<CPU::Flags::H>(false);                \
        ValidateCPUState(state);                            \
    }                                                       \
                                                            \
    TEST_F(GameBoyCPUDecode, CB_RRC_##name##_LSBZero)       \
    {                                                       \
        std::vector<std::uint8_t> opcodes = {0xCB, opcode}; \
        LoadData(opcodes);                                  \
                                                            \
        cpu_->SetRegister<targetReg>(0b11111110);           \
        cpu_->SetFlag<CPU::Flags::C>(true);                 \
                                                            \
        CPU state;                                          \
        SaveCPUState(state);                                \
                                                            \
        /* Process CB */                                    \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        ValidateCPUState(state);                            \
                                                            \
        /* Process RL */                                    \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        state.SetRegister<targetReg>(0b01111111);           \
        state.SetFlag<CPU::Flags::Z>(false);                \
        state.SetFlag<CPU::Flags::C>(false);                \
                                                            \
        state.SetFlag<CPU::Flags::N>(false);                \
        state.SetFlag<CPU::Flags::H>(false);                \
        ValidateCPUState(state);                            \
    }                                                       \
                                                            \
    TEST_F(GameBoyCPUDecode, CB_RRC_##name##_ZFlag)         \
    {                                                       \
        std::vector<std::uint8_t> opcodes = {0xCB, opcode}; \
        LoadData(opcodes);                                  \
                                                            \
        cpu_->SetRegister<targetReg>(0b00000000);           \
        cpu_->SetFlag<CPU::Flags::C>(true);                 \
                                                            \
        CPU state;                                          \
        SaveCPUState(state);                                \
                                                            \
        /* Process CB */                                    \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        ValidateCPUState(state);                            \
                                                            \
        /* Process RL */                                    \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        state.SetRegister<targetReg>(0);                    \
        state.SetFlag<CPU::Flags::Z>(true);                 \
        state.SetFlag<CPU::Flags::C>(false);                \
                                                            \
        state.SetFlag<CPU::Flags::N>(false);                \
        state.SetFlag<CPU::Flags::H>(false);                \
        ValidateCPUState(state);                            \
    }

CB_DecodeRRC(0x08, B, CPU::Registers::B);
CB_DecodeRRC(0x09, C, CPU::Registers::C);
CB_DecodeRRC(0x0A, D, CPU::Registers::D);
CB_DecodeRRC(0x0B, E, CPU::Registers::E);
CB_DecodeRRC(0x0C, H, CPU::Registers::H);
CB_DecodeRRC(0x0D, L, CPU::Registers::L);
// TODO: RR (HL)
CB_DecodeRRC(0x0F, A, CPU::Registers::A);

#undef CB_DecodeRRC

#pragma endregion CB_RRC

#pragma region DecodeRLA

TEST_F(GameBoyCPUDecode, DecodeRLA_MSBZero)
{
    std::vector<std::uint8_t> opcodes = {0x17};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0b01111111);
    cpu_->SetFlag<CPU::Flags::C>(true);

    CPU state;
    SaveCPUState(state);

    /* Process RLA */
    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0b11111111);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::C>(false);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeRLA_LSBZero)
{
    std::vector<std::uint8_t> opcodes = {0x17};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0b11111110);
    cpu_->SetFlag<CPU::Flags::C>(true);

    CPU state;
    SaveCPUState(state);

    /* Process RLA */
    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0b11111101);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::C>(true);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeRLA_ZFlag)
{
    std::vector<std::uint8_t> opcodes = {0x17};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0b10000000);
    cpu_->SetFlag<CPU::Flags::C>(false);

    CPU state;
    SaveCPUState(state);

    /* Process RLA */
    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::C>(true);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    ValidateCPUState(state);
}

#pragma endregion DecodeRLA

#pragma region CB_RL

#define CB_DecodeRL(opcode, name, targetReg)                \
    TEST_F(GameBoyCPUDecode, CB_RL_##name##_MSBZero)        \
    {                                                       \
        std::vector<std::uint8_t> opcodes = {0xCB, opcode}; \
        LoadData(opcodes);                                  \
                                                            \
        cpu_->SetRegister<targetReg>(0b01111111);           \
        cpu_->SetFlag<CPU::Flags::C>(true);                 \
                                                            \
        CPU state;                                          \
        SaveCPUState(state);                                \
                                                            \
        /* Process CB */                                    \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        ValidateCPUState(state);                            \
                                                            \
        /* Process RL */                                    \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        state.SetRegister<targetReg>(0b11111111);           \
        state.SetFlag<CPU::Flags::Z>(false);                \
        state.SetFlag<CPU::Flags::C>(false);                \
                                                            \
        state.SetFlag<CPU::Flags::N>(false);                \
        state.SetFlag<CPU::Flags::H>(false);                \
        ValidateCPUState(state);                            \
    }                                                       \
                                                            \
    TEST_F(GameBoyCPUDecode, CB_RL_##name##_LSBZero)        \
    {                                                       \
        std::vector<std::uint8_t> opcodes = {0xCB, opcode}; \
        LoadData(opcodes);                                  \
                                                            \
        cpu_->SetRegister<targetReg>(0b11111110);           \
        cpu_->SetFlag<CPU::Flags::C>(true);                 \
                                                            \
        CPU state;                                          \
        SaveCPUState(state);                                \
                                                            \
        /* Process CB */                                    \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        ValidateCPUState(state);                            \
                                                            \
        /* Process RL */                                    \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        state.SetRegister<targetReg>(0b11111101);           \
        state.SetFlag<CPU::Flags::Z>(false);                \
        state.SetFlag<CPU::Flags::C>(true);                 \
                                                            \
        state.SetFlag<CPU::Flags::N>(false);                \
        state.SetFlag<CPU::Flags::H>(false);                \
        ValidateCPUState(state);                            \
    }                                                       \
                                                            \
    TEST_F(GameBoyCPUDecode, CB_RL_##name##_ZFlag)          \
    {                                                       \
        std::vector<std::uint8_t> opcodes = {0xCB, opcode}; \
        LoadData(opcodes);                                  \
                                                            \
        cpu_->SetRegister<targetReg>(0b10000000);           \
        cpu_->SetFlag<CPU::Flags::C>(false);                \
                                                            \
        CPU state;                                          \
        SaveCPUState(state);                                \
                                                            \
        /* Process CB */                                    \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        ValidateCPUState(state);                            \
                                                            \
        /* Process RL */                                    \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        state.SetRegister<targetReg>(0);                    \
        state.SetFlag<CPU::Flags::Z>(true);                 \
        state.SetFlag<CPU::Flags::C>(true);                 \
                                                            \
        state.SetFlag<CPU::Flags::N>(false);                \
        state.SetFlag<CPU::Flags::H>(false);                \
        ValidateCPUState(state);                            \
    }

CB_DecodeRL(0x10, B, CPU::Registers::B);
CB_DecodeRL(0x11, C, CPU::Registers::C);
CB_DecodeRL(0x12, D, CPU::Registers::D);
CB_DecodeRL(0x13, E, CPU::Registers::E);
CB_DecodeRL(0x14, H, CPU::Registers::H);
CB_DecodeRL(0x15, L, CPU::Registers::L);
// TODO: RL (HL)
CB_DecodeRL(0x17, A, CPU::Registers::A);

#undef CB_DecodeRL

#pragma endregion CB_RL

#pragma region DecodeRRA

TEST_F(GameBoyCPUDecode, DecodeRRA_MSBZero)
{
    std::vector<std::uint8_t> opcodes = {0x1F};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0b01111111);
    cpu_->SetFlag<CPU::Flags::C>(true);

    CPU state;
    SaveCPUState(state);

    /* Process RRA */
    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0b10111111);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::C>(true);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeRRA_LSBZero)
{
    std::vector<std::uint8_t> opcodes = {0x1F};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0b11111110);
    cpu_->SetFlag<CPU::Flags::C>(false);

    CPU state;
    SaveCPUState(state);

    /* Process RRA */
    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0b01111111);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::C>(false);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeRRA_ZFlag)
{
    std::vector<std::uint8_t> opcodes = {0x1F};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::A>(0b00000001);
    cpu_->SetFlag<CPU::Flags::C>(false);

    CPU state;
    SaveCPUState(state);

    /* Process RRA */
    DoMCycle();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::C>(true);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    ValidateCPUState(state);
}

#pragma endregion DecodeRRA

#pragma region CB_RR

#define CB_DecodeRR(opcode, name, targetReg)                \
    TEST_F(GameBoyCPUDecode, CB_RR_##name##_MSBZero)        \
    {                                                       \
        std::vector<std::uint8_t> opcodes = {0xCB, opcode}; \
        LoadData(opcodes);                                  \
                                                            \
        cpu_->SetRegister<targetReg>(0b01111111);           \
        cpu_->SetFlag<CPU::Flags::C>(true);                 \
                                                            \
        CPU state;                                          \
        SaveCPUState(state);                                \
                                                            \
        /* Process CB */                                    \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        ValidateCPUState(state);                            \
                                                            \
        /* Process RL */                                    \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        state.SetRegister<targetReg>(0b10111111);           \
        state.SetFlag<CPU::Flags::Z>(false);                \
        state.SetFlag<CPU::Flags::C>(true);                 \
                                                            \
        state.SetFlag<CPU::Flags::N>(false);                \
        state.SetFlag<CPU::Flags::H>(false);                \
        ValidateCPUState(state);                            \
    }                                                       \
                                                            \
    TEST_F(GameBoyCPUDecode, CB_RR_##name##_LSBZero)        \
    {                                                       \
        std::vector<std::uint8_t> opcodes = {0xCB, opcode}; \
        LoadData(opcodes);                                  \
                                                            \
        cpu_->SetRegister<targetReg>(0b11111110);           \
        cpu_->SetFlag<CPU::Flags::C>(false);                \
                                                            \
        CPU state;                                          \
        SaveCPUState(state);                                \
                                                            \
        /* Process CB */                                    \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        ValidateCPUState(state);                            \
                                                            \
        /* Process RL */                                    \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        state.SetRegister<targetReg>(0b01111111);           \
        state.SetFlag<CPU::Flags::Z>(false);                \
        state.SetFlag<CPU::Flags::C>(false);                \
                                                            \
        state.SetFlag<CPU::Flags::N>(false);                \
        state.SetFlag<CPU::Flags::H>(false);                \
        ValidateCPUState(state);                            \
    }                                                       \
                                                            \
    TEST_F(GameBoyCPUDecode, CB_RR_##name##_ZFlag)          \
    {                                                       \
        std::vector<std::uint8_t> opcodes = {0xCB, opcode}; \
        LoadData(opcodes);                                  \
                                                            \
        cpu_->SetRegister<targetReg>(0b00000001);           \
        cpu_->SetFlag<CPU::Flags::C>(false);                \
                                                            \
        CPU state;                                          \
        SaveCPUState(state);                                \
                                                            \
        /* Process CB */                                    \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        ValidateCPUState(state);                            \
                                                            \
        /* Process RL */                                    \
        DoMCycle();                                         \
        state.AddRegister<CPU::Registers::PC>(1);           \
        state.SetRegister<targetReg>(0);                    \
        state.SetFlag<CPU::Flags::Z>(true);                 \
        state.SetFlag<CPU::Flags::C>(true);                 \
                                                            \
        state.SetFlag<CPU::Flags::N>(false);                \
        state.SetFlag<CPU::Flags::H>(false);                \
        ValidateCPUState(state);                            \
    }

CB_DecodeRR(0x18, B, CPU::Registers::B);
CB_DecodeRR(0x19, C, CPU::Registers::C);
CB_DecodeRR(0x1A, D, CPU::Registers::D);
CB_DecodeRR(0x1B, E, CPU::Registers::E);
CB_DecodeRR(0x1C, H, CPU::Registers::H);
CB_DecodeRR(0x1D, L, CPU::Registers::L);
// TODO: RR (HL)
CB_DecodeRR(0x1F, A, CPU::Registers::A);

#undef CB_DecodeRR

#pragma endregion CB_RR
