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
     * Expects "ula_size" and a "codebook" array at the root, structured as
     * nested arrays. The outer array runs over a fixed, uniformly-spaced
     * angular grid; each entry ("column") then holds an ascending, but
     * generally non-uniformly-spaced, list of radial entries:
     *
     *   codebook := [ [tilde_theta, column], [tilde_theta, column], ... ]
     *   column   := [ [tilde_r, vector], [tilde_r, vector], ... ]
     *   vector   := [ [re, im], [re, im], ... ]   // one [re, im] pair per ULA element
     *
     * tilde_theta values must be sorted ascending across the outer array
     * (fixed grid spacing), and tilde_r values must be sorted ascending
     * within each inner column array (non-uniform spacing). Each vector must
     * contain exactly N pairs, where N is the number of ULA elements;
     * re/im pairs are converted directly to std::complex<double>.
     *
     * Example for a ULA of size N = 2 (annotations added for clarity; JSON
     * itself does not support comments):
     *
     *   {
     *     "ula_size": 2,
     *     "codebook": [
     *       [
     *         -0.9,                            // tilde_theta for this angular column (= cos(theta_p))
     *         [
     *           [
     *             0.0200,                      // tilde_r for this radial slice (= lambda / r_p)
     *             [
     *               [1.0, 0.0],                // element 0: [re, im]
     *               [0.94, 0.34]               // element 1: [re, im]
     *             ]
     *           ],
     *           [
     *             0.0250,                      // next tilde_r (ascending) within this angular column
     *             [
     *               [1.0, 0.0],                // element 0
     *               [0.95, 0.29]               // element 1
     *             ]
     *           ]
     *         ]
     *       ],
     *       [
     *         -0.7,                            // next tilde_theta (ascending)
     *         [
     *           [0.0200, [[1.0, 0.0], [0.99, 0.30]]],
     *           [0.0250, [[1.0, 0.0], [0.97, 0.38]]]
     *         ]
     *       ]
     *     ]
     *   }
     *
     * @param js JSON object containing "ula_size" and the "codebook" array as described above.
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
        std::vector<std::pair<double, std::vector<std::pair<double, cb_vector>>>> codebook; /// first index: tilde(theta)_p, second index: tilde(r)_p
    };
} // namespace antennavision::codebooks