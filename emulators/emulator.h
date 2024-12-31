#pragma once

#include "components/system.h"

#if defined(_WIN32) || defined(_WIN64)
    #define EMULATOR_EXPORT __declspec(dllexport)
#else
    #define EMULATOR_EXPORT
#endif

extern "C" {
    emulator::component::System* CreateSystem();
};

#undef EMULATOR_EXPORT
