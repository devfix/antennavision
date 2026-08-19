//
// Created by Tristan Krause on 2026-07-13.
//

#include "components/antenna.hpp"
#include <functional>
#include <print>
#include <ranges>

#include "NumCpp/Functions/zeros.hpp"
#include "logging.hpp"
#include "math/coords.hpp"
#include "math/functions.hpp"

namespace components::antenna
{
    using reference::Reference;
    using std::ranges::to;
    using std::views::transform;

    namespace
    {
        constexpr std::string_view LOG_NAME = "antenna";

#ifndef NDEBUG
        constexpr bool DEBUG_MODE = true;
#else
        constexpr bool DEBUG_MODE = false;
#endif

        Vec calc_global_elv(Radiator const& rad, Pos const& pos, double wavelength)
        {
            // locate global pos in local coordinate of the radiator
            auto const pos_local_cartesian = rad.origin->local_from_global_pos(pos);

            // get spherical vector at local coordinate
            auto const elv_spherical = rad.get_elv_spherical_from_cartesian(pos_local_cartesian, wavelength);

            // get rotation matrix of local spherical vector and convert it to local cartesian vector
            auto const rot_mat = math::get_rot_mat_from_cartesian(pos_local_cartesian);
            auto const elv_cartesian = nc::dot(rot_mat, elv_spherical);

            // convert local cartesian vector to global cartesian vector
            return rad.origin->global_from_local_vec(elv_cartesian);
        }

        Complex calc_voltage_gain_direct(Radiator const& tx, Radiator const& rx, double wavelength, setup::SimParams const& sim_params)
        {
            if constexpr (DEBUG_MODE)
            {
                if (tx.origin == nullptr) throw SimulationError("TX Radiator '{}' has unresolved origin '{}'", tx.id, tx.origin_id);
                if (rx.origin == nullptr) throw SimulationError("RX Radiator '{}' has unresolved origin '{}'", rx.id, rx.origin_id);
                sim_params.assert_integrity();
            }

            double r = (tx.origin->global_from_local_pos(POS_ZERO) - rx.origin->global_from_local_pos(POS_ZERO)).norm();
            if (r <= wavelength / 100)
            {
                antennavision::logging::warn(LOG_NAME,
                    "\nWarning: Radiator {} is very close to radiator {}, distance: {} m ({} λ)",
                    tx.id,
                    rx.id,
                    r,
                    r / wavelength);
                r = std::max(r, NUMERICAL_MARGIN); // sanity
            }

            auto const k = 2.0 * pi / wavelength; // wave number
            auto const phase = std::exp(-j * k * r);
            auto const path_loss = sim_params.enable_path_loss ? wavelength / (4 * pi * r) : 1.0;

            auto const tx_iso = static_cast<std::size_t>(tx.type == Radiator::Type::IsotropicRadiator);
            auto const rx_iso = static_cast<std::size_t>(rx.type == Radiator::Type::IsotropicRadiator);
            switch (tx_iso << 1u | rx_iso)
            {
                case 0b00:
                {
                    Vec const elv_tx = calc_global_elv(tx, rx.origin->global_from_local_pos(POS_ZERO), wavelength);
                    Vec const elv_rx = calc_global_elv(rx, tx.origin->global_from_local_pos(POS_ZERO), wavelength);
                    auto const ms_elv_tx = tx.ms_elv ? tx.ms_elv(wavelength) : Radiator::calc_ms_elv(tx.elv_spherical, wavelength, sim_params);
                    auto const ms_elv_rx = rx.ms_elv ? rx.ms_elv(wavelength) : Radiator::calc_ms_elv(rx.elv_spherical, wavelength, sim_params);
                    Complex const coupling = elv_tx.dot(elv_rx).item() / std::sqrt(ms_elv_tx * ms_elv_rx);
                    return -j * coupling * phase * path_loss;
                }
                case 0b01:
                {
                    auto const ms_elv_tx = tx.ms_elv ? tx.ms_elv(wavelength) : Radiator::calc_ms_elv(tx.elv_spherical, wavelength, sim_params);
                    Complex const coupling =
                        nc::norm(calc_global_elv(tx, rx.origin->global_from_local_pos(POS_ZERO), wavelength)).item() / std::sqrt(ms_elv_tx);
                    return -j * coupling * phase * path_loss;
                }
                case 0b10:
                {
                    auto const ms_elv_rx = rx.ms_elv ? rx.ms_elv(wavelength) : Radiator::calc_ms_elv(rx.elv_spherical, wavelength, sim_params);
                    Complex const coupling =
                        nc::norm(calc_global_elv(rx, tx.origin->global_from_local_pos(POS_ZERO), wavelength)).item() / std::sqrt(ms_elv_rx);
                    return -j * coupling * phase * path_loss;
                }
                case 0b11: return -j * phase * path_loss;
                default: break;
            }
            std::unreachable();
        }

