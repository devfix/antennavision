//
// Created by Tristan Krause on 2026-07-28.
//

#pragma once
#include "geometry.hpp"
#include "sweep.hpp"

#include <string_view>

namespace setup::task
{
    namespace detail
    {
        // TODO this meta function should be replaced by the std meta library as soon as it is available
        template <typename T>
        constexpr std::string_view type_name()
        {
#if defined(__clang__) || defined(__GNUC__)
            std::string_view name = __PRETTY_FUNCTION__;
            auto start = name.find("T = ") + 4;
            auto end = name.find_last_of(']');
            return name.substr(start, end - start);
#elif defined(_MSC_VER)
            std::string_view name = __FUNCSIG__;
            auto start = name.find("type_name<") + 10;
            auto end = name.find_last_of('>');
            // Strip "struct " prefix if present on MSVC
            auto sub = name.substr(start, end - start);
            if (sub.starts_with("struct ")) return sub.substr(7);
            if (sub.starts_with("class ")) return sub.substr(6);
            return sub;
#endif
        }
    } // namespace detail

    template <typename Derived>
    struct TaskBase
    {
        std::string_view static constexpr name = detail::type_name<Derived>();

        std::string id() const { return reinterpret_cast<Derived*>(this)->id(); }
    };

    struct DirectivityOverPolarSweepAzimuth : TaskBase<DirectivityOverPolarSweepAzimuth>
    {
        std::string antenna_id;
        std::string sweep_azimuth_id;

        std::string id() const { return std::format("{}.{}.{}", name, antenna_id, sweep_azimuth_id); }
    };

    struct RxVoltageFieldSweepWavelength : TaskBase<RxVoltageFieldSweepWavelength>
    {
        std::string tx_id;
        std::string rx_id;
        std::string geo_id;
        std::string sweep_wavelength_id;

        std::string id() const { return std::format("{}.{}.{}.{}", tx_id, rx_id, geo_id, sweep_wavelength_id); }
    };

    using Task = std::variant<DirectivityOverPolarSweepAzimuth, RxVoltageFieldSweepWavelength>;

    [[nodiscard]] constexpr std::string_view get_name(Task const& task) noexcept
    {
        return std::visit([](auto const& t) -> std::string_view { return t.name; }, task);
    }

    [[nodiscard]] constexpr std::string get_id(Task const& task) noexcept
    {
        return std::visit([](auto const& t) -> std::string { return t.id(); }, task);
    }
} // namespace setup::task
