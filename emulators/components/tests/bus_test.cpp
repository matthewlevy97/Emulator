#include <gtest/gtest.h>

#include "bus.h"
#include "memory.h"
#include "exceptions/AddressInUse.h"

// Test adding RAM to bus
TEST(ComponentBUS, AddRAM)
{
    auto bus = emulator::component::Bus();
    auto ram = new emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(1024);

    ASSERT_NO_THROW(bus.AddComponent(ram));
}

// Test adding multiple RAM to bus
TEST(ComponentBUS, AddMultipleRAM)
{
    auto bus = emulator::component::Bus();
    auto ram = new emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(1024);
    auto ram2 = new emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(2048, 1024);

    ASSERT_NO_THROW(bus.AddComponent(ram));
    ASSERT_NO_THROW(bus.AddComponent(ram2));
}

// Test adding conflicting RAM addresses to bus
TEST(ComponentBUS, AddConflictingRAMAddressese)
{
    auto bus = emulator::component::Bus();
    auto ram = new emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(1024);
    auto ram2 = new emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(1024);

    ASSERT_NO_THROW(bus.AddComponent(ram));
    ASSERT_THROW(bus.AddComponent(ram2), emulator::component::AddressInUse);
}

// Test addressing into component on bus
TEST(ComponentBUS, AddressComponent)
{
    auto bus = emulator::component::Bus();
    auto ram = new emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(1024);

    ASSERT_NO_THROW(bus.AddComponent(ram));

    auto value = bus.Read<std::uint8_t>(0x7F);
    ASSERT_EQ(value, 0);
}

// Test addressing into invalid memory on bus
TEST(ComponentBUS, InvalidAddressComponent)
{
    auto bus = emulator::component::Bus();
    auto ram = new emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(1024);

    ASSERT_NO_THROW(bus.AddComponent(ram));

    ASSERT_THROW(bus.Read<std::uint8_t>(2048), std::out_of_range);
    ASSERT_THROW(bus.Write<std::uint16_t>(2048, 0xCAFE), std::out_of_range);
}

// Test removing component from bus
TEST(ComponentBUS, RemoveComponent)
{
    auto bus = emulator::component::Bus();
    auto ram = new emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(1024);

    ASSERT_NO_THROW(bus.AddComponent(ram));
    bus.RemoveComponent(ram);
}

// Test addressing to removed component on bus
TEST(ComponentBUS, RemoveComponentAccessAddress)
{
    auto bus = emulator::component::Bus();
    auto ram = new emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(1024);

    ASSERT_NO_THROW(bus.AddComponent(ram));

    ASSERT_EQ(bus.Read<std::uint8_t>(512), 0);

    bus.RemoveComponent(ram);

    ASSERT_THROW(bus.Read<std::uint8_t>(512), std::out_of_range);
    ASSERT_THROW(bus.Write<std::uint16_t>(0x50, 0xCAFE), std::out_of_range);
}

// Test reading from RAM each primitive type over bus
TEST(ComponentBUS, ReadPrimitiveTypes)
{
    auto bus = emulator::component::Bus();
    auto ram = new emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(1024);

    ASSERT_NO_THROW(bus.AddComponent(ram));

    ASSERT_EQ(bus.Read<std::uint8_t>(0x50), 0);
    ASSERT_EQ(bus.Read<std::int8_t>(0x50), 0);
    ASSERT_EQ(bus.Read<std::uint16_t>(0x50), 0);
    ASSERT_EQ(bus.Read<std::int16_t>(0x50), 0);
    ASSERT_EQ(bus.Read<std::uint32_t>(0x50), 0);
    ASSERT_EQ(bus.Read<std::int32_t>(0x50), 0);
}

// Test writing to RAM each primitive type over bus
TEST(ComponentBUS, WritePrimitiveTypes)
{
    auto bus = emulator::component::Bus();
    auto ram = new emulator::component::Memory<emulator::component::MemoryType::ReadWrite>(1024);

    ASSERT_NO_THROW(bus.AddComponent(ram));

    bus.Write<std::uint8_t>(0x50, 0x12);
    ASSERT_EQ(bus.Read<std::uint8_t>(0x50), 0x12);

    bus.Write<std::int8_t>(0x50, 0x34);
    ASSERT_EQ(bus.Read<std::int8_t>(0x50), 0x34);

    bus.Write<std::uint16_t>(0x50, 0x1234);
    ASSERT_EQ(bus.Read<std::uint16_t>(0x50), 0x1234);

    bus.Write<std::int16_t>(0x50, 0x5678);
    ASSERT_EQ(bus.Read<std::int16_t>(0x50), 0x5678);

    bus.Write<std::uint32_t>(0x50, 0x12345678);
    ASSERT_EQ(bus.Read<std::uint32_t>(0x50), 0x12345678);

    bus.Write<std::int32_t>(0x50, 0x87654321);
    ASSERT_EQ(bus.Read<std::int32_t>(0x50), 0x87654321);
}
