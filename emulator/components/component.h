#pragma once

#include <cstdint>

#include "exceptions/InvalidAddress.h"

namespace emulator::component {

class Bus;

class IComponent {
public:
    enum class ComponentType {
        CPU,
        Memory,
        Display,
        Input,
        Sound,
        Timer,
        Other
    };

protected:
    ComponentType type_{ComponentType::Other};

    Bus *bus_;
    Bus *GetBus() const { return bus_; }

    std::size_t baseAddress_{0}, boundAddress_{0};

    template <typename T>
    bool ValidateAddress(std::size_t address)
    {
        static_assert(std::is_integral<T>::value, "ValidateAddress only accept integral types.");
        return !(address < baseAddress_ || address + sizeof(T) - 1 >= boundAddress_);
    }

    template <typename T>
    std::size_t ValidateAndNormalizeAddress(std::size_t address)
    {
        if (!ValidateAddress<T>(address)) {
            throw InvalidAddress(address);
        }
        return address - baseAddress_;
    }

public:
    IComponent(ComponentType type) : IComponent(type, nullptr) {}
    IComponent(ComponentType type, Bus* bus) : type_{type}, bus_(bus) {}

    virtual inline ~IComponent() = default;

    ComponentType Type() const noexcept { return type_; }

    virtual void ReceiveTick() = 0;

    virtual void PowerOn() = 0;
    virtual void PowerOff() = 0;

    virtual void AttachToBus(Bus *bus)
    {
        bus_ = bus;
    }

    virtual void RemoveFromBus()
    {
        bus_ = nullptr;
    }

    virtual void LogStacktrace() noexcept
    {}

    //
    // Read and Write Interfaces
    //

    #define RW_STR_TYPE_NAME(x) #x
    #define READ_FUNC_DECL(TypeName, type) \
        virtual type Read ## TypeName (std::size_t address) { \
            throw std::runtime_error("Read" RW_STR_TYPE_NAME(TypeName) " not implemented"); \
        }
    #define WRITE_FUNC_DECL(TypeName, type) \
        virtual void Write ## TypeName (std::size_t address, type value) { \
            throw std::runtime_error("Write" RW_STR_TYPE_NAME(TypeName) " not implemented"); \
        }
    #define READ_WRITE_FUNC_DECL(TypeName, type) \
        READ_FUNC_DECL(TypeName, type) \
        WRITE_FUNC_DECL(TypeName, type)

    READ_WRITE_FUNC_DECL(Float, float);

    READ_WRITE_FUNC_DECL(Int8, std::int8_t);
    READ_WRITE_FUNC_DECL(UInt8, std::uint8_t);

    READ_WRITE_FUNC_DECL(Int16, std::int16_t);
    READ_WRITE_FUNC_DECL(UInt16, std::uint16_t);

    READ_WRITE_FUNC_DECL(Int32, std::int32_t);
    READ_WRITE_FUNC_DECL(UInt32, std::uint32_t);

    #undef RW_STR_TYPE_NAME
    #undef READ_FUNC_DECL
    #undef WRITE_FUNC_DECL
    #undef READ_WRITE_FUNC_DECL

    //
    // End of Read and Write Interfaces
    //
};

}; // namespace emulator::component
