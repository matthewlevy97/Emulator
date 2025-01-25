#include <gtest/gtest.h>

#include "multimappedmemory.h"

TEST(ComponentMultiMappedMemory, ValidAddress)
{
    auto ram = emulator::component::MultiMappedMemory<emulator::component::MemoryType::ReadWrite>(1024);

    ASSERT_NO_THROW(ram.ReadUInt8(0));
    ASSERT_NO_THROW(ram.ReadUInt8(1023));
}

TEST(ComponentMultiMappedMemory, InvalidAddress)
{
    auto ram = emulator::component::MultiMappedMemory<emulator::component::MemoryType::ReadWrite>(
        {{0x100, 0x100 + 1024}},
        1024
    );

    // Under base address
    ASSERT_THROW(ram.ReadUInt8(0), emulator::component::InvalidAddress);
    ASSERT_THROW(ram.WriteUInt8(0, 0), emulator::component::InvalidAddress);

    // Over end address
    ASSERT_THROW(ram.ReadUInt8(0x100 + 1024 + 32), emulator::component::InvalidAddress);
    ASSERT_THROW(ram.WriteUInt8(0x100 + 1024 + 32, 0), emulator::component::InvalidAddress);

    // Overlap end of memory
    ASSERT_THROW(ram.ReadUInt32(0x100 + 1024 - 1), emulator::component::InvalidAddress);
    ASSERT_THROW(ram.WriteUInt32(0x100 + 1024 - 1, 0), emulator::component::InvalidAddress);
}

TEST(ComponentMultiMappedMemory, ReadWriteToMultiMapped)
{
    static const size_t address = 0x50;

    auto ram = emulator::component::MultiMappedMemory<emulator::component::MemoryType::ReadWrite>(
        {
            {0x00000, 0x1000},
            {0x10000, 0x11000}
        },
        0x1000
    );

    ram.WriteUInt8(address, 0x12);
    ASSERT_EQ(ram.ReadUInt8(address), 0x12);
    ASSERT_EQ(ram.ReadUInt8(0x10000 + address), 0x12);

    ram.WriteUInt8(0x10000 + address, 0x34);
    ASSERT_EQ(ram.ReadUInt8(address), 0x34);
    ASSERT_EQ(ram.ReadUInt8(0x10000 + address), 0x34);
}
