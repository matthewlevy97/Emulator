#pragma once

#include <string>

#include "bus.h"

namespace emulator::component {

class System {
private:
    std::string name_;
    float tickRate_;
    Bus bus_;

public:
    System(std::string name, float tickRate, std::initializer_list<IComponent*> components)
        : name_(name), tickRate_(tickRate) {
        for (auto component : components) {
            bus_.AddComponent(component);
        }
    }

    ~System() {}

    std::string Name() const {
        return name_;
    }

    void PowerOn() {
        bus_.PowerOn();
    }

    void PowerOff() {
        bus_.PowerOff();
    }

    void Run() {
        while (true) {
            bus_.ReceiveTick();
        }
    }
};

using CreateSystemFunc = System* (*)();

}; // namespace emulator::component
