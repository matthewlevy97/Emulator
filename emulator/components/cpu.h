#pragma once

#include "component.h"

#include <functional>

namespace emulator::component {
    
class CPU : public IComponent {
public:
    using StepCallbackFunc = std::function<void(void)>;

protected:
    StepCallbackFunc onStepCallback_{nullptr};

public:
    void RegisterStepCallback(StepCallbackFunc func) noexcept
    {
        onStepCallback_ = func;
    }
};

}; // namespace emulator::component
