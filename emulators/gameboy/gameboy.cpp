#include <emulator.h>

#include "cpu.h"

emulator::component::System* CreateSystem() {
    return new emulator::component::System("GameBoy", 4.0f, {
        new emulator::gameboy::CPU()
    });
}