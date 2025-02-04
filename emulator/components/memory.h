#pragma once

#include <algorithm>
#include <cstring>
#include <vector>

#include "bus.h"
#include "component.h"

#include "exceptions/AddressInUse.h"

namespace emulator::component {

enum class MemoryType
{
    ReadWrite,
    ReadOnly
};

template <MemoryType mtype>
class Memory : public IComponent {
public:
    using ContextID = std::size_t;

protected:
    struct MemoryContext {
        std::size_t offset;
        std::size_t size;
        void* data;

        MemoryContext() : offset(0), size(0), data(nullptr) {}
        MemoryContext(std::size_t offset, std::size_t size, void* data) : offset(offset), size(size), data(data) {}
    };

    std::vector<MemoryContext> contexts_;

    std::vector<std::uint8_t> memory_;

public:
    Memory(std::size_t size) : Memory(0, size) {}
    Memory(std::size_t baseAddress, size_t size) : IComponent(IComponent::ComponentType::Memory)
    {
        if (baseAddress_ + size < baseAddress_) {
            throw std::invalid_argument("Invalid address range");
        }

        baseAddress_ = baseAddress;
        boundAddress_ = baseAddress + size;
        
        memory_.resize(size);
        memory_.shrink_to_fit();
    };

    void Clear()
    {
        for (std::size_t i = 0; i < memory_.size(); i++) {
            memory_[i] = 0;
        }
    }

    bool LoadData(const char* data, std::size_t size, std::size_t offset = 0) noexcept
    {
        if (baseAddress_ + offset + size > boundAddress_ || baseAddress_ + offset + size < baseAddress_) {
            return false;
        }

        std::memcpy(memory_.data() + offset, data, size);
        return true;
    }

    void Remap(std::size_t baseAddress, std::size_t size, std::size_t offset = 0)
    {
        if (baseAddress + size < baseAddress) {
            throw std::invalid_argument("Invalid address range");
        }

        baseAddress_ = baseAddress;
        boundAddress_ = baseAddress + size;

        memory_.resize(size);
        memory_.shrink_to_fit();

        if (offset != 0) [[unlikely]] {
            std::shift_left(memory_, offset);
        }

        // Update bus about change
        if (!bus_->UpdateComponentAddressRange(this, { baseAddress_, baseAddress_ + memory_.size() - 1 })) {
            throw AddressInUse(baseAddress_, memory_.size());
        }
    }

    ContextID SaveContext(std::size_t size, std::size_t offset = 0) noexcept
    {
        if (offset + size > memory_.size()) {
            return ContextID(-1);
        }

        void* data = std::malloc(size);
        std::memcpy(data, memory_.data() + offset, size);
        contexts_.emplace_back(offset, size, data);
        return ContextID(contexts_.size() - 1);
    }

    bool RestoreContext(ContextID id) noexcept
    {
        if (id >= contexts_.size()) {
            return false;
        }

        auto& context = contexts_[id];
        std::memcpy(memory_.data() + context.offset, context.data, context.size);
        return true;
    }

    void AttachToBus(Bus* bus) override
    {
        if (!bus->RegisterComponentAddressRange(this, { baseAddress_, baseAddress_ + memory_.size() - 1 })) {
            throw AddressInUse(baseAddress_, memory_.size());
        }
        bus_ = bus;
    }

    void ReceiveTick() override {};

    void PowerOn() noexcept override {};
    void PowerOff() noexcept override {};

    void WriteUInt8(std::size_t address, std::uint8_t value) override
    {
        if constexpr (mtype == MemoryType::ReadOnly) {
            IComponent::WriteUInt8(address, value);
        } else {
            auto normalizedAddress = ValidateAndNormalizeAddress<std::uint8_t>(address);
            memory_[normalizedAddress] = value;
        }
    }

    void WriteInt8(std::size_t address, std::int8_t value) override
    {
        if constexpr (mtype == MemoryType::ReadOnly) {
            IComponent::WriteInt8(address, value);
        } else {
            auto normalizedAddress = ValidateAndNormalizeAddress<std::int8_t>(address);
            memory_[normalizedAddress] = value;
        }
    }

