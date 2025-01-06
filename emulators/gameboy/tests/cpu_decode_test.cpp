#include <gtest/gtest.h>

#include <emulator.h>
#include <components/multimappedmemory.h>
#include "cpu.h"

using emulator::component::MultiMappedMemory;
using emulator::component::MemoryType;
using emulator::gameboy::CPU;

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

    void ValidateCPUState(CPU& ctx)
    {
        ASSERT_EQ(ctx.GetRegister<CPU::Registers::A>(), cpu_->GetRegister<CPU::Registers::A>());
        ASSERT_EQ(ctx.GetRegister<CPU::Registers::BC>(), cpu_->GetRegister<CPU::Registers::BC>());
        ASSERT_EQ(ctx.GetRegister<CPU::Registers::DE>(), cpu_->GetRegister<CPU::Registers::DE>());
        ASSERT_EQ(ctx.GetRegister<CPU::Registers::HL>(), cpu_->GetRegister<CPU::Registers::HL>());
        ASSERT_EQ(ctx.GetRegister<CPU::Registers::PC>(), cpu_->GetRegister<CPU::Registers::PC>());
        ASSERT_EQ(ctx.GetRegister<CPU::Registers::SP>(), cpu_->GetRegister<CPU::Registers::SP>());

        ASSERT_EQ(ctx.GetFlag<CPU::Flags::Z>(), cpu_->GetFlag<CPU::Flags::Z>());
        ASSERT_EQ(ctx.GetFlag<CPU::Flags::N>(), cpu_->GetFlag<CPU::Flags::N>());
        ASSERT_EQ(ctx.GetFlag<CPU::Flags::H>(), cpu_->GetFlag<CPU::Flags::H>());
        ASSERT_EQ(ctx.GetFlag<CPU::Flags::C>(), cpu_->GetFlag<CPU::Flags::C>());
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

    ValidateCPUState(state);
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
        ValidateCPUState(state);                            \
                                                            \
        cpu_->ReceiveTick();                                \
        state.AddRegister<CPU::Registers::PC>(1);           \
        state.SetRegister<targetReg>(val);                  \
        ValidateCPUState(state);                            \
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
#define DecodeIncDec4CycleZ(name, opcode, targetReg, val)   \
    TEST_F(GameBoyCPUDecode, Decode##name##_ZFlag)          \
    {                                                       \
        std::vector<std::uint8_t> opcodes = {opcode};       \
        LoadData(opcodes);                                  \
                                                            \
        CPU state;                                          \
        SaveCPUState(state);                                \
                                                            \
        if constexpr (val > 0)                              \
            cpu_->SetRegister<targetReg>(0xFF);             \
        else                                                \
            cpu_->SetRegister<targetReg>(0x01);             \
                                                            \
        cpu_->ReceiveTick();                                \
        state.AddRegister<CPU::Registers::PC>(1);           \
        state.SetRegister<targetReg>(0);                    \
        state.SetFlag<CPU::Flags::Z>(true);                 \
        state.SetFlag<CPU::Flags::N>(val < 0);              \
        if constexpr (val > 0)                              \
            state.SetFlag<CPU::Flags::H>(true);             \
        else                                                \
            state.SetFlag<CPU::Flags::H>(false);            \
        ValidateCPUState(state);                            \
    }

#define DecodeIncDec4CycleH(name, opcode, targetReg, val)   \
    TEST_F(GameBoyCPUDecode, Decode##name##_HFlag)          \
    {                                                       \
        std::vector<std::uint8_t> opcodes = {opcode};       \
        LoadData(opcodes);                                  \
                                                            \
        CPU state;                                          \
        SaveCPUState(state);                                \
                                                            \
        if constexpr (val > 0)                              \
            cpu_->SetRegister<targetReg>(0x0F);             \
        else                                                \
            cpu_->SetRegister<targetReg>(0x10);             \
                                                            \
        cpu_->ReceiveTick();                                \
        state.AddRegister<CPU::Registers::PC>(1);           \
        if constexpr (val > 0)                              \
            state.SetRegister<targetReg>(0x10);             \
        else                                                \
            state.SetRegister<targetReg>(0x0F);             \
        state.SetFlag<CPU::Flags::Z>(false);                \
        state.SetFlag<CPU::Flags::N>(val < 0);              \
        state.SetFlag<CPU::Flags::H>(true);                 \
        ValidateCPUState(state);                            \
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
        cpu_->ReceiveTick();                                \
        state.AddRegister<CPU::Registers::PC>(1);           \
        state.SetRegister<targetReg>(0x3 + val);            \
        state.SetFlag<CPU::Flags::Z>(false); /* val != 0 */ \
        state.SetFlag<CPU::Flags::N>(val < 0);              \
        state.SetFlag<CPU::Flags::H>(false);                \
        ValidateCPUState(state);                            \
    }                                                       \
    DecodeIncDec4CycleZ(name, opcode, targetReg, val)       \
    DecodeIncDec4CycleH(name, opcode, targetReg, val)

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

#undef DecodeIncDec4CycleZ
#undef DecodeIncDec4CycleH
#undef DecodeIncDec4Cycle
#pragma endregion DecodeIncDec4Cycle

#pragma region DecodePushPop

#define DecodePop(name, opcode, targetReg)                                              \
    TEST_F(GameBoyCPUDecode, DecodePop##name)                                           \
    {                                                                                   \
        std::vector<std::uint8_t> opcodes = {opcode};                                   \
        LoadData(opcodes);                                                              \
                                                                                        \
        /* Set value to pop from stack */                                               \
        cpu_->SetRegister<CPU::Registers::SP>(cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 2); \
        internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::SP>(), 0x34);        \
        internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::SP>() + 1, 0x12);    \
                                                                                        \
        CPU state;                                                                      \
        SaveCPUState(state);                                                            \
                                                                                        \
        /* Need 12 cycles to complete (3 machine cycles) */                             \
        cpu_->ReceiveTick();                                                            \
        state.AddRegister<CPU::Registers::SP>(1);                                       \
        ValidateCPUState(state);                                                        \
                                                                                        \
        cpu_->ReceiveTick();                                                            \
        state.AddRegister<CPU::Registers::SP>(1);                                       \
        ValidateCPUState(state);                                                        \
                                                                                        \
        cpu_->ReceiveTick();                                                            \
        state.AddRegister<CPU::Registers::PC>(1);                                       \
        state.SetRegister<targetReg>(0x1234);                                           \
                                                                                        \
        ValidateCPUState(state);                                                        \
    }

DecodePop(BC, 0xC1, CPU::Registers::BC)
DecodePop(DE, 0xD1, CPU::Registers::DE)
DecodePop(HL, 0xE1, CPU::Registers::HL)
DecodePop(AF, 0xF1, CPU::Registers::AF)

#undef DecodePop

#define DecodePush(name, opcode, targetReg)                                                     \
    TEST_F(GameBoyCPUDecode, DecodePush##name)                                                  \
    {                                                                                           \
        std::vector<std::uint8_t> opcodes = {opcode};                                           \
        LoadData(opcodes);                                                                      \
                                                                                                \
        /* Set compare value on stack */                                                        \
        cpu_->SetRegister<CPU::Registers::SP>(cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 4); \
        internalMem_->WriteUInt16(cpu_->GetRegister<CPU::Registers::SP>() - 2, 0x1234);         \
                                                                                                \
        cpu_->SetRegister<targetReg>(0xCAFE);                                                   \
                                                                                                \
        CPU state;                                                                              \
        SaveCPUState(state);                                                                    \
                                                                                                \
        /* Need 16 cycles to complete (4 machine cycles) */                                     \
        cpu_->ReceiveTick();                                                                    \
        ASSERT_EQ(internalMem_->ReadUInt8(cpu_->GetRegister<CPU::Registers::SP>()), 0x12);      \
        state.SubRegister<CPU::Registers::SP>(1);                                               \
        ValidateCPUState(state);                                                                \
                                                                                                \
        cpu_->ReceiveTick();                                                                    \
        ASSERT_EQ(internalMem_->ReadUInt8(cpu_->GetRegister<CPU::Registers::SP>() + 1), 0xCA);  \
        state.SubRegister<CPU::Registers::SP>(1);                                               \
        ValidateCPUState(state);                                                                \
                                                                                                \
        cpu_->ReceiveTick();                                                                    \
        ASSERT_EQ(internalMem_->ReadUInt8(cpu_->GetRegister<CPU::Registers::SP>() + 1), 0xCA);  \
        ASSERT_EQ(internalMem_->ReadUInt8(cpu_->GetRegister<CPU::Registers::SP>()), 0xFE);      \
        ValidateCPUState(state);                                                                \
                                                                                                \
        cpu_->ReceiveTick();                                                                    \
        state.AddRegister<CPU::Registers::PC>(1);                                               \
        ASSERT_EQ(internalMem_->ReadUInt8(cpu_->GetRegister<CPU::Registers::SP>() + 1), 0xCA);  \
        ASSERT_EQ(internalMem_->ReadUInt8(cpu_->GetRegister<CPU::Registers::SP>()), 0xFE);      \
                                                                                                \
        ValidateCPUState(state);                                                                \
    }

DecodePush(BC, 0xC5, CPU::Registers::BC)
DecodePush(DE, 0xD5, CPU::Registers::DE)
DecodePush(HL, 0xE5, CPU::Registers::HL)
DecodePush(AF, 0xF5, CPU::Registers::AF)

#undef DecodePush

#define DecodePushPop(name, pushOp, popOp, targetReg)               \
    TEST_F(GameBoyCPUDecode, DecodePushPop##name)                   \
    {                                                               \
        std::vector<std::uint8_t> opcodes = {pushOp, popOp};        \
        LoadData(opcodes);                                          \
                                                                    \
        /* Set value to pop from stack */                           \
        cpu_->SetRegister<CPU::Registers::SP>(cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 4); \
        cpu_->SetRegister<targetReg>(0xCAFE);                       \
                                                                    \
        CPU state;                                                  \
        SaveCPUState(state);                                        \
                                                                    \
        for (int i = 4 + 4; --i;) {                                 \
            cpu_->ReceiveTick();                                    \
        }                                                           \
        state.AddRegister<CPU::Registers::PC>(2);                   \
        ValidateCPUState(state);                                    \
    }

DecodePushPop(BC, 0xC5, 0xC1, CPU::Registers::BC)
DecodePushPop(DE, 0xD5, 0xD1, CPU::Registers::DE)
DecodePushPop(HL, 0xE5, 0xE1, CPU::Registers::HL)
DecodePushPop(AF, 0xF5, 0xF1, CPU::Registers::AF)

#undef DecodePushPop

#pragma endregion DecodePushPop

#pragma region DecodeXOR

#define DecodeXOR(name, targetRegister, opcode)                 \
    TEST_F(GameBoyCPUDecode, Decode##name)                      \
    {                                                           \
        std::vector<std::uint8_t> opcodes = {opcode, opcode};   \
        LoadData(opcodes);                                      \
                                                                \
        CPU state;                                              \
        SaveCPUState(state);                                    \
                                                                \
        /* 0b01 XOR 0b10 = 0b11 */                              \
        cpu_->SetRegister<CPU::Registers::A>(0b01);             \
        cpu_->SetRegister<targetRegister>(0b10);                \
        state.SetRegister<targetRegister>(0b10);                \
                                                                \
        cpu_->ReceiveTick();                                    \
        state.AddRegister<CPU::Registers::PC>(1);               \
        state.SetRegister<CPU::Registers::A>(0b11);             \
        state.SetFlag<CPU::Flags::Z>(false);                    \
        state.SetFlag<CPU::Flags::N>(false);                    \
        state.SetFlag<CPU::Flags::H>(false);                    \
        state.SetFlag<CPU::Flags::C>(false);                    \
        ValidateCPUState(state);                                \
                                                                \
        /* 1 XOR 1 = 0 */                                       \
        cpu_->SetRegister<CPU::Registers::A>(1);                \
        cpu_->SetRegister<targetRegister>(1);                   \
        state.SetRegister<targetRegister>(1);                   \
                                                                \
        cpu_->ReceiveTick();                                    \
        state.AddRegister<CPU::Registers::PC>(1);               \
        state.SetRegister<CPU::Registers::A>(0);                \
        state.SetFlag<CPU::Flags::Z>(true);                     \
        state.SetFlag<CPU::Flags::N>(false);                    \
        state.SetFlag<CPU::Flags::H>(false);                    \
        state.SetFlag<CPU::Flags::C>(false);                    \
        ValidateCPUState(state);                                \
    }

DecodeXOR(XOR_B, CPU::Registers::B, 0xA8)
DecodeXOR(XOR_C, CPU::Registers::C, 0xA9)
DecodeXOR(XOR_D, CPU::Registers::D, 0xAA)
DecodeXOR(XOR_E, CPU::Registers::E, 0xAB)
DecodeXOR(XOR_H, CPU::Registers::H, 0xAC)
DecodeXOR(XOR_L, CPU::Registers::L, 0xAD)
DecodeXOR(XOR_A, CPU::Registers::A, 0xAF)

#undef DecodeXOR

TEST_F(GameBoyCPUDecode, DecodeXOR_N)
{
    std::vector<std::uint8_t> opcodes = {0xEE, 0b10, 0xEE, 0b1};
    LoadData(opcodes);

    CPU state;
    SaveCPUState(state);

    /* 0b01 XOR 0b10 = 0b11 */
    cpu_->SetRegister<CPU::Registers::A>(1);

    cpu_->ReceiveTick();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);

    cpu_->ReceiveTick();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0b11);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);

    /* 1 XOR 1 = 0 */
    cpu_->SetRegister<CPU::Registers::A>(1);

    cpu_->ReceiveTick();
    state.AddRegister<CPU::Registers::PC>(1);
    ValidateCPUState(state);

    cpu_->ReceiveTick();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0);
    state.SetFlag<CPU::Flags::Z>(true);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);
}

