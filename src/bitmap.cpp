//
// Created by Tristan Krause on 2026-04-30.
//

#include "../include/bitmap.hpp"
#include <fstream>

Bitmap::Bitmap(std::uint16_t width, std::uint16_t height) :
    width_(width), height_(height), data{static_cast<std::size_t>(width) * static_cast<std::size_t>(height), {0,0,0}}
{
    for (std::uint16_t y = 0; y < height_; ++y) {
        for (std::uint16_t x = 0; x < width_; ++x) {
            (*this)[x, y] = {128,  static_cast<uint8_t>((y * 255) / height_), static_cast<uint8_t>((x * 255) / width_)};
        }
    }
}

Bitmap::bgr_t& Bitmap::operator[](std::uint16_t const x, std::uint16_t const y) {
    return data[y * width_ + x];
}

const Bitmap::bgr_t& Bitmap::operator[](std::uint16_t const x, std::uint16_t const y) const {
    return data[y * width_ + x];
}

void Bitmap::write(const std::filesystem::path& path)
{
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) return;

    // 1. Calculate Padding
    int paddingSize = (4 - (width_ * 3) % 4) % 4;
    uint32_t file_size = 54 + (width_ * 3 + paddingSize) * height_;

    // 2. Setup Headers
    FileHeader file_header;
    file_header.file_size = file_size;

    InfoHeader infoHeader;
    infoHeader.width = width_;
    infoHeader.height = height_;

    // 3. Write Headers
    ofs.write(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    ofs.write(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

    // 4. Write Pixel Data (Bottom-to-top, Blue-Green-Red)
    uint8_t padding[3] = {0, 0, 0};

    for (std::uint16_t y = 0; y < height_; ++y) {
        for (std::uint16_t x = 0; x < width_; ++x) {
            ofs.put(encode_value<0>((*this)[x, y]));
            ofs.put(encode_value<1>((*this)[x, y]));
            ofs.put(encode_value<2>((*this)[x, y]));
        }
        // Write padding bytes at the end of the row
        ofs.write(reinterpret_cast<char*>(padding), paddingSize);
    }
}

Bitmap::bgr_t Bitmap::get_min() const
{
    auto const it_b = std::ranges::min_element(data, {}, [](auto const& t) { return std::get<0>(t); });
    auto const it_g = std::ranges::min_element(data, {}, [](auto const& t) { return std::get<1>(t); });
    auto const it_r = std::ranges::min_element(data, {}, [](auto const& t) { return std::get<2>(t); });
    return { std::get<0>(*it_b), std::get<1>(*it_g), std::get<2>(*it_r) };
}

Bitmap::bgr_t Bitmap::get_max() const
{
    auto const it_b = std::ranges::max_element(data, {}, [](auto const& t) { return std::get<0>(t); });
    auto const it_g = std::ranges::max_element(data, {}, [](auto const& t) { return std::get<1>(t); });
    auto const it_r = std::ranges::max_element(data, {}, [](auto const& t) { return std::get<2>(t); });
    return { std::get<0>(*it_b), std::get<1>(*it_g), std::get<2>(*it_r) };
}

void Bitmap::clamp(double min, double max)
{
    for (auto& [b, g, r] : data) {
        b = std::clamp(b, min, max);
        g = std::clamp(g, min, max);
        r = std::clamp(r, min, max);
    }
}


void Bitmap::normalize()
{
    auto const [b_min, g_min, r_min] = get_min();
    auto const [b_max, g_max, r_max] = get_max();
    auto const b_factor = 255.0/(b_max-b_min);
    auto const g_factor = 255.0/(g_max-g_min);
    auto const r_factor = 255.0/(r_max-r_min);
    for (auto& [b, g, r] : data) {
        b = (b - b_min) * b_factor;
        g = (g - g_min) * g_factor;
        r = (r - r_min) * r_factor;
    }
}
