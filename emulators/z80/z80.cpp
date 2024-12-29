#pragma once

#include "emulator.h"

emulator::component::System* CreateSystem() {
    return new emulator::component::System("Z80", 4.0f, {});
}
