#include <gtest/gtest.h>

#include "memory.h"

TEST(ComponentMemory, ValidAddress)
{
    auto ram = emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(1024);

    ASSERT_NO_THROW(ram.ReadUInt8(0));
    ASSERT_NO_THROW(ram.ReadUInt8(1023));
}

TEST(ComponentMemory, InvalidAddress)
{
    auto ram = emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(0x100, 1024);

    // Under base address
    ASSERT_THROW(ram.ReadUInt8(0), std::out_of_range);
    ASSERT_THROW(ram.WriteUInt8(0, 0), std::out_of_range);

    // Over end address
    ASSERT_THROW(ram.ReadUInt8(0x100 + 1024 + 32), std::out_of_range);
    ASSERT_THROW(ram.WriteUInt8(0x100 + 1024 + 32, 0), std::out_of_range);

    // Overlap end of memory
    ASSERT_THROW(ram.ReadUInt32(0x100 + 1024 - 1), std::out_of_range);
    ASSERT_THROW(ram.WriteUInt32(0x100 + 1024 - 1, 0), std::out_of_range);
}

TEST(ComponentMemory, ReadWrite)
{
    static const size_t address = 0x50;

    auto ram = emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(1024);

    ram.WriteUInt8(address, 0x12);
    ASSERT_EQ(ram.ReadUInt8(address), 0x12);

    ram.WriteUInt8(address, 0x34);
    ASSERT_EQ(ram.ReadUInt8(address), 0x34);
}

TEST(ComponentMemory, ClearToZero)
{
    auto ram = emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(32);

    ram.WriteUInt8(0x0, 0x12);
    ASSERT_EQ(ram.ReadUInt8(0x0), 0x12);

    ram.Clear();

    ASSERT_EQ(ram.ReadUInt8(0x0), 0);
}

TEST(ComponentMemory, AccessReadOnlyMemory)
{
    auto rom = emulator::component::Memory<emulator::component::MemoryType::ReadOnly>(32);

    ASSERT_EQ(rom.ReadUInt8(0x0), 0);
    ASSERT_THROW(rom.WriteUInt8(0x0, 0x12), std::runtime_error);
}