        Vec calc_electrical_field_direct(Radiator const& rad, Pos const& pos, Complex i_exc, double wavelength, setup::SimParams const& sim_params)
        {
            if constexpr (DEBUG_MODE)
            {
                if (rad.origin == nullptr) throw SimulationError("Radiator '{}' has unresolved origin '{}'", rad.id, rad.origin_id);
                sim_params.assert_integrity();
            }
            if (rad.type == Radiator::Type::IsotropicRadiator) throw SimulationError("{} is not supported for isotropic radiators", __func__);

            double r = (rad.origin->global_from_local_pos(POS_ZERO) - pos).norm();
            if (r <= wavelength / 100)
            {
                antennavision::logging::warn(LOG_NAME,
                    "\nWarning: Electrical field position is very close to radiator {}, distance: {} m ({} λ)",
                    rad.id,
                    r,
                    r / wavelength);
                r = std::max(r, NUMERICAL_MARGIN); // sanity
            }

            auto const k = 2.0 * pi / wavelength; // wave number
            auto const propagation = std::exp(-j * k * r) * (sim_params.enable_path_loss ? 1.0 / r : 1.0);

            Vec const elv = calc_global_elv(rad, pos, wavelength);

            return -j * Z0 * k * i_exc / (4.0 * pi) * propagation * elv;
        }

        void resolve_origin_impl(Radiator& rad, std::span<Reference*> refs)
        {
            auto const it = std::ranges::find(refs, rad.origin_id, [](Reference* ref) -> std::string const& { return ref->id; });
            if (it == refs.end()) { throw SimulationError("Radiator '{}' has non-existing origin '{}'", rad.id, rad.origin_id); }
            rad.origin = *it;
        }

        void resolve_origin_impl(Antenna& ant, std::span<Reference*> refs)
        {
            auto origin_id = get_origin_id(ant);
            if (not origin_id.empty()) // TODO is this safe?
            {
                auto const it = std::ranges::find(refs, origin_id, [](Reference* ref) -> std::string const& { return ref->id; });
                if (it == refs.end()) { throw SimulationError("Antenna '{}' has non-existing origin '{}'", get_id(ant), origin_id); }
                get_origin(ant) = *it;
            }

            ant.visit(
                [&refs](auto& a)
                {
                    if constexpr (antenna::is_array<decltype(a)>)
                    {
                        // create pointer vector of passed references and the references of the AntennaArray
                        std::vector refs_merged(refs.begin(), refs.end());
                        refs_merged.reserve(refs_merged.size() + a.references.size());
                        std::ranges::transform(a.references, std::back_insert_iterator(refs_merged), [](Reference& ref) { return std::addressof(ref); });

                        reference::resolve_origins(refs_merged); // resolve all references origins
                        rebind_origin_pointers(a.elements, a.references); // resolve element origins within the AntennaArray
                    };
                });
        }
    } // namespace

