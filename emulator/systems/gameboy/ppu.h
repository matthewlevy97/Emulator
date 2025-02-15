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
    struct Sprite {
        // 16 is subtracted from this value to determine the actual Y-Position
        std::uint8_t y;

        // 8 is subtracted from this value to determine the actual Y-Position
        std::uint8_t x;

        std::uint8_t tileNumber;

        /*
         * false = Sprite is always rendered above background
         * true = Background colors 1-3 overlay sprite, sprite is still rendered above color 0
         */
        bool objToBgPriority;

        // If set to 1 the sprite is flipped vertically, otherwise rendered as normal
        bool yFlip;

        // If set to 1 the sprite is flipped horizontally, otherwise rendered as normal
        bool xFlip;

        // If set to 0, the OBP0 register is used as the palette, otherwise OBP1
        bool palletNumber;

        Sprite() : Sprite(0, 0, 0, 0)
        {
        }

        Sprite(std::uint8_t y, std::uint8_t x, std::uint8_t tileNumber, std::uint8_t flags)
            : y(y), x(x), tileNumber(tileNumber),
              objToBgPriority((flags & 0x80) != 0),
              yFlip((flags & 0x40) != 0),
              xFlip((flags & 0x20) != 0),
              palletNumber((flags & 0x10) != 0)
        {
        }
    };

    class PixelFIFO
    {
    private:
        std::array<std::uint8_t, 10> buffer_;
        std::size_t index_{0};

    public:
        PixelFIFO() = default;

        bool Push(std::uint8_t pixel) noexcept
        {
            if (index_ == buffer_.size()) {
                return false;
            }

            buffer_[index_] = pixel;
            ++index_;
            return true;
        }

        std::uint8_t Pop() noexcept
        {
            auto sprite = buffer_[0];

            // Shift all elements to the left
            for (std::size_t i = 0; i < index_ - 1; ++i) {
                buffer_[i] = buffer_[i + 1];
            }
            --index_;

            return sprite;
        }

        bool Empty() const noexcept
        {
            return index_ == 0;
        }

        bool Full() const noexcept
        {
            return index_ == buffer_.size();
        }

        void Clear() noexcept
        {
            index_ = 0;
        }
    };

    PixelFIFO spriteFIFO_;
    PixelFIFO bgFIFO_;

    std::uint8_t SCY_, SCX_;
    std::uint8_t LY_, LX_;
    std::uint8_t WY_, WX_;

    bool ldcEnabled_{false};

    static const std::uint16_t kWindowTileMapArea0 = 0x9800;
    static const std::uint16_t kWindowTileMapArea1 = 0x9C00;

    // false = 0x9800-0x9BFF, true = 0x9C00-0x9FFF
    bool windowTileMapArea_{false};
    bool windowDisplayEnabled_{false};

    static const std::uint16_t kBGWindowTileDataArea0 = 0x8800;
    static const std::uint16_t kBGWindowTileDataArea1 = 0x8000;
    // false = 0x8800-0x97FF, true = 0x8000-0x8FFF
    bool bgWindowTileDataAddressingMode_{false};

    // false = 0x9800-0x9BFF, true = 0x9C00-0x9FFF
    bool bgTileMapArea_{false};

    // false = 8x8, true = 8x16
    bool spriteSize_{false};
    bool spriteDisplayEnabled_{false};
    bool bgDisplayEnabled_{false};

    PPUMode mode_;
    std::size_t tickTracker_;

    const Pixel kColorPaletteWhite_{0xFF, 0xFF, 0xFF, 0xFF};
    const Pixel kColorPaletteLightGray_{0x55, 0x55, 0x55, 0xFF};
    const Pixel kColorPaletteDarkGray_{0xAA, 0xAA, 0xAA, 0xFF};
    const Pixel kColorPaletteBlack_{0x00, 0x00, 0x00, 0xFF};
    Pixel colorPalette_[4] = {
        kColorPaletteBlack_,
        kColorPaletteDarkGray_,
        kColorPaletteLightGray_,
        kColorPaletteWhite_,
    };

    void handleOAM();
    void handlePixelTransfer();

    struct {
        std::size_t pixelTransferBackgroundTick{0};
        std::uint16_t tileMapAddress{0};
        std::uint8_t tileLine{0};
        std::uint8_t tileDataLow{0};
        std::uint8_t tileDataHigh{0};
    } pixelTransferBackgroundState_;
    void handlePixelTransferBackground() noexcept;
    void handleHBlank();
    void handleVBlank();

    std::uint16_t calculateTileMapAddress(std::uint16_t tileMapAddress) const noexcept;

    const Pixel& getDisplayPixelForColor(std::uint8_t color) const noexcept
    {
        return colorPalette_[color & 0b11];
    }

    std::uint8_t GetLCDCRegister() const noexcept
    {
        return (ldcEnabled_ << 7) |
               (windowTileMapArea_ << 6) |
               (windowDisplayEnabled_ << 5) |
               (bgWindowTileDataAddressingMode_ << 4) |
               (bgTileMapArea_ << 3) |
               (spriteSize_ << 2) |
               (spriteDisplayEnabled_ << 1) |
               bgDisplayEnabled_;
    }

    void SetLCDCRegister(std::uint8_t val) noexcept
    {
        ldcEnabled_ = (val & 0x80) != 0;
        windowTileMapArea_ = (val & 0x40) != 0;
        windowDisplayEnabled_ = (val & 0x20) != 0;
        bgWindowTileDataAddressingMode_ = (val & 0x10) != 0;
        bgTileMapArea_ = (val & 0x08) != 0;
        spriteSize_ = (val & 0x04) != 0;
        spriteDisplayEnabled_ = (val & 0x02) != 0;
        bgDisplayEnabled_ = (val & 0x01) != 0;
    }

public:
    PPU();

    void AttachToBus(emulator::component::Bus* bus) override;

    void ReceiveTick() override;

    void WriteUInt8(std::size_t address, std::uint8_t value) override
    {
        if (address == 0xFF42) {
            SCY_ = value;
        } else if (address == 0xFF43) {
            SCX_ = value;
        } else if (address == 0xFF44) {
            /*
             * LY indicates the current horizontal line, which might be about to be drawn, being drawn, or
             * just been drawn. LY can hold any value from 0 to 153, with values from 144 to 153 indicating
             * the VBlank period.
             */
            LY_ = value;
        } else if (address == 0xFF45) {
            LY_ = value;
        } else if (address == 0xFF47) {
            // This register assigns gray shades to the color IDs of the BG and Window tiles
            for (std::size_t i = 0; i < 4; ++i) {
                auto id = (value >> (i * 2)) & 0b11;
                switch (id) {
                case 0:
                    colorPalette_[i] = kColorPaletteWhite_;
                    break;
                case 1:
                    colorPalette_[i] = kColorPaletteLightGray_;
                    break;
                case 2:
                    colorPalette_[i] = kColorPaletteDarkGray_;
                    break;
                case 3:
                    colorPalette_[i] = kColorPaletteBlack_;
                    break;
                }
            }

        } else if (address == 0xFF4A) {
            WY_ = value;
        } else if (address == 0xFF4B) {
            WX_ = value;
        } else {
            spdlog::error("PPU: Write to unhandled address: 0x{0:X}", address);
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
        } else if (address == 0xFF4A) {
            return WY_;
        } else if (address == 0xFF4B) {
            return WX_;
        } else {
            spdlog::error("PPU: Read from unhandled address: {0:x}", address);
        }
        return 0;
    }
};

}; // namespace emulator::gameboy
