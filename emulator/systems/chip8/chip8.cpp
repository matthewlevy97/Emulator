#include <emulator.h>
#include <components/memory.h>
#include <components/multimappedmemory.h>

emulator::component::System* CreateSystem() {
    auto system = new emulator::component::System("Chip8", 1.0f, {});
    return system;
}