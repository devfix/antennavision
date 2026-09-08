//
// Created by Tristan Krause on 2026-08-17.
//

#pragma once

#include <array>
#include <complex>
#include <memory>
#include <vector>
#include "types/json.hpp"

namespace antennavision::codebooks
{
    /**
     * @brief ULA_SFC codebook.
     *
     * Expects a "weights" array at the root, structured as nested arrays:
     *
     *   weights := [ [tilde_r, arc], [tilde_r, arc], ... ]
     *   arc     := [ [tilde_theta, vector], [tilde_theta, vector], ... ]
     *   vector  := [ [abs, phase], [abs, phase], ... ]   // one [abs, phase] pair per ULA element
     *
     * tilde_r values must be sorted ascending across the outer array, and
     * tilde_theta values must be sorted ascending within each inner arc array.
     * Each vector must contain exactly N pairs, where N is the number of ULA
     * elements; abs/phase pairs are converted to std::complex<double> via
     * math::complex_from_polar.
     *
     * Example for a ULA of size N = 2 (annotations added for clarity; JSON
     * itself does not support comments):
     *
     *   {
     *     "weights": [
     *       [
     *         0.0200,                          // tilde_r for this radial slice (= lambda / r_p)
     *         [
     *           [
     *             -0.9,                        // tilde_theta for this angular slice (= cos(theta_p))
     *             [
     *               [1.0, 0.0],                // element 0: [abs, phase] -> phase-only, abs == 1.0
     *               [1.0, 0.35]                // element 1: [abs, phase]
     *             ]
     *           ],
     *           [
     *             -0.7,                        // next tilde_theta (ascending) within this radial slice
     *             [
     *               [1.0, 0.0],                // element 0
     *               [1.0, 0.41]                // element 1
     *             ]
     *           ]
     *         ]
     *       ],
     *       [
     *         0.0250,                          // next tilde_r (ascending)
     *         [
     *           [-0.9, [[1.0, 0.0], [0.99, 0.30]]],
     *           [-0.7, [[1.0, 0.0], [0.97, 0.38]]]
     *         ]
     *       ]
     *     ]
     *   }
     *
     * @param js JSON object containing the "weights" array as described above.
     * @return   ULA_SFC populated with the parsed codebook.
     */
    struct ULA_SFC
    {
        using cb_vector = std::vector<std::complex<double>>;

        [[nodiscard]] static ULA_SFC from_json(json const& js);

        [[nodiscard]] static ULA_SFC from_file(std::filesystem::path const& p);

        /**
         *
         * @param r_rel relative distance, r_rel = r/wavelength
         * @param polar polar angle in the xy-plane
         * @return closest SFC CB vector
         */
        cb_vector const& get_vector(double r_rel, double polar);

        std::size_t ula_size;
        std::vector<std::pair<double, std::vector<std::pair<double, cb_vector>>>> codebook; /// first index: tilde(r)_p, second index: tilde(theta)_p
    };
} // namespace antennavision::codebooks
