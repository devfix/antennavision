//
// Created by core on 2026-07-13.
//

#include "components/antenna.hpp"
#include <functional>
#include <print>
#include <ranges>

namespace antenna
{
    namespace
    {
        complex_t calc_voltage_gain_direct(Radiator const& tx, Radiator const& rx, math::NumParams const& num_params)
        {
            if (tx.origin == nullptr) { throw SimulationError("TX Radiator '{}' has unresolved origin '{}'", tx.id, tx.origin_id); }
            if (rx.origin == nullptr) { throw SimulationError("RX Radiator '{}' has unresolved origin '{}'", rx.id, rx.origin_id); }
            auto const& wavelength = num_params.system_wavelength;
            double const r = (tx.origin->global_from_local_pos(POS_ZERO) - rx.origin->global_from_local_pos(POS_ZERO)).norm();
            if (r < wavelength / 10)
            {
                std::println("Warning: Radiator {} is very close to radiator {}, distance: {} m ({} λ)", tx.id, rx.id, r, r / wavelength);
            }

            auto const pos_local_tx = tx.origin->localize(*rx.origin); // position of rx radiator in tx coordinate
            auto const pos_local_rx = rx.origin->localize(*tx.origin); // position of tx radiator in rx coordinate
            auto const rot_mat_tx = math::get_rot_mat_from_cartesian(pos_local_tx);
            auto const rot_mat_rx = math::get_rot_mat_from_cartesian(pos_local_rx);
            auto const elv_spherical_tx = tx.get_elv_spherical_from_cartesian(pos_local_tx, wavelength);
            auto const elv_spherical_rx = rx.get_elv_spherical_from_cartesian(pos_local_rx, wavelength);
            auto const elv_cartesian_tx = nc::dot(rot_mat_tx, elv_spherical_tx);
            auto const elv_cartesian_rx = nc::dot(rot_mat_rx, elv_spherical_rx);
            auto const elv_global_tx = tx.origin->global_from_local_vec(elv_cartesian_tx);
            auto const elv_global_rx = rx.origin->global_from_local_vec(elv_cartesian_rx);
            auto const g = elv_global_tx.dot(elv_global_rx).item();
            auto const propagation = std::exp(-j * 2.0 * pi * r / wavelength) * wavelength / (4.0 * pi * r);
            auto const mean_squared_elv_tx =
                tx.mean_squared_elv ? tx.mean_squared_elv(wavelength) : Radiator::calc_mean_squared_effective_length(tx.elv_spherical, num_params);
            auto const mean_squared_elv_rx =
                rx.mean_squared_elv ? rx.mean_squared_elv(wavelength) : Radiator::calc_mean_squared_effective_length(rx.elv_spherical, num_params);
            return -j * g / std::sqrt(mean_squared_elv_tx * mean_squared_elv_rx) * propagation;
        }

        void resolve_origin_impl(Radiator& rad, std::span<Reference*> references)
        {
            auto const it = std::ranges::find(references, rad.origin_id, [](Reference* ref) -> std::string const& { return ref->id; });
            if (it == references.end()) { throw SimulationError("Radiator '{}' has non-existing origin '{}'", rad.id, rad.origin_id); }
            rad.origin = *it;
        }

        void resolve_origin_impl(Antenna& ant, std::span<Reference*> references)
        {
            std::string const& origin_id = get_origin_id(ant);
            auto const it = std::ranges::find(references, origin_id, [](Reference* ref) -> std::string const& { return ref->id; });
            if (it == references.end()) { throw SimulationError("Antenna '{}' has non-existing origin '{}'", get_id(ant), origin_id); }
            get_origin(ant) = *it;

            std::visit(
                [&references](auto& a)
                {
                    if constexpr (antenna::is_array<decltype(a)>)
                    {
                        for (Radiator& rad : a.elements) { resolve_origin_impl(rad, references); }
                    }
                },
                ant);
        }
    } // namespace

