//
// Created by core on 07.07.26.
//

#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <vector>
#include "types.hpp"

struct UlaCodebook
{
    struct Entry
    {
        double angle;
        double focus_distance;
        std::vector<complex_t> weights;
    };

    explicit UlaCodebook(std::filesystem::path const& p);
    std::vector<complex_t> get_steering_vector(double wavelength, double angle, double focus_distance);

    std::uint32_t n_elements;
    std::uint32_t oversampling_factor;
    double distance;
    std::vector<double> wavelengths;
    std::map<std::uint32_t, std::map<std::uint32_t, Entry>> codebook;
};
