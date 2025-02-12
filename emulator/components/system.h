#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <unordered_map>

#include <spdlog/spdlog.h>

#include <debugger/sysdebugger.h>

#include "bus.h"
#include "display.h"

namespace emulator::component
{

enum class SystemStatus {
    RUNNING,
    STOPPING,
    HALTED,
};

struct FrontendInterface {
    std::function<void()> RestartSystem;
    std::function<void(std::string)> Log;
};

using FrontendFunction = std::function<void(FrontendInterface&)>;

class System
{
private:
    std::string name_;
    std::uint64_t tickRate_;
    Bus bus_;

    std::unordered_map<std::string, IComponent*> components_;

    bool enableDebugging_{false};
    emulator::debugger::ISystemDebugger* debugger_;

    std::unordered_map<std::string, FrontendFunction> frontendFunctions_;

public:
    System(std::string name, std::uint64_t tickRate,
           std::unordered_map<std::string, IComponent*> components,
           emulator::debugger::ISystemDebugger* debugger = nullptr)
        : name_(name), tickRate_(tickRate), components_(components), debugger_(debugger)
    {
        bus_.BindSystem(this);

        for (auto& [name, component] : components) {
            bus_.AddComponent(component);
        }
    }

    ~System()
    {
    }

    std::string Name() const noexcept
    {
        return name_;
    }

    void PowerOn() noexcept
    {
        bus_.PowerOn();
    }

    void PowerOff() noexcept
    {
        bus_.PowerOff();
    }

    Bus& GetBus() noexcept
    {
        return bus_;
    }

    IComponent* GetComponent(std::string name) const noexcept
    {
        auto it = components_.find(name);
        if (it == components_.end()) {
            return nullptr;
        }
        return it->second;
    }

    template <typename T = IComponent>
    T* GetFirstComponentByType(IComponent::ComponentType type) const noexcept
    {
        for (auto& [_, component] : components_) {
            if (component->Type() == type) {
                return reinterpret_cast<T*>(component);
            }
        }
        return nullptr;
    }

    template <typename T = IComponent>
    std::vector<T*> GetComponentsByType(IComponent::ComponentType type) const noexcept
    {
        std::vector<T*> ret;
        for (auto& [_, component] : components_) {
            if (component->Type() == type) {
                ret.push_back(static_cast<T*>(component));
            }
        }
        return ret;
    }

    void Run(volatile SystemStatus& status)
    {
        std::chrono::milliseconds interval(1000 / tickRate_);
        while (status == SystemStatus::RUNNING) {
            if (enableDebugging_ && debugger_ != nullptr && debugger_->IsStopped()) {
                continue;
            }

            auto start = std::chrono::high_resolution_clock::now();

            // Fails if powered off
            if (!bus_.ReceiveTick()) {
                break;
            }

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::milliseconds elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            if (elapsed <= interval) {
                std::this_thread::sleep_for(interval - elapsed);
            } else {
                elapsed -= interval;
                spdlog::trace("Tick took {} ms too long", elapsed.count());
            }
        }
        status = SystemStatus::HALTED;
    }

    void Run()
    {
        SystemStatus status = SystemStatus::RUNNING;
        Run(status);
    }

    void UseDebugger(bool enabled = true) noexcept
    {
        enableDebugging_ = enabled;
    }

    bool DebuggerEnabled() const noexcept
    {
        return enableDebugging_;
    }

    emulator::debugger::ISystemDebugger* GetDebugger() const noexcept
    {
        return debugger_;
    }

    void LogStacktrace() noexcept
    {
        bus_.LogStacktrace();
    }

    void RegisterFrontendFunction(std::string name, FrontendFunction func) noexcept
    {
        frontendFunctions_[name] = func;
    }

    std::unordered_map<std::string, FrontendFunction> GetFrontendFunctions() const noexcept
    {
        return frontendFunctions_;
    }
};

using CreateSystemFunc = System* (*)();

}; // namespace emulator::component
