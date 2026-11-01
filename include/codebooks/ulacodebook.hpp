//
// Created by core on 31.10.26.
//

#pragma once

#include <span>
#include "../types/math.hpp"

namespace antennavision::codebooks
{
    struct UlaCodebook
    {
        virtual ~UlaCodebook() = default;
        [[nodiscard]] virtual double wavelength() const = 0;
        [[nodiscard]] virtual std::size_t n_elements() const = 0; /// returns the number of ULA elements
        [[nodiscard]] virtual std::size_t n_sectors() const = 0; /// returns the number of sectors
        [[nodiscard]] virtual std::span<const Complex> get_vector(double tas, double trs) = 0;
        [[nodiscard]] virtual std::vector<std::pair<double, double>> get_sector_centers() = 0; /// returns the TTS coordinates of all sector centers
    };
}
