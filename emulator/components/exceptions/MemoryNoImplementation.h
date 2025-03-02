#pragma once

#include <exception>
#include <format>
#include <string>

namespace emulator::component
{

class MemoryNoImplementation : public std::exception
{
private:
    bool isRead_;
    std::uint64_t address_;
    std::size_t length_;
    std::string msg_;

public:
    MemoryNoImplementation(bool isRead, std::uint64_t address, std::size_t length)
        : isRead_(isRead), address_(address), length_(length)
    {
        msg_ = std::format("No defined implementation to handle {0} @ 0x{1:X}",
            isRead ? "read" : "write",
            address);
    }

    const bool IsRead() const noexcept
    {
        return isRead_;
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
