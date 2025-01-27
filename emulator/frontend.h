#pragma once

#include <spdlog/spdlog.h>
#include <string>

#include <components/system.h>
#include <core/emumanager.h>
#include <debugger/debugger.h>

namespace emulator::frontend
{

class IFrontend
{
protected:
    emulator::core::EmulatorManager* manager_;

public:
    IFrontend(emulator::core::EmulatorManager* manager) : manager_(manager) {}

    virtual ~IFrontend() = default;

    virtual bool Initialize() noexcept = 0;
    virtual void Run() = 0;
    virtual void Shutdown() noexcept = 0;

    emulator::component::System* GetSystem(std::string name, bool enableDebugger = false) noexcept
    {
        auto createSystem = manager_->GetSystem(name);
        if (!createSystem) {
            spdlog::error("Failed to get system handle for {}", name);
            return nullptr;
        }
        auto system = createSystem();

        system->PowerOn();

        auto sysDebugger = system->GetDebugger();
        if (enableDebugger && sysDebugger != nullptr) {
            auto& debugger = manager_->GetDebugger();
            debugger.RegisterDebugger(sysDebugger);
            debugger.SelectDebugger(sysDebugger->GetName());
            system->UseDebugger(true);
            spdlog::info("Registed {} for remote debugging", sysDebugger->GetName());
        }

        return system;
    }
};

}; // namespace emulator::frontend