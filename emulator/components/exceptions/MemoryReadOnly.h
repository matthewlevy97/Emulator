#pragma once

#include <exception>
#include <format>
#include <string>

namespace emulator::component
{

class MemoryReadOnlyViolation : public std::exception
{
private:
    std::uint64_t address_;
    std::size_t length_;
    std::string msg_;

public:
    MemoryReadOnlyViolation(std::uint64_t address, std::size_t length)
        : address_(address), length_(length)
    {
        msg_ = std::format("Attempted to write to read-only memory in range 0x{0:X} -> 0x{1:X}",
            address, length + address);
    }

    const std::uint64_t Address() const noexcept
    {
        return address_;
    }

    const std::size_t Length() const noexcept
    {
        return length_;
    }

    const char* what() const noexcept override
    {
        return msg_.c_str();
    }
};

}; // namespace emulator::component
