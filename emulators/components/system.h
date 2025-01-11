#pragma once

#include <string>

#include <debugger/sysdebugger.h>

#include "bus.h"

namespace emulator::component {

class System {
private:
    std::string name_;
    float tickRate_;
    Bus bus_;

    std::unordered_map<std::string, IComponent*> components_;

    bool enableDebugging_{false};
    emulator::debugger::ISystemDebugger* debugger_;

public:
    System(std::string name, float tickRate,
        std::unordered_map<std::string, IComponent*> components,
        emulator::debugger::ISystemDebugger* debugger = nullptr)
        : name_(name), tickRate_(tickRate), components_(components), debugger_(debugger) {
        for (auto& [name, component] : components) {
            bus_.AddComponent(component);
        }
    }

    ~System()
    {}

    std::string Name() const noexcept
    {
        return name_;
    }

    void PowerOn()
    {
        bus_.PowerOn();
    }

    void PowerOff()
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

    void Run()
    {
        while (true) {
            if (enableDebugging_ && debugger_ != nullptr && debugger_->IsStopped()) {
                continue;
            }
            bus_.ReceiveTick();
        }
    }

    void UseDebugger(bool enabled = true) noexcept
    {
        enableDebugging_ = enabled;
    }

    emulator::debugger::ISystemDebugger* GetDebugger() const noexcept
    {
        return debugger_;
    }
};

using CreateSystemFunc = System* (*)();

}; // namespace emulator::component
