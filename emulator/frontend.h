#pragma once

#include <spdlog/spdlog.h>
#include <string>
#include <thread>

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
        spdlog::info("Stopping emulator: {}", system_->Name());
        systemStatus_ = emulator::component::SystemStatus::STOPPING;
        systemThread_.join();
    }

public:
    IFrontend(emulator::component::System* system) : system_(system)
    {
        if (system == nullptr) {
            throw std::invalid_argument("System cannot be null");
        }
    }

    virtual ~IFrontend() = default;

    virtual bool Initialize() noexcept = 0;
    virtual void Run() = 0;
    virtual void Shutdown() noexcept = 0;
};

}; // namespace emulator::frontend