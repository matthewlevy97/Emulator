#include "bus.h"

#include <spdlog/spdlog.h>

namespace emulator::component {

Bus::Bus() {}

Bus::~Bus() {
    for (auto component : components_) {
        delete component;
    }
}

void Bus::BindSystem(System* system) noexcept
{
    system_ = system;
}

void Bus::AddComponent(IComponent* component)
{
    component->AttachToBus(this);
    components_.push_back(component);
}

void Bus::RemoveComponent(IComponent* component)
{
    for (auto& addressable : addressRanges_) {
        if (addressable.component == component) {
            addressRanges_.erase(
                std::remove(addressRanges_.begin(), addressRanges_.end(), addressable),
                addressRanges_.end()
            );
        }
    }

    components_.erase(
        std::remove(components_.begin(), components_.end(), component),
        components_.end()
    );
}

bool Bus::RegisterComponentAddressRange(IComponent* component, std::pair<std::size_t, std::size_t> range) noexcept
{
    for (const auto& addressable : addressRanges_) {
        // If new address is within an existing address range, return false
        if (range.first >= addressable.start && range.first < addressable.end) {
            return false;
        }
        if (range.second >= addressable.start && range.second <= addressable.end) {
            return false;
        }
    }

    spdlog::trace("[bus] Registering address range 0x{:X}-0x{:X}", range.first, range.second);
    addressRanges_.push_back({
        range.first,
        range.second,
        component
    });

    return true;
}

bool Bus::UpdateComponentAddressRange(IComponent* component, std::pair<std::size_t, std::size_t> range) noexcept
{
    // Is this double loop the most efficient way to do this, no
    // Is it the most readable, yes
    for (const auto& addressable : addressRanges_) {
        if (addressable.component == component) {
            continue;
        }

        // If new address is within an existing address range, return false
        if (range.first >= addressable.start && range.first < addressable.end) {
            return false;
        }
        if (range.second >= addressable.start && range.second <= addressable.end) {
            return false;
        }
    }

    for (auto& addressable : addressRanges_) {
        if (addressable.component == component) {
            spdlog::trace("[bus] Updating address range 0x{:X}-0x{:X} -> 0x{:X}-0x{:X}",
                addressable.start, addressable.end,
                range.first, range.second);

            addressable.start = range.first;
            addressable.end = range.second;
        }
    }
    
    return true;
}

void Bus::AddMemoryWatchPoint(MemoryWatchAddress addr) noexcept
{
    for (const auto& watch : memoryWatchPoints_) {
        if (watch == addr) {
            return;
        }
    }

    memoryWatchPoints_.push_back(addr);
}

void Bus::RemoveMemoryWatchPoint(MemoryWatchAddress addr) noexcept
{
    for (std::size_t i = 0; i < memoryWatchPoints_.size(); ++i) {
        if (memoryWatchPoints_[i] == addr) {
            memoryWatchPoints_[i] = memoryWatchPoints_.back();
            memoryWatchPoints_.pop_back();
            return;
        }
    }
}

void Bus::RegisterMemoryWatchCallback(MemoryWatchCallback callback) noexcept
{
    memoryWatchCallback_ = callback;
}

void Bus::ReceiveTick()
{
    for (auto component : components_) {
        component->ReceiveTick();
    }
}

void Bus::PowerOn() noexcept
{
    for (auto component : components_) {
        component->PowerOn();
    }
}

void Bus::PowerOff() noexcept
{
    for (auto component : components_) {
        component->PowerOff();
    }
}

void Bus::LogStacktrace() noexcept
{
    for (auto component : components_) {
        component->LogStacktrace();
    }
}

}; // namespace emulator::component
