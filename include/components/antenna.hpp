//
// Created by Tristan Krause on 2026-07-08.
//

#pragma once

#include <magic_enum/magic_enum.hpp>
#include <span>
#include <variant>
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

    template <typename T>
    constexpr bool is_array = std::disjunction_v<std::is_same<std::decay_t<T>, UniformLinearArray>, std::is_same<std::decay_t<T>, UniformPlanarArray>>;

    [[nodiscard]] constexpr std::string_view get_type_name(const Antenna& ant)
    {
        auto active_enum = static_cast<AntennaType>(ant.index());
        return magic_enum::enum_name(active_enum);
    }

    [[nodiscard]] constexpr std::string const& get_id(Antenna const& antenna)
    {
        return std::visit([](auto& ant) -> std::string const& { return ant.id; }, antenna);
    }

    [[nodiscard]] constexpr std::string const& get_origin_id(Antenna& antenna)
    {
        return std::visit([](auto& ant) -> std::string const& { return ant.origin_id; }, antenna);
    }

    [[nodiscard]] constexpr Reference*& get_origin(Antenna& antenna)
    {
        return std::visit([](auto& ant) -> Reference*& { return ant.origin; }, antenna);
    }

    template <typename T, IsAntenna A>
    [[nodiscard]] constexpr decltype(auto) cast(A& antenna)
    {
        if (auto specified = std::get_if<T>(&antenna); specified) { return *specified; }
        throw SimulationError("Antenna cast failed: {} has type {}, but {} was requested", static_cast<const void*>(&antenna), get_type_name(antenna),
            magic_enum::enum_name(static_cast<AntennaType>(get_variant_index<T, Antenna>())));
    }

    [[nodiscard]] complex_t calc_voltage_gain(Antenna const& tx, Antenna const& rx, setup::NumParams const& num_params);
    [[nodiscard]] double calc_power_gain(Antenna const& tx, Antenna const& rx, setup::NumParams const& num_params);


    /**
     * Creates new scalar field that is the power field if the tx is fixed in space and the rx is moved around
     * @param tx transmitter antenna
     * @param rx receiver antenna
     * @param num_params numerical parameters
     * @return power field between tx and rx
     */
    // [[nodiscard]] ScalarField<double> get_power_field(Antenna const& tx, Antenna& rx, setup::NumParams const& num_params);

    /**
     * Interconnect all antennas to their reference, i.d., resolving the origins ".origin_id" ids to their actual pointer ".origin".
     * If an antenna is a RadiatorArray, the function recurses for each array element.
     * Important: After this function call, the references must remain at their memory location.
     * Otherwise, the pointers become invalid which will cause segmentations faults.
     * This function is idempotent.
     * @param antennas std::span of antennas that get interconnected
     * @param references std::span of references that are provided for the antennas and looked through
     */
    void resolve_origins(std::span<Antenna> antennas, std::span<Reference> references);

    /**
     * Interconnect all radiators to their reference, i.d., resolving the origins ".origin_id" ids to their actual pointer ".origin".
     * Important: After this function call, the references must remain at their memory location.
     * Otherwise, the pointers become invalid which will cause segmentations faults.
     * This function is idempotent.
     * @param radiators std::span of radiators that get interconnected
     * @param references std::span of references that are provided for the antennas and looked through
     */
    void resolve_origins(std::span<Radiator> radiators, std::span<Reference> references);

    /**
     * Interconnect all antennas to their reference, i.d., resolving the origins ".origin_id" ids to their actual pointer ".origin".
     * If an antenna is a RadiatorArray, the function recurses for each array element.
     * Important: After this function call, the references must remain at their memory location.
     * Otherwise, the pointers become invalid which will cause segmentations faults.
     * This function is idempotent.
     * @param antennas std::initializer_list of antennas that get interconnected
     * @param references std::initializer_list of references that are provided for the antennas and looked through
     */
    void resolve_origins(std::initializer_list<std::reference_wrapper<Antenna>> antennas, std::initializer_list<std::reference_wrapper<Reference>> references);

    /**
     * Interconnect all radiators to their reference, i.d., resolving the origins ".origin_id" ids to their actual pointer ".origin".
     * Important: After this function call, the references must remain at their memory location.
     * Otherwise, the pointers become invalid which will cause segmentations faults.
     * This function is idempotent.
     * @param radiators std::initializer_list of radiators that get interconnected
     * @param references std::initializer_list of references that are provided for the antennas and looked through
     */
    void resolve_origins(std::initializer_list<std::reference_wrapper<Radiator>> radiators, std::initializer_list<std::reference_wrapper<Reference>> references);

    [[nodiscard]] Antenna& get(std::span<Antenna> antennas, std::string const& id);

} // namespace antenna
