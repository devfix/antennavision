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
        std::string sweep_id;

        [[nodiscard]] std::string id() const { return std::format("{}.{}.{}", name, antenna_id, sweep_id); }
    };

    struct RxVoltageFieldAtWavelength
    {
        static constexpr std::string_view name = "RxVoltageField@Wavelength";
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
            return std::format("{}.{}.{}.{}.{}.{}.{}", name, ref_id.empty() ? "origin" : ref_id, tx_id, rx_id, geo_id, n_dim1, n_dim2, sweep_wavelength_id);
        }
    };

    using Task = std::variant<DirectivityOverPolarAtAzimuth, RxVoltageFieldAtWavelength>;

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
