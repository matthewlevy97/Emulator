#pragma once

#include "bus.h"

namespace emulator::component {

Bus::Bus() : tickRate_(0.0f) {}
Bus::~Bus() {
    for (auto component : components_) {
        delete component;
    }
}

void Bus::AddComponent(IComponent* component) {
    components_.push_back(component);
}

void Bus::SetTickRate(float rate) {
    tickRate_ = rate;
}

float Bus::GetTickRate() const {
    return tickRate_;
}

void Bus::PowerOn() {
    for (auto component : components_) {
        component->PowerOn();
    }
}

void Bus::PowerOff() {
    for (auto component : components_) {
        component->PowerOff();
    }
}

}; // namespace emulator::component
