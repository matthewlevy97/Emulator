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
    std::function<std::string()> OpenFileDialog;
    std::function<void(std::function<void()>)> RestartSystem;
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
        static constexpr int kTickRecalculateInterval = 1000;

        auto tickCounter = kTickRecalculateInterval;
        auto interval = std::chrono::nanoseconds(1000000000 / tickRate_).count();

        // Purposefully not in nanoseconds to prevent average at a 0 delay
        auto sleepTime = std::chrono::microseconds(0);

        auto start = std::chrono::high_resolution_clock::now();
        while (status == SystemStatus::RUNNING) {
            if (enableDebugging_ && debugger_ != nullptr && debugger_->IsStopped()) {
                continue;
            }

            if (tickCounter >= kTickRecalculateInterval) {
                start = std::chrono::high_resolution_clock::now();
            }

            // Fails if powered off
            if (!bus_.ReceiveTick()) {
                break;
            }

            // Average the tick rate to minimize spin calls
            // Higher values for kTickRecalculateInterval can result in longer stutters
            if (--tickCounter == 0) {
                tickCounter = kTickRecalculateInterval;
                auto elapsedAverage = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                          std::chrono::high_resolution_clock::now() - start)
                                          .count() /
                                      kTickRecalculateInterval;
                if (elapsedAverage <= interval) {
                    // We are running too fast, slow down
                    sleepTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::nanoseconds(interval - elapsedAverage));
                } else {
                    // We are running too slow, speed up
                    sleepTime = std::chrono::microseconds(0);
                }
            }

            std::this_thread::sleep_for(sleepTime);
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
