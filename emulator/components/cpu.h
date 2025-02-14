#pragma once

#include "component.h"
#include "system.h"

#include <functional>
#include <iostream>

namespace emulator::component
{

class CPU : public IComponent
{
protected:
    System* GetSystem() noexcept
    {
        if (bus_ == nullptr) {
            return nullptr;
        }
        return bus_->GetBoundSystem();
    }

public:
    CPU() : IComponent(IComponent::ComponentType::CPU) {}
};

}; // namespace emulator::component
