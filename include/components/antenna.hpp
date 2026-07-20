//
// Created by core on 2026-07-08.
//

#pragma once

#include <magic_enum/magic_enum.hpp>
#include <variant>
#include "components/uniformlineararray.hpp"
#include "components/uniformplanararray.hpp"
#include "scalarfield.hpp"
#include "simulationerror.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
using Antenna = std::variant<Radiator, UniformLinearArray, UniformPlanarArray>;
enum struct AntennaType // must be same order as in Antenna
{
    Radiator,
    UniformLinearArray,
    UniformPlanarArray
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, typename Variant>
constexpr std::size_t get_variant_index()
{
    return []<typename... Types>(std::variant<Types...>*)
    {
        constexpr std::array<bool, sizeof...(Types)> matches = {std::is_same_v<T, Types>...};

        for (std::size_t i = 0; i < matches.size(); ++i)
        {
            if (matches[i]) { return i; }
        }
        throw "Type not found in variant!";
    }(static_cast<Variant*>(nullptr));
}

namespace antenna
{
    template <typename R>
    concept AntennaContainer = std::ranges::range<R> && std::same_as<std::ranges::range_value_t<R>, Antenna>;

    template <typename R>
    concept RadiatorContainer = std::ranges::range<R> && std::same_as<std::ranges::range_value_t<R>, Radiator>;

    template <typename T>
    concept IsAntenna = std::same_as<std::decay_t<T>, Antenna>;

    template <typename T>
    constexpr bool is_array = std::disjunction_v<
        std::is_same<std::decay_t<T>, UniformLinearArray>,
        std::is_same<std::decay_t<T>, UniformPlanarArray>
    >;

    constexpr std::string_view get_type_name(const Antenna& ant)
    {
        auto active_enum = static_cast<AntennaType>(ant.index());
        return magic_enum::enum_name(active_enum);
    }

    constexpr std::string const& id(Antenna const& antenna)
    {
        return std::visit([](auto& ant) -> std::string const& { return ant.id; }, antenna);
    }

    constexpr Reference* origin(Antenna& antenna)
    {
        return std::visit([](auto& ant) -> Reference* { return ant.origin; }, antenna);
    }

    template <typename T, IsAntenna A>
    constexpr decltype(auto) cast(A& antenna)
    {
        if (auto specified = std::get_if<T>(&antenna); specified) { return *specified; }
        throw SimulationError("Antenna cast failed: {} has type {}, but {} was requested", static_cast<const void*>(&antenna), get_type_name(antenna),
            magic_enum::enum_name(static_cast<AntennaType>(get_variant_index<T, Antenna>())));
    }

    [[nodiscard]] complex_t calc_voltage_gain(Antenna const& tx, Antenna const& rx, math::NumParams const& num_params);
    [[nodiscard]] double calc_power_gain(Antenna const& tx, Antenna const& rx, math::NumParams const& num_params);

    /**
     * Creates new scalar field that is the voltage field if the tx is fixed in space and the rx is moved around
     * @param tx transmitter antenna
     * @param rx receiver antenna
     * @param num_params numerical parameters
     * @return voltage field between tx and rx
     */
    [[nodiscard]] ScalarField<complex_t> get_voltage_field(Antenna const& tx, Antenna& rx, math::NumParams const& num_params);

    /**
     * Creates new scalar field that is the power field if the tx is fixed in space and the rx is moved around
     * @param tx transmitter antenna
     * @param rx receiver antenna
     * @param num_params numerical parameters
     * @return power field between tx and rx
     */
    [[nodiscard]] ScalarField<double> get_power_field(Antenna const& tx, Antenna& rx, math::NumParams const& num_params);

    void resolve_origins(AntennaContainer auto& antennas, ReferenceContainer auto& references);
    void resolve_origins(RadiatorContainer auto& radiators, ReferenceContainer auto& references);
    [[nodiscard]] Antenna& get(AntennaContainer auto& antennas, std::string const& id);

} // namespace antenna

void antenna::resolve_origins(AntennaContainer auto& antennas, ReferenceContainer auto& references)
{
    for (Antenna& ant : antennas)
    {
        std::string const& origin_id = std::visit([](auto& a) -> std::string const& { return a.origin_id; }, ant);
        Reference** origin = std::visit([](auto& a) -> Reference** { return &a.origin; }, ant);
        auto const it = std::ranges::find(references, origin_id, &Reference::id);
        if (it == references.end()) { throw SimulationError("Antenna '{}' has non-existing origin '{}'", id(ant), origin_id); }
        *origin = std::to_address(it);
    }
}

void antenna::resolve_origins(RadiatorContainer auto& radiators, ReferenceContainer auto& references)
{
    for (Radiator& rad : radiators)
    {
        auto const it = std::ranges::find(references, rad.origin_id, &Reference::id);
        if (it == references.end()) { throw SimulationError("Antenna '{}' has non-existing origin '{}'", id(rad), rad.origin_id); }
        rad.origin = std::to_address(it);
    }
}

Antenna& antenna::get(AntennaContainer auto& antennas, std::string const& id)
{
    auto const it = std::ranges::find(antennas, id, [](auto& ant) { return std::visit([](auto& a) { return a.id; }, ant); });
    if (it == antennas.end()) { throw SimulationError("Could not find antenna with id '{}'", id); }
    return *it;
}