    Complex calc_voltage_gain( //
        Antenna const& tx,
        Antenna const& rx,
        double wavelength,
        std::span<Complex const> tx_coeffs,
        std::span<Complex const> rx_coeffs,
        setup::SimParams const& sim_params //
    )
    {
        if (tx_coeffs.size() != size(tx))
            throw SimulationError("Invalid number of tx coefficients for '{}': expected {}, got {}", get_id(tx), size(tx), tx_coeffs.size());
        if (rx_coeffs.size() != size(rx))
            throw SimulationError("Invalid number of rx coefficients for '{}': expected {}, got {}", get_id(rx), size(rx), rx_coeffs.size());

        auto const tx_rad = static_cast<std::size_t>(std::holds_alternative<Radiator>(tx));
        auto const rx_rad = static_cast<std::size_t>(std::holds_alternative<Radiator>(rx));
        switch (tx_rad << 1u | rx_rad)
        {
            case 0b00: // both antennas are arrays
                return std::visit(
                    [&](const auto& tx_arr, const auto& rx_arr)
                    {
                        Complex gain = 0;
                        if constexpr (is_array<decltype(tx_arr)> and is_array<decltype(rx_arr)>)
                        {
                            // core computation loop
                            for (std::size_t k_rx = 0; k_rx < rx_arr.elements.size(); k_rx++)
                                for (std::size_t k_tx = 0; k_tx < tx_arr.elements.size(); k_tx++)
                                    gain += tx_coeffs[k_tx] //
                                        * calc_voltage_gain_direct(tx_arr.elements[k_tx], rx_arr.elements[k_rx], wavelength, sim_params) //
                                        * rx_coeffs[k_rx];
                        }
                        else
                            std::unreachable();
                        return gain;
                    },
                    tx,
                    rx);
            case 0b01: // the transmitter is an antenna array, the receiver a single radiator
                return tx.visit(
                    [&](const auto& tx_arr)
                    {
                        auto& rx_rad = std::get<Radiator>(rx);
                        Complex gain = 0;
                        if constexpr (is_array<decltype(tx_arr)>)
                        {
                            // core computation loop
                            for (std::size_t k = 0; k < tx_arr.elements.size(); k++)
                                gain += tx_coeffs[k] //
                                    * calc_voltage_gain_direct(tx_arr.elements[k], rx_rad, wavelength, sim_params) //
                                    * rx_coeffs[0];
                        }
                        else
                            std::unreachable();
                        return gain;
                    });
            case 0b10: // the transmitter is a single radiator, the receiver an antenna array
                return rx.visit(
                    [&](const auto& rx_arr)
                    {
                        auto& tx_rad = std::get<Radiator>(tx);
                        Complex gain = 0;
                        if constexpr (is_array<decltype(rx_arr)>)
                        {
                            // core computation loop
                            for (std::size_t k = 0; k < rx_arr.elements.size(); k++)
                                gain += tx_coeffs[0] //
                                    * calc_voltage_gain_direct(tx_rad, rx_arr.elements[k], wavelength, sim_params) //
                                    * rx_coeffs[k];
                        }
                        else
                            std::unreachable();
                        return gain;
                    });
                ;
            case 0b11: // both antennas are single radiators
            {
                // no computation loop, only direct forward
                return tx_coeffs[0] * calc_voltage_gain_direct(std::get<Radiator>(tx), std::get<Radiator>(rx), wavelength, sim_params) * rx_coeffs[0];
            }
            default: std::unreachable();
        }
    }

    double calc_power_gain( //
        Antenna const& tx,
        Antenna const& rx,
        double wavelength,
        std::span<Complex const> tx_coeffs,
        std::span<Complex const> rx_coeffs,
        setup::SimParams const& sim_params //
    )
    { return math::square(std::abs(calc_voltage_gain(tx, rx, wavelength, tx_coeffs, rx_coeffs, sim_params))); }

