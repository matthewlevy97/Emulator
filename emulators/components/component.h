#pragma once

namespace emulator::component {

class IComponent {
public:
    IComponent() {}
    virtual inline ~IComponent() {};

    virtual void ReceiveTick();

    virtual void PowerOn() {};
    virtual void PowerOff() {};
};

}; // namespace emulator::component
