#pragma once

#include <spdlog/spdlog.h>
#include <string>
#include <thread>
#include <vector>

#include "emumanager.h"

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
    emulator::core::EmulatorManager* manager_;

    emulator::component::SystemStatus systemStatus_{emulator::component::SystemStatus::HALTED};
    emulator::component::System* system_{nullptr};
    std::thread systemThread_;

    emulator::component::FrontendInterface frontendInterface_ = {
        .OpenFileDialog = []() -> std::string {
            spdlog::error("OpenFileDialog not implemented");
            return "";
        },

        .RestartSystem = [this](std::function<void()> doDuringOff = nullptr) {
            StopSystem();
            if (doDuringOff)
                doDuringOff();
            RunSystem();
        },

        .Log = [](std::string message) {
            spdlog::debug("Frontend: {}", message);
        },
    };

    emulator::component::Display* display_{nullptr};
    std::vector<emulator::component::Input*> inputs_;

    std::size_t width_{1280}, height_{720};

protected:
    void RunSystem()
    {
        if (system_ == nullptr) {
            return;
        }

        if (systemStatus_ == emulator::component::SystemStatus::RUNNING) {
            return;
        }

        systemThread_ = std::thread([this]() {
            try {
                system_->PowerOn();
                systemStatus_ = emulator::component::SystemStatus::RUNNING;
                system_->Run(systemStatus_);
                system_->PowerOff();
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

        // Probably not needed, but rather ensure its set by this point
        systemStatus_ = emulator::component::SystemStatus::HALTED;
    }

    void LoadSystem(const std::string& name, bool enableDebugger = false)
    {
        auto system = GetSystem(name, enableDebugger);
        if (system == nullptr) {
            return;
        }

        LoadSystem(system);
    }

    void LoadSystem(emulator::component::System* system, bool enableDebugger = false)
    {
        if (system_ != nullptr) {
            // Have to handle fact that system may be running
            StopSystem();
            delete system_;
        }

        system_ = system;
        if (system_ == nullptr) {
            return;
        }

        if (!LoadDisplay()) {
            throw std::runtime_error("Failed to load display component");
        }
        LoadInputs();
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

private:
    emulator::component::System* GetSystem(const std::string& name,
                                           bool enableDebugger) const noexcept
    {
        auto createSystem = manager_->GetSystem(name);
        if (!createSystem) {
            spdlog::error("Failed to get system handle for {}", name);
            return nullptr;
        }

        emulator::component::System* system = nullptr;
        try {
            system = createSystem();
        } catch (std::exception& e) {
            spdlog::error("Failed to create system: {}", e.what());
            return nullptr;
        }

        auto sysDebugger = system->GetDebugger();
        if (sysDebugger != nullptr) {
            auto& debugger = manager_->GetDebugger();
            debugger.RegisterDebugger(sysDebugger);
            debugger.SelectDebugger(sysDebugger->GetName());
            system->UseDebugger(enableDebugger);
            spdlog::info("Registed debugger for {}", sysDebugger->GetName());
        }

        return system;
    }

public:
    IFrontend(emulator::core::EmulatorManager* manager) : manager_(manager)
    {
        if (manager_ == nullptr) {
            throw std::invalid_argument("Manager cannot be null");
        }
    }

    virtual ~IFrontend() = default;

    virtual bool Initialize() noexcept = 0;
    virtual void Run() = 0;
    virtual void Shutdown() noexcept = 0;

    virtual void ScaleSystemDisplay(std::size_t scale) noexcept = 0;
};

IFrontend* CreateFrontend(emulator::core::EmulatorManager* system);

}; // namespace emulator::frontend
