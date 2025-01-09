#pragma once

#include <string>

namespace emulator::debugger {

using Address = std::uint64_t;

struct RegisterInfo {
    std::string name;
    std::string altName; // Optional

    enum class RegisterInfoEncoding {
        UINT,
        SINT,
        FLOAT,
    } encoding;

    enum class RegisterInfoFormat {
        BINARY,
        DECIMAL,
        FLOAT,
        HEX,
    } format;

    std::size_t bitSize;
    std::size_t offset;

    std::string group;

    RegisterInfo() : name(""), altName(""),
        encoding(RegisterInfoEncoding::UINT), format(RegisterInfoFormat::HEX),
        bitSize(32), offset(0), group("")
    {}

    std::string Encoding() const
    {
        switch (encoding)
        {
        case RegisterInfoEncoding::UINT:
            return "uint";
        case RegisterInfoEncoding::SINT:
            return "sint";
        case RegisterInfoEncoding::FLOAT:
            return "ieee754";
        }
    }

    std::string Format() const
    {
        switch (format)
        {
        case RegisterInfoFormat::BINARY:
            return "binary";
        case RegisterInfoFormat::DECIMAL:
            return "decimal";
        case RegisterInfoFormat::FLOAT:
            return "float";
        case RegisterInfoFormat::HEX:
            return "hex";
        }
    }
};

class ISystemDebugger {
protected:
    std::string name_;

public:
    ISystemDebugger(std::string name) : name_(name)
    {}

    virtual ~ISystemDebugger() = default;

    std::string GetName() const { return name_; }

    virtual std::uint32_t GetCurrentPID() const { return 1; }
    virtual std::uint32_t GetPtrSize() const { return 4; }

    virtual RegisterInfo& GetRegisterInfo(std::size_t regNum) const = 0;

    virtual std::uint64_t GetRegister(std::string name) const = 0;
    virtual bool SetRegister(std::string name, std::uint64_t) = 0;

    virtual void* ReadMemory(Address, std::size_t) const = 0;
    virtual bool WriteMemory(Address, std::size_t, void*) = 0;
};


}; // namespace emulator::debugger
