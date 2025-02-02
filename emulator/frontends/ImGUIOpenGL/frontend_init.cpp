#include "frontend.h"

namespace emulator::frontend
{
    IFrontend* CreateFrontend(emulator::component::System* system)
    {
        return new emulator::frontend::imgui_opengl::ImGuiFrontend(system);
    }
};