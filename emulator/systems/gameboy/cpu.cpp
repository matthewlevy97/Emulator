#include "cpu.h"
#include "debugger.h"

#include <components/exceptions/AddressInUse.h>
#include <components/memory.h>

#include <spdlog/spdlog.h>

namespace emulator::gameboy
{

CPU::CPU() : microcodeStackLength_(0), microcode_{}, registers_{}
{
}

CPU::CPU(const CPU& other)
    : microcodeStackLength_(other.microcodeStackLength_),
      microcode_(other.microcode_),
      registers_(other.registers_)
{
}

CPU::~CPU() {}

void CPU::ReceiveTick()
{
    // CPU based off M-Cycles which are every 4 T-Cycles
    static std::size_t TCycles = TCycleToMCycle;
    if (--TCycles > 0) {
        return;
    } else {
        TCycles = TCycleToMCycle;
    }

    // Pipeline executes fetch on same cycle as end of execute
    // Decode and execute are same cycle(s) on real system
    // Fetch and decode are same cycle(s) when emulating

    if (microcodeStackLength_ > 0) {
        auto microcode = microcode_[--microcodeStackLength_];
        if (microcode != nullptr) {
            microcode_[microcodeStackLength_] = nullptr;
            microcode(this);
        }
    }

    if (microcodeStackLength_ == 0) {
        // Step to next instruction
        auto system = GetSystem();
        if (system->DebuggerEnabled()) {
            auto debugger = system->GetDebugger();
            if (debugger != nullptr) {
                // Do nothing if debugger has CPU stopped
                if (debugger->IsStopped()) {
                    return;
                }

                // Alert that stepping to next instruction
                debugger->Notify(emulator::debugger::NotificationType::CPU_STEP, nullptr);
            }
        }

        // Fetch and generate microcode for execution
        auto pc = GetRegister<Registers::PC>();
        auto opcode = bus_->Read<std::uint8_t>(pc);
        AddRegister<Registers::PC>(1);

        DecodeOpcode(opcode);
    }
}

void CPU::PowerOn() noexcept {}

void CPU::PowerOff() noexcept
{
    microcodeStackLength_ = 0;
    microcode_.fill(0);
    registers_.fill(0);
}

void CPU::PushMicrocode(MicroCode code)
{
    if (microcodeStackLength_ >= sizeof(microcode_) / sizeof(MicroCode)) {
        throw std::runtime_error("Not enough space for registering microcode for execution");
    }

    microcode_[microcodeStackLength_++] = code;
}

void CPU::AttachToBus(component::Bus* bus)
{
    if (!bus->RegisterComponentAddressRange(this, {0xFF00, 0xFF40})) {
        throw component::AddressInUse(0xFF00, 0x40);
    }
    // Skip PPU controlled registers
    if (!bus->RegisterComponentAddressRange(this, {0xFF50, 0xFF70})) {
        throw component::AddressInUse(0xFF50, 0x20);
    }
    bus_ = bus;
}

void CPU::LogStacktrace() noexcept
{
    spdlog::debug("[CPU] AF: {:04X}   BC: {:04X}", GetRegister<Registers::AF>(), GetRegister<Registers::BC>());
    spdlog::debug("[CPU] DE: {:04X}   HL: {:04X}", GetRegister<Registers::DE>(), GetRegister<Registers::HL>());
    spdlog::debug("[CPU] SP: {:04X}   PC: {:04X}", GetRegister<Registers::SP>(), GetRegister<Registers::PC>());
}

// Load the first 32KiB of the ROM into the cartridge memory
void CPU::LoadROM(const char* data, std::size_t size) noexcept
{
    if (romData_ != nullptr) {
        delete romData_;
        romData_ = nullptr;
    }

    romData_ = new std::uint8_t[size];
    romSize_ = size;
    std::memcpy((void*)romData_, data, size);

    auto cartridge0 = reinterpret_cast<emulator::component::Memory<emulator::component::MemoryType::ReadOnly>*>(
        GetSystem()->GetComponent(emulator::gameboy::kCartridge0Name));
    if (cartridge0 != nullptr) {
        auto loadDataSize = std::min(size, static_cast<std::size_t>(0x4000));

        cartridge0->LoadData(data, loadDataSize);
        cartridge0->OverwriteContext(1, loadDataSize); // Make Context 1 the actual ROM data
        cartridge0->RestoreContext(0);                 // Restore Context 0 to boot/startup data

        if (loadDataSize < size) {
            size -= loadDataSize;
        }
    }

    if (size) {
        auto cartridgeN = reinterpret_cast<emulator::component::Memory<emulator::component::MemoryType::ReadOnly>*>(
            GetSystem()->GetComponent(emulator::gameboy::kCartridgeSwitchableName));
        if (cartridgeN != nullptr) {
            cartridgeN->LoadData(data, std::min(size, static_cast<std::size_t>(0x4000)));
        }
    }
}
}; // namespace emulator::gameboy