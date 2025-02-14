#pragma once

#include <exception>
#include <format>
#include <string>

namespace emulator::component
{

class AddressInUse : public std::exception
{
private:
    std::uint64_t address_;
    std::size_t length_;
    std::string msg_;

public:
    AddressInUse(std::uint64_t address, std::size_t length) : address_(address), length_(length)
    {
        msg_ = std::format("Address already in use: 0x{:04X} -> 0x{:04X}", address, address + length);
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
