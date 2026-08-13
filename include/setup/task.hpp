//
// Created by Tristan Krause on 2026-07-28.
//

#pragma once
#include "geometry.hpp"
#include "sweep.hpp"

#include <string_view>

namespace setup::task
{
    struct DirectivityOverPolarAtAzimuth
    {
        static constexpr std::string_view name = "DirectivityOverPolar@Azimuth";
        std::string path_output;
        std::string antenna_id;
        double wavelength;
        std::string sweep_id;

        [[nodiscard]] std::string id() const { return std::format("{}.{}.{}", name, antenna_id, sweep_id); }
    };

    struct VoltGainOverPoints
    {
        static constexpr std::string_view name = "VoltGain(Points)";
        std::string path_output;
        std::string ref_id;
        std::string tx_id;
        std::string rx_id;
        std::vector<std::string> tx_codebook;
        std::vector<std::string> rx_codebook;
        std::vector<Pos> points;
        double wavelength;

        [[nodiscard]] std::string id() const
        { //
            return std::format("{}.{}.{}.{}.{}.{:06.0f}", name, ref_id.empty() ? "<global origin>" : ref_id, tx_id, rx_id, points.size(), wavelength * 1e6);
        }
    };

    struct VoltGainOverGeometry
    {
        static constexpr std::string_view name = "VoltGain(Geometry)";
        std::string path_output;
        std::string ref_id;
        std::string tx_id;
        std::string rx_id;
        std::vector<std::string> tx_codebook;
        std::vector<std::string> rx_codebook;
        std::string geo_id;
        std::size_t n_dim1;
        std::size_t n_dim2;
        double wavelength;

        [[nodiscard]] std::string id() const
        { //
            return std::format("{}.{}.{}.{}.{}.{}.{}.{:06.0f}", name, ref_id.empty() ? "<global origin>" : ref_id, tx_id, rx_id, geo_id, n_dim1, n_dim2, wavelength * 1e6);
        }
    };

    struct VoltGainOverGeometryAtWavelength
    {
        static constexpr std::string_view name = "VoltGain(Geometry)@Wavelength";
        std::string path_output;
        std::string ref_id;
        std::string tx_id;
        std::string rx_id;
        std::vector<std::string> tx_codebook;
        std::vector<std::string> rx_codebook;
        std::string geo_id;
        std::size_t n_dim1;
        std::size_t n_dim2;
        std::string sweep_wavelength_id;

        [[nodiscard]] std::string id() const
        { //
            return std::format("{}.{}.{}.{}.{}.{}.{}", name, ref_id.empty() ? "<global origin>" : ref_id, tx_id, rx_id, geo_id, n_dim1, n_dim2, sweep_wavelength_id);
        }
    };

    struct VoltGainPeakAndCutoffs
    {
        static constexpr std::string_view name = "VoltGain.PeakAndCutoffs";
        std::string path_output;
        std::string ref_id;
        std::string tx_id;
        std::string rx_id;
        std::vector<std::string> tx_codebook;
        std::vector<std::string> rx_codebook;
        std::string curve_id;
        std::size_t n_scan;
        double ratio;
        double wavelength;

        [[nodiscard]] std::string id() const
        { //
            return std::format("{}.{}.{}.{}.{}.{}.{:03.0f}.{:06.0f}", name, ref_id.empty() ? "<global origin>" : ref_id, tx_id, rx_id, curve_id, n_scan, ratio*1e3, wavelength * 1e6);
        }
    };

    using Task = std::variant<DirectivityOverPolarAtAzimuth, VoltGainOverPoints, VoltGainOverGeometry, VoltGainOverGeometryAtWavelength, VoltGainPeakAndCutoffs>;

    template <AnyJson JsonType>
    void to_json(JsonType& js, Task const& task);

    template <AnyJson JsonType>
    void from_json(JsonType const& js, Task& task);

    [[nodiscard]] constexpr std::string_view get_name(Task const& task) noexcept
    {
        return task.visit([](auto const& t) -> std::string_view { return t.name; });
    }

    [[nodiscard]] constexpr std::string const& get_output_path(Task const& task) noexcept
    {
        return task.visit([](auto const& t) -> std::string const& { return t.path_output; });
    }

    [[nodiscard]] constexpr std::string get_id(Task const& task) noexcept
    {
        return task.visit([](auto const& t) -> std::string { return t.id(); });
    }
} // namespace setup::task
