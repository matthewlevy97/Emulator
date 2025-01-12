#pragma once

#include <cstdint>
#include <exception>
#include <format>
#include <string>

namespace emulator::component {

class InvalidAddress : public std::exception {
public:
    enum class AccessType {
        UNKNOWN,
        READ,
        WRITE
    };

private:
    std::uint64_t address_;
    AccessType access_;
    std::string msg_;

public:
    InvalidAddress(std::uint64_t address)
        : InvalidAddress(address, AccessType::UNKNOWN, "") {}

    InvalidAddress(std::uint64_t address, AccessType type)
        : InvalidAddress(address, type, "") {}

    InvalidAddress(std::uint64_t address, const std::string& extraInfo)
        : InvalidAddress(address, AccessType::UNKNOWN, extraInfo) {}

    InvalidAddress(std::uint64_t address, AccessType type, const std::string& extraInfo)
        : address_(address), access_(type)
    {
        const char* typeStr = type == AccessType::READ ? "read" : (type == AccessType::WRITE ? "write" : "access");
        if (extraInfo.empty()) {
            msg_ = std::format("Invalid address {}: 0x{:04X}", typeStr, address);
        } else {
            msg_ = std::format("Invalid address {}: 0x{:04X} ({})", typeStr, address, extraInfo);
        }
    }

    const std::uint64_t Address() const noexcept
    {
        return address_;
    }

    const AccessType Access() const noexcept
    {
        return access_;
    }

    const char* what() const noexcept override
    {
        return msg_.c_str();
    }
};

}; // namespace emulator::component
