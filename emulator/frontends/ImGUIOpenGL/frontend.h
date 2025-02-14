#pragma once

#include <frontend.h>

namespace emulator::frontend::imgui_opengl
{

class ImGuiFrontend : public emulator::frontend::IFrontend
{
private:
    void *window_;
    std::size_t menuBarHeight_{0};

    std::uint64_t targetFPS_{60};

public:
    ImGuiFrontend(emulator::core::EmulatorManager* manager);

    bool Initialize() noexcept override;
    void Run() override;
    void Shutdown() noexcept override;

    void ScaleSystemDisplay(std::size_t scale) noexcept override;
};

}; // namespace emulator::frontend
