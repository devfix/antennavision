//
// Created by Tristan Krause on 2026-07-08.
//

#pragma once

#include <magic_enum/magic_enum.hpp>
#include <span>
#include <variant>
#include "components/radiatorarray.hpp"
#include "memory.hpp"
#include "simulationerror.hpp"

namespace components
{
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    using Antenna = std::variant<Radiator, RadiatorArray>;
    enum struct AntennaType // must be same order as in Antenna
    {
        Radiator,
        RadiatorArray
    };
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    namespace antenna
    {
        template <typename T>
        concept is_antenna = std::same_as<std::decay_t<T>, Antenna>;

        template <typename T>
        concept is_radiator = std::same_as<std::decay_t<T>, Radiator>;

        template <typename T>
        concept is_array = not is_radiator<T>;

        [[nodiscard]] constexpr std::string_view get_type_name(const Antenna& ant)
        {
            auto active_enum = static_cast<AntennaType>(ant.index());
            return magic_enum::enum_name(active_enum);
        }

        [[nodiscard]] constexpr std::string_view get_id(Antenna const& antenna)
        {
            return antenna.visit([](auto& ant) -> std::string_view { return ant.id; });
        }

        [[nodiscard]] constexpr std::string_view get_origin_id(Antenna const& antenna)
        {
            return antenna.visit([](auto& ant) -> std::string_view { return ant.origin_id; });
        }

        [[nodiscard]] constexpr reference::Reference* const& get_origin(Antenna const& antenna)
        {
            return antenna.visit([](auto& ant) -> reference::Reference* const& { return ant.origin; });
        }

        [[nodiscard]] constexpr reference::Reference*& get_origin(Antenna& antenna)
        {
            return antenna.visit([](auto& ant) -> reference::Reference*& { return ant.origin; });
        }

        template <typename T, is_antenna A>
        [[nodiscard]] constexpr decltype(auto) cast(A& antenna)
        {
            if (auto specified = std::get_if<T>(&antenna); specified) { return *specified; }
            throw SimulationError("Antenna cast failed: {} has type {}, but {} was requested",
                static_cast<const void*>(&antenna),
                get_type_name(antenna),
                magic_enum::enum_name(static_cast<AntennaType>(variant_index_v<T, Antenna>)));
        }

        [[nodiscard]] Complex calc_voltage_gain( //
            Antenna const& tx,
            Antenna const& rx,
            double wavelength,
            std::span<Complex const> tx_coeffs,
            std::span<Complex const> rx_coeffs,
            setup::SimParams const& sim_params //
        );

        [[nodiscard]] double calc_power_gain( //
            Antenna const& tx,
            Antenna const& rx,
            double wavelength,
            std::span<Complex const> tx_coeffs,
            std::span<Complex const> rx_coeffs,
            setup::SimParams const& sim_params //
        );

        [[nodiscard]] Vec calc_electrical_field( //
            Antenna const& ant,
            Pos const& pos_global,
            Complex i_exc,
            double wavelength,
            std::span<Complex const> coeffs,
            setup::SimParams const& sim_params //
        );

        [[nodiscard]] double calc_directivity_from_spherical( //
            Antenna const& ant,
            double polar,
            double azimuth,
            double wavelength,
            std::span<Complex const> coeffs,
            setup::SimParams const& sim_params //
        );

        [[nodiscard]] double calc_directivity_from_cartesian( //
            Antenna const& ant,
            Pos const& pos_local,
            double wavelength,
            std::span<Complex const> coeffs,
            setup::SimParams const& sim_params //
        );

        /**
         * Creates new scalar field that is the power field if the tx is fixed in space and the rx is moved around
         * @param tx transmitter antenna
         * @param rx receiver antenna
         * @param sim_params numerical parameters
         * @return power field between tx and rx
         */
        // [[nodiscard]] ScalarField<double> get_power_field(Antenna const& tx, Antenna& rx, setup::NumParams const& sim_params);

        /**
         * Interconnect all antennas to their reference, i.d., resolving the origins ".origin_id" ids to their actual pointer ".origin".
         * If an antenna is a RadiatorArray, the function recurses for each array element.
         * Important: After this function call, the references must remain at their memory location.
         * Otherwise, the pointers become invalid which will cause segmentations faults.
         * This function is idempotent.
         * @param antennas std::span of antennas that get interconnected
         * @param references std::span of references that are provided for the antennas and looked through
         */
        void rebind_origin_pointers(std::span<Antenna> antennas, std::span<reference::Reference> references);

        /**
         * Interconnect all radiators to their reference, i.d., resolving the origins ".origin_id" ids to their actual pointer ".origin".
         * Important: After this function call, the references must remain at their memory location.
         * Otherwise, the pointers become invalid which will cause segmentations faults.
         * This function is idempotent.
         * @param radiators std::span of radiators that get interconnected
         * @param references std::span of references that are provided for the antennas and looked through
         */
        void rebind_origin_pointers(std::span<Radiator> radiators, std::span<reference::Reference> references);

        /**
         * Interconnect all antennas to their reference, i.d., resolving the origins ".origin_id" ids to their actual pointer ".origin".
         * If an antenna is a RadiatorArray, the function recurses for each array element.
         * Important: After this function call, the references must remain at their memory location.
         * Otherwise, the pointers become invalid which will cause segmentations faults.
         * This function is idempotent.
         * @param antennas std::initializer_list of antennas that get interconnected
         * @param references std::initializer_list of references that are provided for the antennas and looked through
         */
        void rebind_origin_pointers(std::initializer_list<std::reference_wrapper<Antenna>> antennas,
            std::initializer_list<std::reference_wrapper<reference::Reference>> references);

        /**
         * Interconnect all radiators to their reference, i.d., resolving the origins ".origin_id" ids to their actual pointer ".origin".
         * Important: After this function call, the references must remain at their memory location.
         * Otherwise, the pointers become invalid which will cause segmentations faults.
         * This function is idempotent.
         * @param radiators std::initializer_list of radiators that get interconnected
         * @param references std::initializer_list of references that are provided for the antennas and looked through
         */
        void rebind_origin_pointers(std::initializer_list<std::reference_wrapper<Radiator>> radiators,
            std::initializer_list<std::reference_wrapper<reference::Reference>> references);

        [[nodiscard]] Antenna const& get(std::span<Antenna const> antennas, std::string_view id);
        [[nodiscard]] Antenna& get(std::span<Antenna> antennas, std::string_view id);

        [[nodiscard]] std::size_t size(Antenna const& ant);

    } // namespace antenna
} // namespace components
