#pragma once

#include <string>

#include "bus.h"

namespace emulator::component {

class System {
private:
    std::string name_;
    float tickRate_;
    Bus bus_;

    std::unordered_map<std::string, IComponent*> components_;

public:
    System(std::string name, float tickRate, std::unordered_map<std::string, IComponent*> components)
        : name_(name), tickRate_(tickRate), components_(components) {
        for (auto& [name, component] : components) {
            bus_.AddComponent(component);
        }
    }

    ~System() {}

    std::string Name() const
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

    Bus& GetBus()
    {
        return bus_;
    }

    IComponent* GetComponent(std::string name) const {
        auto it = components_.find(name);
        if (it == components_.end()) {
            return nullptr;
        }
        return it->second;
    }

    void Run()
    {
        while (true) {
            bus_.ReceiveTick();
        }
    }
};

using CreateSystemFunc = System* (*)();

}; // namespace emulator::component
