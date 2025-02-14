#include "ppu.h"

#include <components/exceptions/AddressInUse.h>

namespace emulator::gameboy
{

PPU::PPU() : emulator::component::Display(160, 144),
             mode_(PPUMode::OAM),
             SCY_(0), SCX_(0),
             LY_(0), LX_(0),
             WY_(0), WX_(0),
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
    if (tickTracker_ < 80) [[likely]] {
        return;
    }

    LX_ = 0;
    mode_ = PPUMode::PixelTransfer;

    spriteFIFO_.Clear();
    bgFIFO_.Clear();
}

void PPU::handlePixelTransfer()
{
    // TODO: Determine if Background or Window is being rendered

    // Handle Background FIFO
    handlePixelTransferBackground();

    // TODO: Handle Sprite FIFO

    // Pixel Mix
    if (bgFIFO_.Empty()) {
        return;
    }
    auto bgPixel = bgFIFO_.Pop();
    // TODO: Get Sprite Pixel
    std::uint8_t mixedPixel = bgPixel;

    // Generate Pixel Data
    SetPixel(LX_, LY_, getDisplayPixelForColor(mixedPixel));
    ++LX_;

    if (LX_ >= 160) {
        mode_ = PPUMode::HBlank;
    }
}

void PPU::handlePixelTransferBackground() noexcept
{
    switch (pixelTransferBackgroundState_.pixelTransferBackgroundTick) {
    case 0:
        // Optimized fallthrough
        break;
    case 1:
        pixelTransferBackgroundState_.tileMapAddress = calculateTileMapAddress(
            !bgTileMapArea_
                ? kWindowTileMapArea0
                : kWindowTileMapArea1);
        pixelTransferBackgroundState_.tileLine = 2 * ((LY_ + SCY_) % 8);
        break;
    case 2:
        // Optimized fallthrough
        break;
    case 3:
        pixelTransferBackgroundState_.tileDataLow = bus_->Read<std::uint8_t>(
            pixelTransferBackgroundState_.tileMapAddress +
            pixelTransferBackgroundState_.tileLine);
        break;
    case 4:
        // Optimized fallthrough
        break;
    case 5:
        pixelTransferBackgroundState_.tileDataHigh = bus_->Read<std::uint8_t>(
            pixelTransferBackgroundState_.tileMapAddress +
            pixelTransferBackgroundState_.tileLine + 1);
        break;
    default:
        // Push pixel data to FIFO
        if (!bgFIFO_.Empty()) {
            return;
        }

        // TODO: Possibly Loop Unroll this manually
        for (int i = 0; i < 8; ++i) {
            std::uint8_t colorID = ((pixelTransferBackgroundState_.tileDataLow >> (7 - i)) & 0x1) | (((pixelTransferBackgroundState_.tileDataHigh >> (7 - i)) & 0x1) << 1);
            bgFIFO_.Push(colorID);
        }
        pixelTransferBackgroundState_.pixelTransferBackgroundTick = 0;
        return;
    }

    ++pixelTransferBackgroundState_.pixelTransferBackgroundTick;
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

std::uint16_t PPU::calculateTileMapAddress(std::uint16_t tileMapAddress) const noexcept
{
    // Calculate the tile coordinates
    std::uint8_t tileX = ((LX_ + SCX_) / 8);
    std::uint8_t tileY = ((LY_ + SCY_) / 8);

    // Calculate the tile map address
    std::uint16_t tileNumber = (tileY * 32) + tileX;
    std::uint16_t tileDataNumber = bus_->Read<std::uint8_t>(tileMapAddress + tileNumber);

    // Use the addressing mode to calculate the tile map address
    if (bgWindowTileDataAddressingMode_) {
        return kBGWindowTileDataArea0 + (tileDataNumber + 128) * 16;
    } else {
        return kBGWindowTileDataArea1 + tileDataNumber * 16;
    }
}

}; // namespace emulator::gameboy
