#include "frontend.h"

namespace emulator::frontend
{
    IFrontend* CreateFrontend(emulator::core::EmulatorManager* manager)
    {
        return new emulator::frontend::imgui_opengl::ImGuiFrontend(manager);
    }
};
