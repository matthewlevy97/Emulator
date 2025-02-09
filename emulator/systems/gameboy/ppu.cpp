#include "ppu.h"

#include <components/exceptions/AddressInUse.h>

namespace emulator::gameboy
{

PPU::PPU() : emulator::component::Display(160, 144),
             mode_(PPUMode::OAM),
             SCY_(0), SCX_(0),
             LY_(0), LX_(0),
             tickTracker_(0)
{
}

void PPU::ReceiveTick()
{
    ++tickTracker_;

    switch (mode_) {
    case PPUMode::OAM:
        handleOAM();
        break;
    case PPUMode::PixelTransfer:
        handlePixelTransfer();
        break;
    case PPUMode::HBlank:
        handleHBlank();
        break;
    case PPUMode::VBlank:
        handleVBlank();
        break;
    }
}

void PPU::AttachToBus(emulator::component::Bus* bus)
{
    if (!bus->RegisterComponentAddressRange(this, {0xFF40, 0xFF4B})) {
        throw component::AddressInUse(0xFF40, 0xB);
    }
    bus_ = bus;
}

void PPU::handleOAM()
{
    // TODO: Implement
    if (tickTracker_ < 80) [[likely]] {
        return;
    }

    LX_ = 0;
    mode_ = PPUMode::PixelTransfer;
}

void PPU::handlePixelTransfer()
{
    // TODO: Implement
    ++LX_;
    if (LX_ >= 160) {
        mode_ = PPUMode::HBlank;
    }
}

void PPU::handleHBlank()
{
    if (tickTracker_ < 456) [[likely]] {
        return;
    }
    LY_++;
    tickTracker_ = 0;

    if (LY_ >= 144) {
        mode_ = PPUMode::VBlank;
    } else {
        mode_ = PPUMode::OAM;
    }
}

void PPU::handleVBlank()
{
    if (tickTracker_ < 456) [[likely]] {
        return;
    }
    LY_++;
    tickTracker_ = 0;

    if (LY_ >= 153) {
        LY_ = 0;
        mode_ = PPUMode::OAM;
    }
}

}; // namespace emulator::gameboy