TEST_F(GameBoyCPUDecode, DecodeXOR_HLAddr)
{
    std::vector<std::uint8_t> opcodes = {0xAE, 0xAE};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::HL>(cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 2);

    CPU state;
    SaveCPUState(state);

    /* 0b01 XOR 0b10 = 0b11 */
    cpu_->SetRegister<CPU::Registers::A>(0b01);
    internalMem_->WriteUInt16(cpu_->GetRegister<CPU::Registers::HL>(), 0b10);

    cpu_->ReceiveTick();
    ValidateCPUState(state);

    cpu_->ReceiveTick();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0b11);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);

    /* 1 XOR 1 = 0 */
    cpu_->SetRegister<CPU::Registers::A>(1);
    internalMem_->WriteUInt16(cpu_->GetRegister<CPU::Registers::HL>(), 1);

    cpu_->ReceiveTick();
    ValidateCPUState(state);

    cpu_->ReceiveTick();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0);
    state.SetFlag<CPU::Flags::Z>(true);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::H>(false);
    state.SetFlag<CPU::Flags::C>(false);
    ValidateCPUState(state);
}

#pragma endregion DecodeXOR

#pragma region DecodeADD

#define DecodeADD(name, opcode, targetReg)              \
    TEST_F(GameBoyCPUDecode, DecodeAdd##name)           \
    {                                                   \
        std::vector<std::uint8_t> opcodes = {opcode};   \
        LoadData(opcodes);                              \
                                                        \
        cpu_->SetRegister<CPU::Registers::A>(4);        \
        cpu_->SetRegister<targetReg>(4);                \
                                                        \
        CPU state;                                      \
        SaveCPUState(state);                            \
                                                        \
        cpu_->ReceiveTick();                            \
        state.AddRegister<CPU::Registers::PC>(1);       \
        state.SetRegister<CPU::Registers::A>(8);        \
        state.SetFlag<CPU::Flags::Z>(false);            \
        state.SetFlag<CPU::Flags::N>(false);            \
        state.SetFlag<CPU::Flags::C>(false);            \
        state.SetFlag<CPU::Flags::H>(false);            \
        ValidateCPUState(state);                        \
    }                                                   \
                                                        \
    TEST_F(GameBoyCPUDecode, DecodeAdd##name##_ZFlag)   \
    {                                                   \
        std::vector<std::uint8_t> opcodes = {opcode};   \
        LoadData(opcodes);                              \
                                                        \
        cpu_->SetRegister<CPU::Registers::A>(0x80);     \
        cpu_->SetRegister<targetReg>(0x80);             \
                                                        \
        CPU state;                                      \
        SaveCPUState(state);                            \
                                                        \
        cpu_->ReceiveTick();                            \
        state.AddRegister<CPU::Registers::PC>(1);       \
        state.SetRegister<CPU::Registers::A>(0x00);     \
        state.SetFlag<CPU::Flags::Z>(true);             \
        state.SetFlag<CPU::Flags::N>(false);            \
        state.SetFlag<CPU::Flags::C>(true);             \
        state.SetFlag<CPU::Flags::H>(false);            \
        ValidateCPUState(state);                        \
    }                                                   \
                                                        \
    TEST_F(GameBoyCPUDecode, DecodeAdd##name##_CFlag)   \
    {                                                   \
        std::vector<std::uint8_t> opcodes = {opcode};   \
        LoadData(opcodes);                              \
                                                        \
        cpu_->SetRegister<CPU::Registers::A>(0xF0);     \
        cpu_->SetRegister<targetReg>(0xF0);             \
                                                        \
        CPU state;                                      \
        SaveCPUState(state);                            \
                                                        \
        cpu_->ReceiveTick();                            \
        state.AddRegister<CPU::Registers::PC>(1);       \
        state.SetRegister<CPU::Registers::A>(0xE0);     \
        state.SetFlag<CPU::Flags::Z>(false);            \
        state.SetFlag<CPU::Flags::N>(false);            \
        state.SetFlag<CPU::Flags::C>(true);             \
        state.SetFlag<CPU::Flags::H>(false);            \
        ValidateCPUState(state);                        \
    }                                                   \
                                                        \
    TEST_F(GameBoyCPUDecode, DecodeAdd##name##_HFlag)   \
    {                                                   \
        std::vector<std::uint8_t> opcodes = {opcode};   \
        LoadData(opcodes);                              \
                                                        \
        cpu_->SetRegister<CPU::Registers::A>(0x0F);     \
        cpu_->SetRegister<targetReg>(0x0F);             \
                                                        \
        CPU state;                                      \
        SaveCPUState(state);                            \
                                                        \
        cpu_->ReceiveTick();                            \
        state.AddRegister<CPU::Registers::PC>(1);       \
        state.SetRegister<CPU::Registers::A>(0x1E);     \
        state.SetFlag<CPU::Flags::Z>(false);            \
        state.SetFlag<CPU::Flags::N>(false);            \
        state.SetFlag<CPU::Flags::C>(false);            \
        state.SetFlag<CPU::Flags::H>(true);             \
        ValidateCPUState(state);                        \
    }

DecodeADD(B, 0x80, CPU::Registers::B)
DecodeADD(C, 0x81, CPU::Registers::C)
DecodeADD(D, 0x82, CPU::Registers::D)
DecodeADD(E, 0x83, CPU::Registers::E)
DecodeADD(H, 0x84, CPU::Registers::H)
DecodeADD(L, 0x85, CPU::Registers::L)
DecodeADD(A, 0x87, CPU::Registers::A)

#undef DecodeADD

TEST_F(GameBoyCPUDecode, DecodeADDHLAddress)
{
    std::vector<std::uint8_t> opcodes = {0x86};
    LoadData(opcodes);

    cpu_->SetRegister<CPU::Registers::HL>(cpu_->GetRegister<CPU::Registers::PC>() + opcodes.size() * 2);
    internalMem_->WriteUInt8(cpu_->GetRegister<CPU::Registers::HL>(), 4);

    cpu_->SetRegister<CPU::Registers::A>(4);

    CPU state;
    SaveCPUState(state);

    cpu_->ReceiveTick();
    ValidateCPUState(state);

    cpu_->ReceiveTick();
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

    cpu_->ReceiveTick();
    ValidateCPUState(state);

    cpu_->ReceiveTick();
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

    cpu_->ReceiveTick();
    ValidateCPUState(state);

    cpu_->ReceiveTick();
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

    cpu_->ReceiveTick();
    ValidateCPUState(state);

    cpu_->ReceiveTick();
    state.AddRegister<CPU::Registers::PC>(1);
    state.SetRegister<CPU::Registers::A>(0x1E);
    state.SetFlag<CPU::Flags::Z>(false);
    state.SetFlag<CPU::Flags::N>(false);
    state.SetFlag<CPU::Flags::C>(false);
    state.SetFlag<CPU::Flags::H>(true);
    ValidateCPUState(state);
}

#define DecodeADDHL(name, opcode, targetReg)                \
    TEST_F(GameBoyCPUDecode, DecodeAdd##name##_LowerCFlag)  \
    {                                                       \
        std::vector<std::uint8_t> opcodes = {opcode};       \
        LoadData(opcodes);                                  \
                                                            \
        cpu_->SetRegister<CPU::Registers::HL>(0x00F0);      \
        cpu_->SetRegister<targetReg>(0x00F0);               \
                                                            \
        CPU state;                                          \
        SaveCPUState(state);                                \
                                                            \
        cpu_->ReceiveTick();                                \
        state.SetRegister<CPU::Registers::L>(0xE0);         \
        state.SetFlag<CPU::Flags::C>(true);                 \
        state.SetFlag<CPU::Flags::H>(false);                \
        ValidateCPUState(state);                            \
                                                            \
        cpu_->ReceiveTick();                                \
        state.AddRegister<CPU::Registers::PC>(1);           \
        state.SetRegister<CPU::Registers::HL>(0x01E0);      \
        state.SetFlag<CPU::Flags::Z>(false);                \
        state.SetFlag<CPU::Flags::N>(false);                \
        state.SetFlag<CPU::Flags::C>(false);                \
        state.SetFlag<CPU::Flags::H>(false);                \
        ValidateCPUState(state);                            \
    }                                                       \
                                                            \
    TEST_F(GameBoyCPUDecode, DecodeAdd##name##_LowerHFlag)  \
    {                                                       \
        std::vector<std::uint8_t> opcodes = {opcode};       \
        LoadData(opcodes);                                  \
                                                            \
        cpu_->SetRegister<CPU::Registers::HL>(0x000F);      \
        cpu_->SetRegister<targetReg>(0x000F);               \
                                                            \
        CPU state;                                          \
        SaveCPUState(state);                                \
                                                            \
        cpu_->ReceiveTick();                                \
        state.SetRegister<CPU::Registers::L>(0x1E);         \
        state.SetFlag<CPU::Flags::C>(false);                \
        state.SetFlag<CPU::Flags::H>(true);                 \
        ValidateCPUState(state);                            \
                                                            \
        cpu_->ReceiveTick();                                \
        state.AddRegister<CPU::Registers::PC>(1);           \
        state.SetRegister<CPU::Registers::HL>(0x001E);      \
        state.SetFlag<CPU::Flags::Z>(false);                \
        state.SetFlag<CPU::Flags::N>(false);                \
        state.SetFlag<CPU::Flags::C>(false);                \
        state.SetFlag<CPU::Flags::H>(false);                \
        ValidateCPUState(state);                            \
    }                                                       \
                                                            \
    TEST_F(GameBoyCPUDecode, DecodeAdd##name##_UpperCFlag)  \
    {                                                       \
        std::vector<std::uint8_t> opcodes = {opcode};       \
        LoadData(opcodes);                                  \
                                                            \
        cpu_->SetRegister<CPU::Registers::HL>(0xF000);      \
        cpu_->SetRegister<targetReg>(0xF000);               \
                                                            \
        CPU state;                                          \
        SaveCPUState(state);                                \
                                                            \
        cpu_->ReceiveTick();                                \
        ValidateCPUState(state);                            \
                                                            \
        cpu_->ReceiveTick();                                \
        state.AddRegister<CPU::Registers::PC>(1);           \
        state.SetRegister<CPU::Registers::HL>(0xE000);      \
        state.SetFlag<CPU::Flags::Z>(false);                \
        state.SetFlag<CPU::Flags::N>(false);                \
        state.SetFlag<CPU::Flags::C>(true);                 \
        state.SetFlag<CPU::Flags::H>(false);                \
        ValidateCPUState(state);                            \
    }                                                       \
                                                            \
    TEST_F(GameBoyCPUDecode, DecodeAdd##name##_UpperHFlag)  \
    {                                                       \
        std::vector<std::uint8_t> opcodes = {opcode};       \
        LoadData(opcodes);                                  \
                                                            \
        cpu_->SetRegister<CPU::Registers::HL>(0x0F00);      \
        cpu_->SetRegister<targetReg>(0x0F00);               \
                                                            \
        CPU state;                                          \
        SaveCPUState(state);                                \
                                                            \
        cpu_->ReceiveTick();                                \
        ValidateCPUState(state);                            \
                                                            \
        cpu_->ReceiveTick();                                \
        state.AddRegister<CPU::Registers::PC>(1);           \
        state.SetRegister<CPU::Registers::HL>(0x1E00);      \
        state.SetFlag<CPU::Flags::Z>(false);                \
        state.SetFlag<CPU::Flags::N>(false);                \
        state.SetFlag<CPU::Flags::C>(false);                \
        state.SetFlag<CPU::Flags::H>(true);                 \
        ValidateCPUState(state);                            \
    }

DecodeADDHL(BC, 0x09, CPU::Registers::BC)
DecodeADDHL(DE, 0x19, CPU::Registers::DE)
DecodeADDHL(HL, 0x29, CPU::Registers::HL)
DecodeADDHL(SP, 0x39, CPU::Registers::SP)

#undef DecodeADDHL

#pragma endregion DecodeADD
