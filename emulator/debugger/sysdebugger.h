#pragma once

#include <cstdint>
#include <format>
#include <functional>
#include <string>

namespace emulator::debugger
{

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
    {
    }

    std::string Generic() const
    {
        switch (generic) {
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
        return "";
    }

    std::string Encoding() const
    {
        switch (encoding) {
        case RegisterInfoEncoding::UINT:
            return "uint";
        case RegisterInfoEncoding::SINT:
            return "sint";
        case RegisterInfoEncoding::FLOAT:
            return "ieee754";
        }
        return "";
    }

    std::string Format() const
    {
        switch (format) {
        case RegisterInfoFormat::BINARY:
            return "binary";
        case RegisterInfoFormat::DECIMAL:
            return "decimal";
        case RegisterInfoFormat::FLOAT:
            return "float";
        case RegisterInfoFormat::HEX:
            return "hex";
        }
        return "";
    }

    std::string ToString() const
    {
        auto ret = std::format("name:{};alt-name:{};bitsize:{};offset:{};encoding:{};format:{};set:{}",
                               name, altName, bitSize, offset, Encoding(), Format(), group);

        if (generic != RegisterInfoGenericType::NONE) {
            ret += std::format(";generic:{}", Generic());
        }

        return ret;
    }
};

enum class NotificationType {
    CPU_STEP,
};

class ISystemDebugger
{
protected:
    std::string name_;

    bool stepMode_{false}; // Determine if the CPU should step or not
    std::size_t stepCount_{0};
    std::function<void(void)> stepCompleteCallback_{nullptr};

    mutable bool stopped_{true};

public:
    ISystemDebugger(std::string name) : name_(name)
    {
    }

    virtual ~ISystemDebugger() = default;

    std::string GetName() const noexcept { return name_; }
    bool IsStopped() const noexcept { return stopped_; };

    virtual std::uint32_t GetCurrentPID() const noexcept { return 1; }

    virtual constexpr std::uint32_t GetPtrSize() const noexcept = 0;

    virtual const RegisterInfo* GetRegisterInfo(std::size_t regNum) const noexcept = 0;

    virtual void HandleSignal(std::uint8_t signal) const noexcept = 0;

    virtual bool GetRegister(std::string name, std::uint64_t&) const noexcept = 0;
    virtual bool SetRegister(std::string name, std::uint64_t) noexcept = 0;

    virtual std::uint8_t* ReadMemory(Address, std::size_t&) const noexcept = 0;
    virtual bool WriteMemory(Address, void*, std::size_t) noexcept = 0;

    virtual void StepCPU(std::size_t instructions, std::function<void(void)> callback = nullptr) noexcept
    {
        stepCompleteCallback_ = callback;
        stepCount_ = instructions;
        stepMode_ = true;
        stopped_ = false;
    }

    virtual void RunCPU() noexcept
    {
        stepCount_ = 0;
        stepMode_ = false;
        stopped_ = false;
    }

    virtual void ShutdownCPU() noexcept
    {
    }

    // Function that the emulator systems can call to alert of state changes
    virtual void Notify(NotificationType type, void*) noexcept {};
};

}; // namespace emulator::debugger
