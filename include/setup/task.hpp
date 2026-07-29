//
// Created by Tristan Krause on 2026-07-28.
//

#pragma once
#include "geometry.hpp"
#include "sweep.hpp"

#include <string_view>

namespace setup::task
{
    struct DirectivityOverPolarSweepAzimuth
    {
        static constexpr std::string_view name = "DirectivityOverPolarSweepAzimuth";
        std::string antenna_id;
        std::string sweep_azimuth_id;

        [[nodiscard]] std::string id() const { return std::format("{}.{}.{}", name, antenna_id, sweep_azimuth_id); }
    };

    struct RxVoltageFieldAtWavelength
    {
        static constexpr std::string_view name = "RxVoltageFieldAtWavelength";
        std::string ref_id;
        std::string tx_id;
        std::string rx_id;
        std::string geo_id;
        std::string sweep_wavelength_id;

        [[nodiscard]] std::string id() const { return std::format("{}.{}.{}.{}.{}", name, ref_id.empty() ? "origin" : ref_id, tx_id, rx_id, geo_id, sweep_wavelength_id); }
    };

    using Task = std::variant<DirectivityOverPolarSweepAzimuth, RxVoltageFieldAtWavelength>;

    [[nodiscard]] constexpr std::string_view get_name(Task const& task) noexcept
    {
        return std::visit([](auto const& t) -> std::string_view { return t.name; }, task);
    }

    [[nodiscard]] constexpr std::string get_id(Task const& task) noexcept
    {
        return std::visit([](auto const& t) -> std::string { return t.id(); }, task);
    }
} // namespace setup::task
