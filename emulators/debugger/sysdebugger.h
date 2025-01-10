#pragma once

#include <string>
#include <format>

namespace emulator::debugger {

static constexpr int kSIGTRAP = 5;

using Address = std::uint64_t;

struct RegisterInfo {
    std::string name;
    std::string altName; // Optional

    enum class RegisterInfoGenericType {
        NONE,
        PROGRAM_COUNTER,
        STACK_POINTER,
        FRAME_POINTER,
        RETURN_ADDRESS,
        FLAGS,
        ARG1,
        ARG2,
        ARG3,
        ARG4,
        ARG5,
        ARG6,
        ARG7,
        ARG8,
    } generic;

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
        bitSize(32), offset(0), group(""), generic(RegisterInfoGenericType::NONE)
    {}

    std::string Generic() const
    {
        switch (generic)
        {
        case RegisterInfoGenericType::NONE:
            return "";
        case RegisterInfoGenericType::PROGRAM_COUNTER:
            return "pc";
        case RegisterInfoGenericType::STACK_POINTER:
            return "sp";
        case RegisterInfoGenericType::FRAME_POINTER:
            return "fp";
        case RegisterInfoGenericType::RETURN_ADDRESS:
            return "ra";
        case RegisterInfoGenericType::FLAGS:
            return "flag";
        case RegisterInfoGenericType::ARG1:
            return "arg1";
        case RegisterInfoGenericType::ARG2:
            return "arg2";
        case RegisterInfoGenericType::ARG3:
            return "arg3";
        case RegisterInfoGenericType::ARG4:
            return "arg4";
        case RegisterInfoGenericType::ARG5:
            return "arg5";
        case RegisterInfoGenericType::ARG6:
            return "arg6";
        case RegisterInfoGenericType::ARG7:
            return "arg7";
        case RegisterInfoGenericType::ARG8:
            return "arg8";
        }
    }

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

    std::string ToString() const
    {
        auto ret = std::format("name:{};alt-name:{};bitsize:{};offset:{};encoding:{};format:{};set:{}",
            name, altName, bitSize, offset, Encoding(), Format(), group
        );

        if (generic != RegisterInfoGenericType::NONE) {
            ret += std::format(";generic:{}", Generic());
        }

        return ret;
    }
};

class ISystemDebugger {
protected:
    std::string name_;

public:
    ISystemDebugger(std::string name) : name_(name)
    {}

    virtual ~ISystemDebugger() = default;

    std::string GetName() const noexcept { return name_; }

    virtual std::uint32_t GetCurrentPID() const noexcept { return 1; }
    virtual std::uint32_t GetPtrSize() const noexcept { return 4; }

    virtual RegisterInfo* GetRegisterInfo(std::size_t regNum) const noexcept = 0;

    virtual void HandleSignal(std::uint8_t signal) const noexcept = 0;

    virtual std::uint64_t GetRegister(std::string name) const noexcept = 0;
    virtual bool SetRegister(std::string name, std::uint64_t) noexcept = 0;

    virtual std::uint8_t* ReadMemory(Address, std::size_t&) const noexcept = 0;
    virtual bool WriteMemory(Address, void*, std::size_t) noexcept = 0;

    virtual void StepCPU(std::size_t instructions) noexcept = 0;
    virtual void RunCPU() noexcept = 0;
    virtual void ShutdownCPU() noexcept = 0;

    virtual bool IsStopped() const noexcept = 0;
};


}; // namespace emulator::debugger
