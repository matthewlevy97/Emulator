#pragma once

#include <vector>

#include "component.h"

namespace emulator::component {

class Bus : public IComponent {
private:
    std::vector<IComponent*> components_;

public:
    Bus();
    ~Bus() override;

    void AddComponent(IComponent* component);

    void ReceiveTick() override;

    void PowerOn() override;
    void PowerOff() override;
};

}; // namespace emulator::component
