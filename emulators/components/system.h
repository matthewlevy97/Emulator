#pragma once

#include <string>

#include "bus.h"

namespace emulator::component {

class System {
private:
    std::string name_;
    Bus bus_;

public:

    System(std::string name, float tickRate, std::initializer_list<IComponent*> components) : name_(name) {
        bus_.SetTickRate(tickRate);

        for (auto component : components) {
            bus_.AddComponent(component);
        }
    }

    ~System() {}

    void PowerOn() {
        bus_.PowerOn();
    }

    void PowerOff() {
        bus_.PowerOff();
    }
};

}; // namespace emulator::component
