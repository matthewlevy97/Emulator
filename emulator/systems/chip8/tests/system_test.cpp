#include <gtest/gtest.h>

#include <emulator.h>

TEST(Chip8System, CreateSystem)
{
    auto system = CreateSystem();
    ASSERT_NE(system, nullptr);
}

// Test getting Chip8 name
TEST(Chip8System, GetSystemName)
{
    auto system = CreateSystem();
    ASSERT_STRCASEEQ(system->Name().c_str(), "Chip8");
}
