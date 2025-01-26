#pragma once

#include "component.h"

#include <functional>
#include <iostream>

namespace emulator::component {
    
class CPU : public IComponent {
public:
    using StepCallbackFunc = std::function<void(void)>;

protected:
    StepCallbackFunc onStepCallback_{nullptr};

public:
    CPU() : IComponent(IComponent::ComponentType::CPU) {}

    void RegisterStepCallback(StepCallbackFunc func) noexcept
    {
        onStepCallback_ = func;
    }
};

}; // namespace emulator::component
