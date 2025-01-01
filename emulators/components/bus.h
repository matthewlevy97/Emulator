#pragma once

#include <unordered_map>
#include <vector>

#include "component.h"

namespace emulator::component {

class Bus {
private:
    std::vector<IComponent*> components_;

    struct AddressRange {
        std::size_t start;
        std::size_t end;

        IComponent* component;

        bool operator<(const AddressRange& other) const {
            return std::tie(start, end) < std::tie(other.start, other.end);
        }

        bool operator==(const AddressRange& other) const {
            return start == other.start && end == other.end && component == other.component;
        }
    };

    std::vector<AddressRange> addressRanges_;

public:
    Bus();
    ~Bus();

    template <typename T>
    struct always_false : std::false_type {};

    void AddComponent(IComponent* component);
    void RemoveComponent(IComponent* component);
    bool RegisterComponentAddressRange(IComponent* component, std::pair<size_t, std::size_t> range);

    void ReceiveTick();

    void PowerOn();
    void PowerOff();

    template <typename T>
    T Read(std::size_t address) {
        static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>,
            "Read function only accepts integral or floating point types.");
        
        for (const auto& addressable : addressRanges_) {
            if (address >= addressable.start && address <= addressable.end) {
                if constexpr (std::is_same_v<T, uint8_t>) {
                    return addressable.component->ReadUInt8(address);
                } else if constexpr (std::is_same_v<T, int8_t>) {
                    return addressable.component->ReadInt8(address);
                } else if constexpr (std::is_same_v<T, uint16_t>) {
                    return addressable.component->ReadUInt16(address);
                } else if constexpr (std::is_same_v<T, int16_t>) {
                    return addressable.component->ReadInt16(address);
                } else if constexpr (std::is_same_v<T, uint32_t>) {
                    return addressable.component->ReadUInt32(address);
                } else if constexpr (std::is_same_v<T, int32_t>) {
                    return addressable.component->ReadInt32(address);
                } else if constexpr (std::is_same_v<T, float>) {
                    return addressable.component->ReadFloat(address);
                } else {
                    static_assert(always_false<T>::value, "Unsupported read type on bus");
                }
            }
        }
        throw std::out_of_range("No component found for read address on bus");
    }

    template <typename T>
    void Write(std::size_t address, T value) {
        static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value,
            "Write function only accepts integral or floating point types.");

        for (const auto& addressable : addressRanges_) {
            if (address >= addressable.start && address <= addressable.end) {
                if constexpr (std::is_same_v<T, uint8_t>) {
                    addressable.component->WriteUInt8(address, value);
                    return;
                } else if constexpr (std::is_same_v<T, int8_t>) {
                    addressable.component->WriteInt8(address, value);
                    return;
                } else if constexpr (std::is_same_v<T, uint16_t>) {
                    addressable.component->WriteUInt16(address, value);
                    return;
                } else if constexpr (std::is_same_v<T, int16_t>) {
                    addressable.component->WriteInt16(address, value);
                    return;
                } else if constexpr (std::is_same_v<T, uint32_t>) {
                    addressable.component->WriteUInt32(address, value);
                    return;
                } else if constexpr (std::is_same_v<T, int32_t>) {
                    addressable.component->WriteInt32(address, value);
                    return;
                } else if constexpr (std::is_same_v<T, float>) {
                    addressable.component->WriteFloat(address, value);
                    return;
                } else {
                    static_assert(always_false<T>::value, "Unsupported write type on bus");
                }
            }
        }
        throw std::out_of_range("No component found for write address on bus");
    }
};

}; // namespace emulator::component
