//
// Created by Tristan Krause on 2026-07-24.
//

#pragma once

#include "types/json.hpp"

namespace setup
{
    struct SimParams
    {
        double system_wavelength = 0; /// required as a basic length reference, does *not* need to match the actual simulation wavelength (frequency)
        bool enable_path_loss = true; /// If true, includes the 1/r free-space path loss attenuation in the spherical wave propagation model
        std::size_t n_polar = 25; /// number of polar integration / differentiation samples
        std::size_t n_azimuth = 50; /// number of azimuthal integration / differentiation samples
        std::size_t n_linear1 = 50; /// number of linear integration / differentiation samples in the first dimension
        std::size_t n_linear2 = 50; /// number of linear integration / differentiation samples in the second dimension
        double xtol_rel = 1e-8; /// stop condition for optimizer: relative tolerance for the argument
        double ftol_rel = 1e-8; /// stop condition for optimizer: relative tolerance for the value of the objective function

        void assert_integrity() const;
    };

    template <AnyJson JsonType>
    void to_json(JsonType& js, SimParams const& sim_params);

    template <AnyJson JsonType>
    void from_json(JsonType const& js, SimParams& sim_params);
} // namespace setup
