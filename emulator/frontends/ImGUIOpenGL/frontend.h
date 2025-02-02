#pragma once

#include <frontend.h>

namespace emulator::frontend::imgui_opengl
{

class ImGuiFrontend : public emulator::frontend::IFrontend
{
private:
    void *window_;
    std::size_t menuBarHeight_{0};

public:
    ImGuiFrontend(emulator::component::System* system) : emulator::frontend::IFrontend(system) {}

    bool Initialize() noexcept override;
    void Run() override;
    void Shutdown() noexcept override;

    void ScaleSystemDisplay(std::size_t scale) noexcept override;
};

}; // namespace emulator::frontend
