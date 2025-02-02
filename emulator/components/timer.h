#pragma once

#include "component.h"

#include <functional>

namespace emulator::component
{

class Timer : public IComponent
{
public:
    using TriggerCallbackFunc = std::function<void(void)>;

protected:
    std::string name_;

    std::uint32_t tickCtr_{0};
    std::uint32_t tickSampling_;

    std::uint32_t counter_{0};
    std::uint32_t resetValue_{0};
    TriggerCallbackFunc onCompleteCallback_{nullptr};

public:
    Timer(std::string name) : Timer(name, 1, 0) {}
    Timer(std::string name, std::uint32_t tickSampling) : Timer(name, tickSampling, 0) {}
    Timer(std::string name, std::uint32_t tickSampling, std::uint32_t resetValue)
        : IComponent(IComponent::ComponentType::Timer),
          name_(name), tickSampling_(tickSampling), resetValue_(resetValue) {}

    void RegisterCompletionCallback(TriggerCallbackFunc func) noexcept
    {
        onCompleteCallback_ = func;
    }

    void ReceiveTick() override
    {
        // Sample the tick counter to enable variable tick rates on system
        if (tickCtr_ == 0) {
            tickCtr_ = tickSampling_;
        } else {
            tickCtr_--;
            return;
        }

        if (counter_ == 0) {
            if (onCompleteCallback_) {
                onCompleteCallback_();
            }
        } else {
            counter_--;
        }
    }

    void PowerOn() noexcept override
    {
        counter_ = resetValue_;
    }

    void PowerOff() noexcept override
    {
    }

    void Reset() noexcept
    {
        counter_ = resetValue_;
    }

    void SetCounter(std::uint32_t value) noexcept
    {
        counter_ = value;
    }

    std::uint32_t GetCounter() const noexcept
    {
        return counter_;
    }

    std::string GetName() const noexcept
    {
        return name_;
    }
};

}; // namespace emulator::component
