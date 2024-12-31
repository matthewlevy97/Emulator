#pragma once

namespace emulator::component {

class IComponent {
public:
    IComponent() {}
    virtual inline ~IComponent() = default;

    virtual void ReceiveTick() = 0;

    virtual void PowerOn() = 0;
    virtual void PowerOff() = 0;
};

}; // namespace emulator::component
