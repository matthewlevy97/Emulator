#pragma once

#include <spdlog/spdlog.h>
#include <string>
#include <thread>
#include <vector>

#include <components/display.h>
#include <components/input.h>
#include <components/system.h>

#include <debugger/debugger.h>

/**
 * TODO: Get Keyboard Input
 * TODO: Relay Keyboard Input to all Input components
 * TODO: Get the Display component and draw to screen (Likely OpenGL texture that gets drawn)
 */

namespace emulator::frontend
{

class IFrontend
{
public:
    static constexpr const char* kWindowTitle = "Emulator";

protected:
    emulator::component::SystemStatus systemStatus_{emulator::component::SystemStatus::RUNNING};
    emulator::component::System* system_;
    std::thread systemThread_;

    emulator::component::Display* display_{nullptr};
    std::vector<emulator::component::Input*> inputs_;

    std::size_t width_{1280}, height_{720};

protected:
    void RunSystem()
    {
        systemThread_ = std::thread([this]() {
            try {
                system_->Run(systemStatus_);
                spdlog::info("Emulator {} exited", system_->Name());
            } catch (const std::exception& e) {
                spdlog::error("Emulator {} exited with exception: {}", system_->Name(), e.what());
                system_->LogStacktrace();
            }

            // In-case not already set when here (e.g., Exception throw)
            systemStatus_ = emulator::component::SystemStatus::HALTED;
        });
        spdlog::info("Starting emulator: {}", system_->Name());
    }

    void StopSystem()
    {
        systemStatus_ = emulator::component::SystemStatus::STOPPING;
        if (systemThread_.joinable()) {
            systemThread_.join();
        }
    }

    bool LoadDisplay() noexcept
    {
        auto displays = system_->GetComponentsByType(emulator::component::IComponent::ComponentType::Display);
        if (displays.size() != 1) {
            spdlog::error("Expected 1 display component, found {}", displays.size());
            return false;
        }
        display_ = dynamic_cast<emulator::component::Display*>(displays[0]);
        return true;
    }

    void LoadInputs() noexcept
    {
        auto inputs = system_->GetComponentsByType(emulator::component::IComponent::ComponentType::Input);
        std::transform(inputs.begin(), inputs.end(), std::back_inserter(inputs_),
                       [](emulator::component::IComponent* component) {
                           return dynamic_cast<emulator::component::Input*>(component);
                       });
    }

public:
    IFrontend(emulator::component::System* system) : system_(system)
    {
        if (system == nullptr) {
            throw std::invalid_argument("System cannot be null");
        }
        if (!LoadDisplay()) {
            throw std::runtime_error("Failed to load display component");
        }
        LoadInputs();
    }

    virtual ~IFrontend() = default;

    virtual bool Initialize() noexcept = 0;
    virtual void Run() = 0;
    virtual void Shutdown() noexcept = 0;

    virtual void ScaleSystemDisplay(std::size_t scale) noexcept = 0;
};

}; // namespace emulator::frontend
