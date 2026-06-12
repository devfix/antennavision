//
// Created by Tristan Krause on 2026-04-30.
//


#pragma once

#include <cstdint>
#include <cmath>
#include <vector>
#include <algorithm>
#include <filesystem>


struct Bitmap
{

#pragma pack(push, 1)
    struct FileHeader
    {
        std::uint16_t file_type{0x4D42}; // "BM"
        std::uint32_t file_size{0}; // Size of file in bytes
        std::uint16_t reserved1{0};
        std::uint16_t reserved2{0};
        std::uint32_t offset_data{54}; // Start of pixel data
    };

    struct InfoHeader
    {
        std::uint32_t size{40}; // Size of this header
        std::int32_t width{0};
        std::int32_t height{0};
        std::uint16_t planes{1}; // Must be 1
        std::uint16_t bit_count{24}; // 24-bit (RGB)
        std::uint32_t compression{0}; // No compression
        std::uint32_t size_image{0}; // 0 for uncompressed
        std::int32_t x_pixels_per_meter{0};
        std::int32_t y_pixels_per_meter{0};
        std::uint32_t colors_used{0};
        std::uint32_t colors_important{0};
    };
#pragma pack(pop)

    using bgr_t = std::tuple<double, double, double>;

    Bitmap(std::uint16_t width, std::uint16_t height);
    bgr_t &operator[](std::uint16_t x, std::uint16_t y);
    const bgr_t &operator[](std::uint16_t x, std::uint16_t y) const;
    void write(const std::filesystem::path& path);
    bgr_t get_min() const;
    bgr_t get_max() const;
    void clamp(double min, double max);
    void normalize();

    std::vector<bgr_t> data;

private:
    template<std::size_t n>
    [[nodiscard]] constexpr char encode_value(Bitmap::bgr_t bgr) const
    {
        float val = std::round(std::get<n>(bgr));
        return static_cast<char>(std::clamp(static_cast<std::int64_t>(val), 0l, 255l));
    }

    std::uint16_t const width_;
    std::uint16_t const height_;
};
