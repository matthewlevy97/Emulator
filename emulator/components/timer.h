#pragma once

#include "component.h"

#include <functional>

namespace emulator::component {
    
class Timer : public IComponent {
public:
    using TriggerCallbackFunc = std::function<void(void)>;

protected:
    std::uint32_t counter_{0};
    std::uint32_t startValue_{0};
    TriggerCallbackFunc onCompleteCallback_{nullptr};

public:
    Timer(std::uint32_t startValue) : IComponent(IComponent::ComponentType::Timer), startValue_(startValue) {}

    void RegisterCompletionCallback(TriggerCallbackFunc func) noexcept
    {
        onCompleteCallback_ = func;
    }

    void ReceiveTick() override
    {
        if (counter_ == 0) {
            if (onCompleteCallback_) {
                onCompleteCallback_();
                counter_ = startValue_;
            }
        } else {
            counter_--;
        }
    }

    void PowerOn() override
    {
        counter_ = startValue_;
    }

    void PowerOff() override
    {}
};

}; // namespace emulator::component
