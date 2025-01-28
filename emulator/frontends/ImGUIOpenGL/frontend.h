#pragma once

#include <frontend.h>

namespace emulator::frontend::imgui_opengl
{

class ImGuiFrontend : public emulator::frontend::IFrontend
{
private:
    void *window_;

public:
    ImGuiFrontend(emulator::component::System* system) : emulator::frontend::IFrontend(system) {}

    bool Initialize() noexcept override;
    void Run() override;
    void Shutdown() noexcept override;
};

}; // namespace emulator::frontend