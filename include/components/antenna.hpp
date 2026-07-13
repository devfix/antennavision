//
// Created by core on 2026-07-08.
//

#pragma once

#include <variant>
#include <magic_enum/magic_enum.hpp>
#include "components/uniformlineararray.hpp"
#include "components/uniformplanararray.hpp"
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
    template <typename T>
    concept IsAntenna = std::same_as<std::decay_t<T>, Antenna>;

    constexpr std::string_view get_type_name(const Antenna& ant)
    {
        auto active_enum = static_cast<AntennaType>(ant.index());
        return magic_enum::enum_name(active_enum);
    }

    constexpr std::string_view get_id(Antenna const& antenna)
    {
        return std::visit([](auto const& ant) -> std::string_view { return ant.id; }, antenna);
    }

    constexpr Reference& get_origin(Antenna& antenna)
    {
        return std::visit([](auto const& ant) -> Reference& { return ant.origin; }, antenna);
    }

    template <typename T, IsAntenna A>
    constexpr decltype(auto) cast(A& antenna)
    {
        if (auto specified = std::get_if<T>(&antenna); specified) { return *specified; }
        throw SimulationError("Antenna cast failed: {} has type {}, but {} was requested", static_cast<const void*>(&antenna), get_type_name(antenna),
                              magic_enum::enum_name(static_cast<AntennaType>(get_variant_index<T, Antenna>())));
    }
} // namespace antenna