    Vec calc_electrical_field(Antenna const& ant,
        Pos const& pos_global,
        Complex i_exc,
        double wavelength,
        std::span<Complex const> coeffs,
        setup::SimParams const& sim_params)
    {
        if (coeffs.size() != size(ant))
            throw SimulationError("Invalid number of coefficients for '{}': expected {}, got {}", get_id(ant), size(ant), coeffs.size());

        // case 1: antenna is a single radiator
        if (std::holds_alternative<Radiator>(ant))
        {
            auto const rad = std::get<Radiator>(ant);
            return coeffs[0] * calc_electrical_field_direct(rad, pos_global, i_exc, wavelength, sim_params);
        }

        // case 2: antenna is an array
        return ant.visit(
            [&](const auto& arr)
            {
                using Type = std::decay_t<decltype(arr)>;
                Vec field = nc::zeros<Complex>(coeffs.size(), 1);
                if constexpr (std::is_same_v<Type, components::RadiatorArray>)
                {
                    // core computation loop
                    for (std::size_t k = 0; k < arr.elements.size(); k++)
                        field += coeffs[k] * calc_electrical_field_direct(arr.elements[k], pos_global, i_exc, wavelength, sim_params);
                }
                else
                    std::unreachable();
                return field;
            });
    }

    double calc_directivity_from_spherical(Antenna const& ant,
        double polar,
        double azimuth,
        double wavelength,
        std::span<Complex const> coeffs,
        setup::SimParams const& sim_params)
    {
        return ant.visit(
            [&](auto const& arr) -> double
            {
                if constexpr (is_radiator<decltype(arr)>)
                {
                    Radiator const& rad = arr;
                    if (arr.type == Radiator::Type::IsotropicRadiator) return 1.0;
                    Vec const elv = rad.elv_spherical(polar, azimuth, wavelength);
                    double const ms_elv = rad.ms_elv ? rad.ms_elv(wavelength) : Radiator::calc_ms_elv(rad.elv_spherical, wavelength, sim_params);
                    return math::square(math::norm(elv)) / ms_elv;
                }
                else
                {
                    if (coeffs.size() != arr.elements.size())
                        throw SimulationError("Invalid number of array coefficients for '{}': expected {}, got {}", //
                            arr.id,
                            arr.elements.size(),
                            coeffs.size());

                    // integrity check: all elements must have the same radiator type
                    if (arr.elements[0].type == Radiator::Type::IsotropicRadiator)
                    {
                        for (std::size_t i = 1; i < arr.elements.size(); ++i)
                        {
                            if (arr.elements[i].type != Radiator::Type::IsotropicRadiator)
                                throw SimulationError( //
                                    "Unsupported array '{}': {} requires either all or none of the elements to be isotropic radiators\n"
                                    "Hint: element '{}' is isotropic, but '{}' is not",
                                    arr.id,
                                    __func__,
                                    arr.elements[0].id,
                                    arr.elements[i].id);
                        }
                    }
                    else
                    {
                        for (std::size_t i = 1; i < arr.elements.size(); ++i)
                        {
                            if (arr.elements[i].type == Radiator::Type::IsotropicRadiator)
                                throw SimulationError( //
                                    "Unsupported array '{}': {} requires either all or none of the elements to be isotropic radiators\n"
                                    "Hint: element '{}' is isotropic, but '{}' is not",
                                    arr.id,
                                    __func__,
                                    arr.elements[i].id,
                                    arr.elements[0].id);
                        }
                    }

                    Pos const pos_arr_center = arr.origin->global_from_local_pos(POS_ZERO); // center position of the array
                    double const k = 2.0 * pi / wavelength; // wave number

                    // ELV callback for superimposed far-field elv of the array
                    auto calc_elv_superimposed = [&](double polar, double azimuth, double wavelength) -> Vec
                    {
                        // unit vector pointing towards the far-field direction (polar, azimuth)
                        Pos const dir = math::cartesian_from_spherical_pos(1.0, polar, azimuth);

                        Vec total_elv = nc::zeros<Complex>(3, 1);
                        for (std::size_t i = 0; i < size(arr); ++i)
                        {
                            Radiator const& el = arr.elements[i]; // current array element

                            // Relative position of current element to the center of the array
                            Pos const pos = el.origin->global_from_local_pos(POS_ZERO) - pos_arr_center;

                            double const delta_r = dir.dot(pos);
                            Complex const phase_term = std::exp(j * k * delta_r);

                            Vec const elv =
                                el.type == Radiator::Type::IsotropicRadiator ? math::vec<Complex>(1, 0, 0) : el.elv_spherical(polar, azimuth, wavelength);

                            total_elv += coeffs[i] * phase_term * elv;
                        }
                        asm("nop");
                        return total_elv;
                    };

                    double const elv_squared = math::square(math::norm(calc_elv_superimposed(polar, azimuth, wavelength)));
                    double const ms_elv = Radiator::calc_ms_elv(calc_elv_superimposed, wavelength, sim_params);
                    return elv_squared / ms_elv;
                }
            });
    }

