//
// Created by Tristan Krause on 2026-07-24.
//

#pragma once

#include "types/json.hpp"

namespace setup
{
    struct NumParams
    {
        double system_wavelength{};
        std::size_t n_polar{};
        std::size_t n_azimuth{};
        std::size_t n_linear1{};
        std::size_t n_linear2{};
        double xtol_rel{};
        double ftol_rel{};

        [[nodiscard]] static NumParams create();
        [[nodiscard]] static NumParams create(NumParams const& num_params);
        void check() const;
    };

    NumParams constexpr DEFAULT_NUM_PARAMS = {.n_polar = 25, .n_azimuth = 50, .n_linear1 = 50, .n_linear2 = 50, .xtol_rel = 1e-8, .ftol_rel = 1e-8};

    template <AnyJson JsonType>
    void to_json(JsonType& js, NumParams const& num_params);

    template <AnyJson JsonType>
    void from_json(JsonType const& js, NumParams& num_params);
} // namespace setup
