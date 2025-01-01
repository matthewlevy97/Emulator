#pragma once

#include <vector>

#include "memory.h"
#include "exceptions/AddressInUse.h"

namespace emulator::component {

template <MemoryType mtype>
class MultiMappedMemory : public Memory<mtype> {
private:
    std::vector<std::size_t> baseAddresses_;

    bool InMemoryRange(std::size_t address) noexcept
    {
        for (auto base : baseAddresses_) {
            std::size_t bound = base + this->memory_.size();
            if (base <= address && bound > address) {
                return true;
            }
        }
        return false;
    }

    std::size_t NormalizeToBaseAddress(std::size_t address)
    {
        for (auto base : baseAddresses_) {
            std::size_t bound = base + this->memory_.size();
            if (base <= address && bound > address) {
                return address - base;
            }
        }
        throw std::out_of_range("Cannot normalize multi-mapped memory for OOB address.");
    }

public:
    MultiMappedMemory(std::size_t size) : MultiMappedMemory({0}, size) {}
    MultiMappedMemory(std::vector<std::size_t> baseAddresses, size_t size)
        : Memory<mtype>(size), baseAddresses_(baseAddresses)
    {}

    void AttachToBus(Bus* bus) override
    {
        for (auto base : baseAddresses_) {
            std::size_t bound = base + this->memory_.size();
            if (!bus->RegisterComponentAddressRange(this, { base, bound })) {
                throw AddressInUse();
            }
        }
        this->bus_ = bus;
    }

    void WriteInt8(std::size_t address, std::int8_t value) override
    {
        if (!InMemoryRange(address)) {
            throw std::out_of_range("Not in range of multi-mapped memory.");
        }
        Memory<mtype>::WriteInt8(NormalizeToBaseAddress(address), value);
    }

    void WriteUInt8(std::size_t address, std::uint8_t value) override
    {
        if (!InMemoryRange(address)) {
            throw std::out_of_range("Not in range of multi-mapped memory.");
        }
        Memory<mtype>::WriteUInt8(NormalizeToBaseAddress(address), value);
    }

    void WriteInt16(std::size_t address, std::int16_t value) override
    {
        if (!InMemoryRange(address)) {
            throw std::out_of_range("Not in range of multi-mapped memory.");
        }
        Memory<mtype>::WriteInt16(NormalizeToBaseAddress(address), value);
    }

    void WriteUInt16(std::size_t address, std::uint16_t value) override
    {
        if (!InMemoryRange(address)) {
            throw std::out_of_range("Not in range of multi-mapped memory.");
        }
        Memory<mtype>::WriteUInt16(NormalizeToBaseAddress(address), value);
    }

    void WriteUInt32(std::size_t address, std::uint32_t value) override
    {
        if (!InMemoryRange(address)) {
            throw std::out_of_range("Not in range of multi-mapped memory.");
        }
        Memory<mtype>::WriteUInt32(NormalizeToBaseAddress(address), value);
    }

    void WriteInt32(std::size_t address, std::int32_t value) override
    {
        // Validate in one of our address ranges
        if (!InMemoryRange(address)) {
            throw std::out_of_range("Not in range of multi-mapped memory.");
        }
        Memory<mtype>::WriteInt32(NormalizeToBaseAddress(address), value);
    }

    std::int8_t ReadInt8(std::size_t address) override
    {
        if (!InMemoryRange(address)) {
            throw std::out_of_range("Not in range of multi-mapped memory.");
        }
        return Memory<mtype>::ReadInt8(NormalizeToBaseAddress(address));
    }

    std::uint8_t ReadUInt8(std::size_t address) override
    {
        if (!InMemoryRange(address)) {
            throw std::out_of_range("Not in range of multi-mapped memory.");
        }
        return Memory<mtype>::ReadUInt8(NormalizeToBaseAddress(address));
    }

    std::int16_t ReadInt16(std::size_t address) override
    {
        if (!InMemoryRange(address)) {
            throw std::out_of_range("Not in range of multi-mapped memory.");
        }
        return Memory<mtype>::ReadInt16(NormalizeToBaseAddress(address));
    }

    std::uint16_t ReadUInt16(std::size_t address) override
    {
        if (!InMemoryRange(address)) {
            throw std::out_of_range("Not in range of multi-mapped memory.");
        }
        return Memory<mtype>::ReadUInt16(NormalizeToBaseAddress(address));
    }

    std::uint32_t ReadUInt32(std::size_t address) override
    {
        if (!InMemoryRange(address)) {
            throw std::out_of_range("Not in range of multi-mapped memory.");
        }
        return Memory<mtype>::ReadUInt32(NormalizeToBaseAddress(address));
    }
    
    std::int32_t ReadInt32(std::size_t address) override
    {
        // Validate in one of our address ranges
        if (!InMemoryRange(address)) {
            throw std::out_of_range("Not in range of multi-mapped memory.");
        }
        return Memory<mtype>::ReadInt32(NormalizeToBaseAddress(address));
    }
};

}; // namespace emulator::component
