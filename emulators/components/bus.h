#pragma once

#include <vector>

#include "component.h"

namespace emulator::component {

class Bus {
private:
    float tickRate_;
    std::vector<IComponent*> components_;

public:
    Bus();
    ~Bus();

    void AddComponent(IComponent* component);

    void SetTickRate(float);
    float GetTickRate() const;

    void PowerOn();
    void PowerOff();
};

}; // namespace emulator::component