    void WriteUInt16(std::size_t address, std::uint16_t value) override
    {
        if constexpr (mtype == MemoryType::ReadOnly) {
            IComponent::WriteUInt16(address, value);
        } else {
            auto normalizedAddress = ValidateAndNormalizeAddress<std::uint16_t>(address);
            memory_[normalizedAddress] = value & 0xFF;
            memory_[normalizedAddress + 1] = (value >> 8) & 0xFF;
        }
    }

    void WriteInt16(std::size_t address, std::int16_t value) override
    {
        if constexpr (mtype == MemoryType::ReadOnly) {
            IComponent::WriteInt16(address, value);
        } else {
            auto normalizedAddress = ValidateAndNormalizeAddress<std::int16_t>(address);
            memory_[normalizedAddress] = value & 0xFF;
            memory_[normalizedAddress + 1] = (value >> 8) & 0xFF;
        }
    }

    void WriteUInt32(std::size_t address, std::uint32_t value) override
    {
        if constexpr (mtype == MemoryType::ReadOnly) {
            IComponent::WriteUInt32(address, value);
        } else {
            auto normalizedAddress = ValidateAndNormalizeAddress<std::uint32_t>(address);
            memory_[normalizedAddress] = value & 0xFF;
            memory_[normalizedAddress + 1] = (value >> 8) & 0xFF;
            memory_[normalizedAddress + 2] = (value >> 16) & 0xFF;
            memory_[normalizedAddress + 3] = (value >> 24) & 0xFF;
        }
    }

    void WriteInt32(std::size_t address, std::int32_t value) override
    {
        if constexpr (mtype == MemoryType::ReadOnly) {
            IComponent::WriteInt32(address, value);
        } else {
            auto normalizedAddress = ValidateAndNormalizeAddress<std::int32_t>(address);
            memory_[normalizedAddress] = value & 0xFF;
            memory_[normalizedAddress + 1] = (value >> 8) & 0xFF;
            memory_[normalizedAddress + 2] = (value >> 16) & 0xFF;
            memory_[normalizedAddress + 3] = (value >> 24) & 0xFF;
        }
    }

    std::uint8_t ReadUInt8(std::size_t address) override
    {
        auto normalizedAddress = ValidateAndNormalizeAddress<std::uint8_t>(address);
        return static_cast<std::uint8_t>(memory_[normalizedAddress]);
    }

    std::int8_t ReadInt8(std::size_t address) override
    {
        auto normalizedAddress = ValidateAndNormalizeAddress<std::int8_t>(address);
        return static_cast<std::int8_t>(memory_[normalizedAddress]);
    }

    std::uint16_t ReadUInt16(std::size_t address) override
    {
        auto normalizedAddress = ValidateAndNormalizeAddress<std::uint16_t>(address);
        return static_cast<std::uint16_t>(memory_[normalizedAddress] | (memory_[normalizedAddress + 1] << 8));
    }

    std::int16_t ReadInt16(std::size_t address) override
    {
        auto normalizedAddress = ValidateAndNormalizeAddress<std::int16_t>(address);
        return static_cast<std::int16_t>(memory_[normalizedAddress] | (memory_[normalizedAddress + 1] << 8));
    }

    std::uint32_t ReadUInt32(std::size_t address) override
    {
        auto normalizedAddress = ValidateAndNormalizeAddress<std::uint32_t>(address);
        return static_cast<std::uint32_t>(memory_[normalizedAddress] | (memory_[normalizedAddress + 1] << 8) | (memory_[normalizedAddress + 2] << 16) | (memory_[normalizedAddress + 3] << 24));
    }

    std::int32_t ReadInt32(std::size_t address) override
    {
        auto normalizedAddress = ValidateAndNormalizeAddress<std::int32_t>(address);
        return static_cast<std::int32_t>(memory_[normalizedAddress] | (memory_[normalizedAddress + 1] << 8) | (memory_[normalizedAddress + 2] << 16) | (memory_[normalizedAddress + 3] << 24));
    }
};

}; // namespace emulator::component