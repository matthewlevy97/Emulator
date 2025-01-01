#include <gtest/gtest.h>

#include "cpu.h"

TEST(GameBoyCPU, RegisterRW_AF)
{
    auto cpu = emulator::gameboy::CPU();
    ASSERT_EQ(cpu.GetRegister<emulator::gameboy::CPU::Registers::AF>(), 0);
    cpu.SetRegister<emulator::gameboy::CPU::Registers::AF>(0x1234);
    ASSERT_EQ(cpu.GetRegister<emulator::gameboy::CPU::Registers::AF>(), 0x1234);
}

TEST(GameBoyCPU, RegisterRW_Flags)
{
    auto cpu = emulator::gameboy::CPU();
    ASSERT_EQ(cpu.GetRegister<emulator::gameboy::CPU::Registers::AF>(), 0);

    cpu.SetFlag<emulator::gameboy::CPU::Flags::Z>(true);
    ASSERT_EQ(cpu.GetFlag<emulator::gameboy::CPU::Flags::Z>(), true);
    ASSERT_EQ(cpu.GetRegister<emulator::gameboy::CPU::Registers::AF>(), 1 << 7);

    cpu.SetRegister<emulator::gameboy::CPU::Registers::A>(0x55);
    ASSERT_EQ(cpu.GetRegister<emulator::gameboy::CPU::Registers::AF>(), (0x55 << 8) | (1 << 7));

    cpu.SetFlag<emulator::gameboy::CPU::Flags::H>(true);
    ASSERT_EQ(cpu.GetRegister<emulator::gameboy::CPU::Registers::AF>(), (0x55 << 8) | (1 << 7) | (1 << 5));
}

TEST(GameBoyCPU, RegisterRW_HL)
{
    auto cpu = emulator::gameboy::CPU();
    ASSERT_EQ(cpu.GetRegister<emulator::gameboy::CPU::Registers::HL>(), 0);
    cpu.SetRegister<emulator::gameboy::CPU::Registers::HL>(0x1234);
    ASSERT_EQ(cpu.GetRegister<emulator::gameboy::CPU::Registers::H>(), 0x12);
    ASSERT_EQ(cpu.GetRegister<emulator::gameboy::CPU::Registers::L>(), 0x34);

    cpu.SetRegister<emulator::gameboy::CPU::Registers::H>(0xCA);
    ASSERT_EQ(cpu.GetRegister<emulator::gameboy::CPU::Registers::HL>(), 0xCA34);
    cpu.SetRegister<emulator::gameboy::CPU::Registers::L>(0xFE);
    ASSERT_EQ(cpu.GetRegister<emulator::gameboy::CPU::Registers::HL>(), 0xCAFE);
}