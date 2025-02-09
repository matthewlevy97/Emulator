#pragma once

#include <components/bus.h>
#include <components/display.h>

#include "names.h"

namespace emulator::gameboy
{

enum class PPUMode {
    HBlank = 0,
    VBlank = 1,
    OAM = 2,
    PixelTransfer = 3
};

class PPU : public emulator::component::Display
{
private:
    std::size_t SCY_, SCX_;
    std::size_t LY_, LX_;

    PPUMode mode_;
    std::size_t tickTracker_;

    void handleOAM();
    void handlePixelTransfer();
    void handleHBlank();
    void handleVBlank();

public:
    PPU();

    void AttachToBus(emulator::component::Bus* bus) override;

    void ReceiveTick() override;

    void WriteUInt8(std::size_t address, std::uint8_t value) override
    {
        if (address == 0xFF44) {
            /*
            * LY indicates the current horizontal line, which might be about to be drawn, being drawn, or
            * just been drawn. LY can hold any value from 0 to 153, with values from 144 to 153 indicating
            * the VBlank period.
            */
           LY_ = 0;
        }
    }

    std::uint8_t ReadUInt8(std::size_t address) override
    {
        if (address == 0xFF42) {
            return SCY_;
        } else if (address == 0xFF43) {
            return SCX_;
        } else if (address == 0xFF44) {
            return LY_;
        } else {
            spdlog::error("PPU: Read from unhandled address: {0:x}", address);
        }
        return 0;
    }
};

}; // namespace emulator::gameboy
