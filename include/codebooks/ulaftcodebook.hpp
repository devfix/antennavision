//
// Created by core on 31.10.26.
//

#pragma once

#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "ulacodebook.hpp"

namespace antennavision::codebooks
{
    /// Codebook of a ULA with a fixed transverse grid (tas) and, for every transverse position, a radial grid (trs) in
    /// the TTS, loaded from the JSON file written by the Python class UlaFtCodebook.
    ///
    /// JSON layout (all keys are required, unknown keys are ignored):
    ///   { "description": <text>, "n_elements": <N>, "wavelength": <design wavelength in m>,
    ///     "relative_spacing": <d / wavelength>, "power_ratio": <rho>, "amplitude_ratio": <kappa0>,
    ///     "n_sectors": <number of vectors>,
    ///     "weights": [ [ tas, [ [ trs, [ [re, im], ... N elements ... ] ], ... ] ], ... ] }
    ///
    /// The columns must be sorted by ascending tas and the rows of each column by ascending trs (as written by the
    /// script). get_vector() snaps a query to the codebook: first the column with the nearest tas is selected, then the
    /// row of that column with the nearest trs. Queries outside the grid are clamped to its border.
    struct UlaFtCodebook final : public UlaCodebook
    {
    public:
        /// loads and validates the codebook, throws std::runtime_error on I/O, syntax or consistency errors
        explicit UlaFtCodebook(const std::filesystem::path& path);

        /// weights of the focal point nearest to (tas, trs). The span stays valid until the codebook is destroyed.
        /// Throws std::invalid_argument if tas or trs is NaN.
        std::span<const Complex> get_vector(double tas, double trs) override;
        /// (tas, trs) of every focal point of the codebook, i.e. the centers of the sectors that get_vector() snaps to.
        /// The order is column by column (ascending tas), within a column by ascending trs, i.e. the vector order of
        /// the file. Querying get_vector() with a returned pair yields exactly the vector of that point.
        std::vector<std::pair<double, double>> get_sector_centers() override;

        // members of the Python data class
        [[nodiscard]] const std::string& description() const; /// free text description of the codebook
        [[nodiscard]] std::size_t n_elements() const override; /// number of ULA elements N
        [[nodiscard]] double wavelength() const override; /// design wavelength in m
        [[nodiscard]] double relative_spacing() const; /// ULA element spacing divided by the design wavelength
        [[nodiscard]] double power_ratio() const; /// cut-off power ratio rho the codebook was designed for
        [[nodiscard]] double amplitude_ratio() const; /// amplitude ratio kappa0 of the uniform-power distance
        [[nodiscard]] std::size_t n_sectors() const override; /// total number of sectors, equals the number of vectors

        [[nodiscard]] std::size_t column_count() const; /// number of transverse positions
        /// (tas, trs) of the focal point that get_vector() returns for the query
        [[nodiscard]] std::pair<double, double> nearest_point(double tas, double trs) const;

    private:
        struct Column
        {
            double tas;
            std::size_t first; /// index of the first vector (and its trs_ entry) of this column
            std::size_t count; /// number of vectors (rows) in this column
        };

        [[nodiscard]] std::size_t nearest_index(double tas, double trs) const;

        std::string description_;
        std::size_t n_elements_ = 0;
        double wavelength_ = 0;
        double relative_spacing_ = 0;
        double power_ratio_ = 0;
        double amplitude_ratio_ = 0;
        std::size_t n_sectors_ = 0;

        // the weights of the Python class, flattened for fast lookup
        std::vector<Column> columns_;
        std::vector<double> trs_; /// radial position of every vector, grouped by column
        std::vector<Complex> weights_; /// vector i occupies [i * n_elements_, (i + 1) * n_elements_)
    };
}