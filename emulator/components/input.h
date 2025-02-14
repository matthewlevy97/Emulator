#pragma once
#include <functional>
#include <spdlog/spdlog.h>
#include <unordered_map>

#include "component.h"

namespace emulator::component
{

struct InputKeyHandler {
    std::function<void()> onPress, onRelease;
    bool isPressed{false};
};

class Input : public IComponent
{
public:
    using InputKeyCode = int;

protected:
    std::unordered_map<InputKeyCode, InputKeyHandler> inputs_;

    std::function<void(InputKeyCode)> onKeyPress_{nullptr}, onKeyRelease_{nullptr};

public:
    Input() : IComponent(ComponentType::Input)
    {
    }

    void ReceiveTick() override
    {
    }

    void PowerOn() noexcept override
    {
    }

    void PowerOff() noexcept override
    {
    }

    struct InputKeyHandler& RegisterKey(InputKeyCode key)
    {
        if (inputs_.find(key) == inputs_.end()) {
            inputs_[key] = {nullptr, nullptr};
        }
        return inputs_[key];
    }

    void SetKeyPressHandlers(std::function<void(InputKeyCode)> onPress, std::function<void(InputKeyCode)> onRelease)
    {
        onKeyPress_ = onPress;
        onKeyRelease_ = onRelease;
    }

    void PressKey(InputKeyCode key)
    {
        if (inputs_.find(key) == inputs_.end()) {
            return;
        }
        auto& handler = inputs_[key];

        // Don't signal onPress if key is already pressed
        if (handler.isPressed) {
            return;
        }

        handler.isPressed = true;
        if (handler.onPress) {
            handler.onPress();
        }

        if (onKeyPress_) {
            onKeyPress_(key);
        }
    }

    void ReleaseKey(InputKeyCode key)
    {
        if (inputs_.find(key) == inputs_.end()) {
            return;
        }
        auto& handler = inputs_[key];

        // Don't signal onRelease if key is already released
        if (!handler.isPressed) {
            return;
        }

        handler.isPressed = false;
        if (handler.onRelease) {
            handler.onRelease();
        }

        if (onKeyRelease_) {
            onKeyRelease_(key);
        }
    }

    void ToggleKey(InputKeyCode key)
    {
        if (inputs_.find(key) == inputs_.end()) {
            return;
        }
        auto& handler = inputs_[key];

        handler.isPressed = !handler.isPressed;
        if (handler.isPressed && handler.onPress) {
            handler.onPress();
        } else if (!handler.isPressed && handler.onRelease) {
            handler.onRelease();
        }
    }

    bool IsPressed(InputKeyCode key) const noexcept
    {
        if (inputs_.find(key) == inputs_.end()) {
            return false;
        }
        return inputs_.at(key).isPressed;
    }
};

}; // namespace emulator::component
