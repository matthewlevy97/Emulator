#pragma once

#include <vector>

#include "component.h"

namespace emulator::component
{

class Display : public IComponent
{
public:
    struct Pixel {
        std::uint8_t r;
        std::uint8_t g;
        std::uint8_t b;
        std::uint8_t a;

        Pixel() : r(0), g(0), b(0), a(0) {}
        Pixel(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) : r(r), g(g), b(b), a(a) {}
        Pixel(const Pixel& other) : r(other.r), g(other.g), b(other.b), a(other.a) {}
    };

private:
    void ValidatePixelPosition(std::size_t x, std::size_t y) const
    {
        if (x >= width_ || y >= height_) {
            throw std::out_of_range("Pixel position out of range for display");
        }
    }

protected:
    std::size_t width_{0}, height_{0};

    std::vector<Pixel> pixels_;

public:
    Display(std::size_t width, std::size_t height)
        : IComponent(IComponent::ComponentType::Display),
          width_(width), height_(height)
    {
        pixels_.resize(width_ * height_);
    }

    void ReceiveTick() override
    {
    }

    void PowerOn() noexcept override
    {
    }

    void PowerOff() noexcept override
    {
    }

    std::size_t GetWidth() const noexcept
    {
        return width_;
    }

    std::size_t GetHeight() const noexcept
    {
        return height_;
    }

    const Pixel& GetPixel(std::size_t x, std::size_t y) const
    {
        ValidatePixelPosition(x, y);
        return pixels_[y * width_ + x];
    }

    void SetPixel(std::size_t x, std::size_t y, const Pixel& pixel)
    {
        ValidatePixelPosition(x, y);
        pixels_[y * width_ + x] = pixel;
    }

    void SetPixel(std::size_t x, std::size_t y, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a)
    {
        ValidatePixelPosition(x, y);
        pixels_[y * width_ + x] = Pixel(r, g, b, a);
    }

    std::uint32_t* GetPixelData(std::size_t& pixelDataWidth, std::size_t& pixelDataHeight) const noexcept
    {
        auto pixels = new std::uint32_t[width_ * height_];

        for (std::size_t i = 0; i < width_ * height_; ++i) {
            auto& pixel = pixels_[i];
            pixels[i] = (pixel.r << 24) | (pixel.g << 16) | (pixel.b << 8) | pixel.a;
        }

        pixelDataWidth = width_;
        pixelDataHeight = height_;
        return pixels;
    }
};

}; // namespace emulator::component
