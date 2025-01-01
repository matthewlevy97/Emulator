#include <gtest/gtest.h>

#include <emulator.h>

TEST(GameBoySystem, CreateSystem)
{
    auto system = CreateSystem();
    ASSERT_NE(system, nullptr);
}

// Test getting GameBoy name
TEST(GameBoySystem, GetSystemName)
{
    auto system = CreateSystem();
    ASSERT_STRCASEEQ(system->Name().c_str(), "GameBoy");
}

// Test reading and writing to 8KiB internal ram
TEST(GameBoySystem, ReadWriteInternal8KiBRAM)
{
    auto system = CreateSystem();
    auto bus = system->GetBus();

    // Write and read 8-bit values
    bus.Write<std::uint8_t>(0xE000 - sizeof(std::uint8_t) - 1, 0xCA);
    ASSERT_EQ(bus.Read<std::uint8_t>(0xE000 - sizeof(std::uint8_t) - 1), (std::uint8_t)0xCA);

    bus.Write<std::uint8_t>(0xC000, 0xDE);
    ASSERT_EQ(bus.Read<std::uint8_t>(0xC000), 0xDE);

    bus.Write<std::int8_t>(0xC000, 0xCA);
    ASSERT_EQ(bus.Read<std::int8_t>(0xC000), (std::int8_t)0xCA);


    // Write and read 16-bit values
    bus.Write<std::int16_t>(0xE000 - sizeof(std::int16_t) - 1, 0xCAFE);
    ASSERT_EQ(bus.Read<std::int16_t>(0xE000 - sizeof(std::int16_t) - 1), (std::int16_t)0xCAFE);

    bus.Write<std::uint16_t>(0xC000, 0xDEAD);
    ASSERT_EQ(bus.Read<std::uint16_t>(0xC000), 0xDEAD);

    bus.Write<std::int16_t>(0xC000, 0xCAFE);
    ASSERT_EQ(bus.Read<std::int16_t>(0xC000), (std::int16_t)0xCAFE);


    // Write and read 32-bit values
    bus.Write<std::int32_t>(0xE000 - sizeof(std::int32_t) - 1, 0xCAFEBABE);
    ASSERT_EQ(bus.Read<std::int32_t>(0xE000 - sizeof(std::int32_t) - 1), (std::int32_t)0xCAFEBABE);

    bus.Write<std::uint32_t>(0xC000, 0xDEADBEEF);
    ASSERT_EQ(bus.Read<std::uint32_t>(0xC000), 0xDEADBEEF);

    bus.Write<std::int32_t>(0xC000, 0xCAFEBABE);
    ASSERT_EQ(bus.Read<std::int32_t>(0xC000), (std::int32_t)0xCAFEBABE);
}

// Test reading and writing to echo internal ram
TEST(GameBoySystem, ReadWriteInternalEcho8KiBRAM)
{
    auto system = CreateSystem();
    auto bus = system->GetBus();

    constexpr std::size_t offset = 32;

    bus.Write<std::uint8_t>(0xC000 + offset, 0xDE);
    ASSERT_EQ(bus.Read<std::uint8_t>(0xC000 + offset), 0xDE);
    ASSERT_EQ(bus.Read<std::uint8_t>(0xE000 + offset), 0xDE);

    bus.Write<std::uint8_t>(0xE000 + offset, 0xCA);
    ASSERT_EQ(bus.Read<std::uint8_t>(0xC000 + offset), 0xCA);
    ASSERT_EQ(bus.Read<std::uint8_t>(0xE000 + offset), 0xCA);
}

// Test reading from upper internal ram
TEST(GameBoySystem, ReadWriteUpperInternalRAM)
{
    auto system = CreateSystem();
    auto bus = system->GetBus();
    // Write and read 8-bit values
    bus.Write<std::uint8_t>(0xFF80, 0xDE);
    ASSERT_EQ(bus.Read<std::uint8_t>(0xFF80), 0xDE);
    
    bus.Write<std::int8_t>(0xFF80, 0xCA);
    ASSERT_EQ(bus.Read<std::int8_t>(0xFF80), (std::int8_t)0xCA);
    
    bus.Write<std::uint8_t>(0xFFFF - sizeof(std::uint8_t), 0xBE);
    ASSERT_EQ(bus.Read<std::uint8_t>(0xFFFF - sizeof(std::uint8_t)), 0xBE);

    bus.Write<std::int8_t>(0xFFFF - sizeof(std::int8_t), 0xBA);
    ASSERT_EQ(bus.Read<std::int8_t>(0xFFFF - sizeof(std::int8_t)), (std::int8_t)0xBA);


    // Write and read 16-bit values
    bus.Write<std::uint16_t>(0xFF80, 0xDEAD);
    ASSERT_EQ(bus.Read<std::uint16_t>(0xFF80), 0xDEAD);

    bus.Write<std::int16_t>(0xFF80, 0xCAFE);
    ASSERT_EQ(bus.Read<std::int16_t>(0xFF80), (std::int16_t)0xCAFE);

    bus.Write<std::uint16_t>(0xFFFF - sizeof(std::uint16_t), 0xBEEF);
    ASSERT_EQ(bus.Read<std::uint16_t>(0xFFFF - sizeof(std::uint16_t)), 0xBEEF);

    bus.Write<std::int16_t>(0xFFFF - sizeof(std::int16_t), 0xBABE);
    ASSERT_EQ(bus.Read<std::int16_t>(0xFFFF - sizeof(std::int16_t)), (std::int16_t)0xBABE);


    // Write and read 32-bit values
    bus.Write<std::uint32_t>(0xFF80, 0xDEADBEEF);
    ASSERT_EQ(bus.Read<std::uint32_t>(0xFF80), 0xDEADBEEF);

    bus.Write<std::int32_t>(0xFF80, 0xCAFEBABE);
    ASSERT_EQ(bus.Read<std::int32_t>(0xFF80), (std::int32_t)0xCAFEBABE);

    bus.Write<std::uint32_t>(0xFFFF - sizeof(std::uint32_t), 0xFEEDFACE);
    ASSERT_EQ(bus.Read<std::uint32_t>(0xFFFF - sizeof(std::uint32_t)), 0xFEEDFACE);

    bus.Write<std::int32_t>(0xFFFF - sizeof(std::int32_t), 0xDEADC0DE);
    ASSERT_EQ(bus.Read<std::int32_t>(0xFFFF - sizeof(std::int32_t)), (std::int32_t)0xDEADC0DE);
}

// Test writing to upper internal ram