    complex_t calc_voltage_gain(Antenna const& tx, Antenna const& rx, math::NumParams const& num_params)
    {
        Radiator const& radiator_rx = cast<Radiator>(rx);
        return std::visit(
            [&](auto const& ant_tx)
            {
                using Type = std::decay_t<decltype(ant_tx)>;
                if constexpr (std::is_same_v<Type, Radiator>) { return calc_voltage_gain_direct(ant_tx, radiator_rx, num_params); }
                else if constexpr (std::is_base_of_v<RadiatorArray<Type>, Type>)
                {
                    complex_t gain = 0;
                    for (std::size_t k = 0; k < ant_tx.elements.size(); k++)
                    {
                        gain += ant_tx.coefficients[k] * calc_voltage_gain_direct(ant_tx.elements[k], radiator_rx, num_params);
                    }
                    return gain;
                }
                else
                {
                    throw SimulationError("Invalid antenna type");
                }
            },
            tx);
    }

    double calc_power_gain(Antenna const& tx, Antenna const& rx, math::NumParams const& num_params)
    { return math::square(std::abs(calc_voltage_gain(tx, rx, num_params))); }

    ScalarField<complex_t> get_voltage_field(Antenna const& tx, Antenna& rx, math::NumParams const& num_params)
    {
        return {std::format("voltage-field.{}.{}", get_id(tx), get_id(rx)),
            [&tx, &rx, num_params](pos_t const& pos, double const wavelength) -> complex_t
            {
                get_origin(rx)->pos = pos;
                return calc_voltage_gain(tx, rx, num_params);
            },
            [] {}, num_params};
    }

    ScalarField<double> get_power_field(Antenna const& tx, Antenna& rx, math::NumParams const& num_params)
    {
        return {std::format("power-field.{}.{}", get_id(tx), get_id(rx)),
            [&tx, &rx, num_params](pos_t const& pos, double const wavelength) -> double
            {
                get_origin(rx)->pos = pos;
                return calc_power_gain(tx, rx, num_params);
            },
            [] {}, num_params};
    }

    void resolve_origins(std::span<Antenna> antennas, std::span<Reference> references)
    {
        std::vector<Reference*> ref_vec =
            references | std::views::transform([](Reference& ref) { return std::addressof(ref); }) | std::ranges::to<std::vector>();
        for (Antenna& ant : antennas) { resolve_origin_impl(ant, ref_vec); }
    }

    void resolve_origins(std::span<Radiator> radiators, std::span<Reference> references)
    {
        std::vector<Reference*> ref_vec =
            references | std::views::transform([](Reference& ref) { return std::addressof(ref); }) | std::ranges::to<std::vector>();
        for (Radiator& rad : radiators) { resolve_origin_impl(rad, ref_vec); }
    }

    void resolve_origins(std::initializer_list<std::reference_wrapper<Antenna>> antennas, std::initializer_list<std::reference_wrapper<Reference>> references)
    {
        std::vector<Reference*> ref_vec =
            references | std::views::transform([](Reference& ref) { return std::addressof(ref); }) | std::ranges::to<std::vector>();
        for (auto const& ant : antennas) { resolve_origin_impl(ant, ref_vec); }
    }

    void resolve_origins(std::initializer_list<std::reference_wrapper<Radiator>> radiators, std::initializer_list<std::reference_wrapper<Reference>> references)
    {
        std::vector<Reference*> ref_vec =
            references | std::views::transform([](Reference& ref) { return std::addressof(ref); }) | std::ranges::to<std::vector>();
        for (auto const& rad : radiators) { resolve_origin_impl(rad, ref_vec); }
    }

    Antenna& get(std::span<Antenna> antennas, std::string const& id)
    {
        auto const it = std::ranges::find(antennas, id, [](auto& ant) { return std::visit([](auto& a) { return a.id; }, ant); });
        if (it == antennas.end()) { throw SimulationError("Could not find antenna with id '{}'", id); }
        return *it;
    }

} // namespace antenna