    double calc_directivity_from_cartesian(Antenna const& ant,
        Pos const& pos_local,
        double wavelength,
        std::span<Complex const> coeffs,
        setup::SimParams const& sim_params)
    {
        auto const [r, polar, azimuth] = math::spherical_from_cartesian_pos(pos_local);
        return calc_directivity_from_spherical(ant, polar, azimuth, wavelength, coeffs, sim_params);
    }

    void rebind_origin_pointers(std::span<Antenna> antennas, std::span<Reference> references)
    {
        std::vector<Reference*> ref_vec = references | transform([](Reference& ref) { return std::addressof(ref); }) | to<std::vector>();
        for (Antenna& ant : antennas) { resolve_origin_impl(ant, ref_vec); }
    }

    void rebind_origin_pointers(std::span<Radiator> radiators, std::span<Reference> references)
    {
        std::vector<Reference*> ref_vec = references | transform([](Reference& ref) { return std::addressof(ref); }) | to<std::vector>();
        for (Radiator& rad : radiators) { resolve_origin_impl(rad, ref_vec); }
    }

    void rebind_origin_pointers(std::initializer_list<std::reference_wrapper<Antenna>> antennas,
        std::initializer_list<std::reference_wrapper<Reference>> references)
    {
        std::vector<Reference*> ref_vec = references | transform([](Reference& ref) { return std::addressof(ref); }) | to<std::vector>();
        for (auto const& ant : antennas) { resolve_origin_impl(ant, ref_vec); }
    }

    void rebind_origin_pointers(std::initializer_list<std::reference_wrapper<Radiator>> radiators,
        std::initializer_list<std::reference_wrapper<Reference>> references)
    {
        std::vector<Reference*> ref_vec = references | transform([](Reference& ref) { return std::addressof(ref); }) | to<std::vector>();
        for (auto const& rad : radiators) { resolve_origin_impl(rad, ref_vec); }
    }

    Antenna const& get(std::span<Antenna const> antennas, std::string_view id)
    {
        auto const it = std::ranges::find(antennas, id, [](auto& ant) { return std::visit([](auto& a) { return a.id; }, ant); });
        if (it == antennas.end()) { throw SimulationError("Could not find antenna with id '{}'", id); }
        return *it;
    }

    Antenna& get(std::span<Antenna> antennas, std::string_view id)
    {
        // Safe const_cast: Delegates to the const overload to eliminate duplication.
        // Safe because the underlying 'antennas' span refers to non-const objects.
        return const_cast<Antenna&>(get(std::span<Antenna const>{antennas}, id));
    }

    std::size_t size(Antenna const& ant)
    {
        return ant.visit(
            [](auto const& a)
            {
                if constexpr (is_radiator<decltype(a)>)
                    return 1uz;
                else
                    return a.elements.size();
            });
    }

} // namespace components::antenna
