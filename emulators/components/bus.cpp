#include "bus.h"

namespace emulator::component {

Bus::Bus() {}

Bus::~Bus() {
    for (auto component : components_) {
        delete component;
    }
}

void Bus::AddComponent(IComponent* component) {
    components_.push_back(component);
}

void Bus::ReceiveTick()
{
    for (auto component : components_) {
        component->ReceiveTick();
    }
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
