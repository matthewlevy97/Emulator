#include "gtest/gtest.h"

#include <emulator.h>

TEST(GameBoySystem, CreateSystem)
{
    auto system = CreateSystem();
    ASSERT_NE(system, nullptr);
}

TEST(GameBoySystem, GetSystemName)
{
    auto system = CreateSystem();
    ASSERT_STRCASEEQ(system->Name().c_str(), "GameBoy");
}

TEST(GameBoySystem, PowerOn)
{
    auto system = CreateSystem();
    system->PowerOn();
}

TEST(GameBoySystem, PowerOff)
{
    auto system = CreateSystem();
    system->PowerOff();
}