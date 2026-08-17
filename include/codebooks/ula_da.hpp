//
// Created by Tristan Krause on 2026-08-17.
//

#pragma once

#include <vector>
#include <array>
#include <memory>
#include <complex>
#include "types/json.hpp"

namespace antennavision::codebooks
{
    struct ULA_DA
    {
        using polar_pos = std::array<double, 2>;
        using cb_vector = std::vector<std::complex<double>>;

        // Constructor
        [[nodiscard]] static ULA_DA from_vector(const std::vector<std::pair<polar_pos, cb_vector>>& raw_codebook);

        [[nodiscard]] static ULA_DA from_json(json const& js);

        [[nodiscard]] static ULA_DA from_file(std::filesystem::path const& p);

        // Destructor MUST be declared here, but defined in the .cpp file
        ~ULA_DA();

        // Query function
        [[nodiscard]] std::pair<double, cb_vector> find_closest(const polar_pos& target_polar) const;

    private:
        // Forward declare the implementation struct
        struct Impl;

        ULA_DA(std::unique_ptr<Impl> pimpl);

        // Opaque pointer to the implementation
        std::unique_ptr<Impl> pimpl;
    };
} // antennavision::codebooks
