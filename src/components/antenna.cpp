//
// Created by Tristan Krause on 2026-07-13.
//

#include "components/antenna.hpp"
#include <functional>
#include <print>
#include <ranges>
#include "math/functions.hpp"
#include "math/coords.hpp"

namespace antenna
{
    using reference::Reference;
    using std::ranges::to;
    using std::views::transform;

    namespace
    {
        Complex calc_voltage_gain_direct(Radiator const& tx, Radiator const& rx, setup::NumParams const& num_params, double wavelength)
        {
            if (tx.origin == nullptr) { throw SimulationError("TX Radiator '{}' has unresolved origin '{}'", tx.id, tx.origin_id); }
            if (rx.origin == nullptr) { throw SimulationError("RX Radiator '{}' has unresolved origin '{}'", rx.id, rx.origin_id); }
            num_params.check();

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

        void resolve_origin_impl(Radiator& rad, std::span<Reference*> refs)
        {
            auto const it = std::ranges::find(refs, rad.origin_id, [](Reference* ref) -> std::string const& { return ref->id; });
            if (it == refs.end()) { throw SimulationError("Radiator '{}' has non-existing origin '{}'", rad.id, rad.origin_id); }
            rad.origin = *it;
        }

        void resolve_origin_impl(Antenna& ant, std::span<Reference*> refs)
        {
            std::string const& origin_id = get_origin_id(ant);
            auto const it = std::ranges::find(refs, origin_id, [](Reference* ref) -> std::string const& { return ref->id; });
            if (it == refs.end()) { throw SimulationError("Antenna '{}' has non-existing origin '{}'", get_id(ant), origin_id); }
            get_origin(ant) = *it;

            std::visit(
                [&refs](auto& a)
                {
                    if constexpr (antenna::is_array<decltype(a)>)
                    {
                        // create pointer vector of passed references and the references of the AntennaArray
                        std::vector refs_merged(refs.begin(), refs.end());
                        refs_merged.reserve(refs_merged.size() + a.references.size());
                        std::ranges::transform(a.references, std::back_insert_iterator(refs_merged), [](Reference& ref) { return std::addressof(ref); });

                        reference::resolve_origins(refs_merged); // resolve all references origins
                        resolve_origins(a.elements, a.references); // resolve element origins within the AntennaArray
                    };
                },
                ant);
        }
    } // namespace

    Complex calc_voltage_gain(Antenna const& tx, Antenna const& rx, setup::NumParams const& num_params, double wavelength)
    {
        std::size_t constexpr Key_TxArr_RxArr = 0b00;
        std::size_t constexpr Key_TxArr_RxRad = 0b01;
        std::size_t constexpr Key_TxRad_RxArr = 0b10;
        std::size_t constexpr Key_TxRad_RxRad = 0b11;
        std::size_t const key = (std::holds_alternative<Radiator>(tx) << 1) | std::holds_alternative<Radiator>(rx);
        switch (key)
        {
            case Key_TxArr_RxArr:
                return std::visit(
                    [&](const auto& tx_arr, const auto& rx_arr)
                    {
                        using TxType = std::decay_t<decltype(tx_arr)>;
                        using RxType = std::decay_t<decltype(rx_arr)>;
                        Complex gain = 0;
                        if constexpr (std::is_base_of_v<RadiatorArray<TxType>, TxType> and std::is_base_of_v<RadiatorArray<RxType>, RxType>)
                            for (std::size_t k_rx = 0; k_rx < rx_arr.elements.size(); k_rx++)
                                for (std::size_t k_tx = 0; k_tx < tx_arr.elements.size(); k_tx++)
                                    gain += tx_arr.coefficients[k_tx] *
                                        calc_voltage_gain_direct(tx_arr.elements[k_tx], rx_arr.elements[k_rx], num_params, wavelength) *
                                        rx_arr.coefficients[k_rx];
                        else
                            std::unreachable();
                        return gain;
                    },
                    tx,
                    rx);
            case Key_TxArr_RxRad:
                return std::visit(
                    [&](const auto& tx_arr)
                    {
                        using TxType = std::decay_t<decltype(tx_arr)>;
                        Complex gain = 0;
                        if constexpr (std::is_base_of_v<RadiatorArray<TxType>, TxType>)
                            for (std::size_t k = 0; k < tx_arr.elements.size(); k++)
                                gain += tx_arr.coefficients[k] * calc_voltage_gain_direct(tx_arr.elements[k], std::get<Radiator>(rx), num_params, wavelength);
                        else
                            std::unreachable();
                        return gain;
                    },
                    tx);
            case Key_TxRad_RxArr:
                return std::visit(
                    [&](const auto& rx_arr)
                    {
                        using RxType = std::decay_t<decltype(rx_arr)>;
                        Complex gain = 0;
                        if constexpr (std::is_base_of_v<RadiatorArray<RxType>, RxType>)
                            for (std::size_t k = 0; k < rx_arr.elements.size(); k++)
                                gain += calc_voltage_gain_direct(std::get<Radiator>(tx), rx_arr.elements[k], num_params, wavelength) * rx_arr.coefficients[k];
                        else
                            std::unreachable();
                        return gain;
                    },
                    rx);
                ;
            case Key_TxRad_RxRad: return calc_voltage_gain_direct(std::get<Radiator>(tx), std::get<Radiator>(rx), num_params, wavelength); ;
            default: std::unreachable();
        }
    }

    double calc_power_gain(Antenna const& tx, Antenna const& rx, setup::NumParams const& num_params, double wavelength)
    { return math::square(std::abs(calc_voltage_gain(tx, rx, num_params, wavelength))); }

    void resolve_origins(std::span<Antenna> antennas, std::span<Reference> references)
    {
        std::vector<Reference*> ref_vec = references | transform([](Reference& ref) { return std::addressof(ref); }) | to<std::vector>();
        for (Antenna& ant : antennas) { resolve_origin_impl(ant, ref_vec); }
    }

    void resolve_origins(std::span<Radiator> radiators, std::span<Reference> references)
    {
        std::vector<Reference*> ref_vec = references | transform([](Reference& ref) { return std::addressof(ref); }) | to<std::vector>();
        for (Radiator& rad : radiators) { resolve_origin_impl(rad, ref_vec); }
    }

    void resolve_origins(std::initializer_list<std::reference_wrapper<Antenna>> antennas, std::initializer_list<std::reference_wrapper<Reference>> references)
    {
        std::vector<Reference*> ref_vec = references | transform([](Reference& ref) { return std::addressof(ref); }) | to<std::vector>();
        for (auto const& ant : antennas) { resolve_origin_impl(ant, ref_vec); }
    }

    void resolve_origins(std::initializer_list<std::reference_wrapper<Radiator>> radiators, std::initializer_list<std::reference_wrapper<Reference>> references)
    {
        std::vector<Reference*> ref_vec = references | transform([](Reference& ref) { return std::addressof(ref); }) | to<std::vector>();
        for (auto const& rad : radiators) { resolve_origin_impl(rad, ref_vec); }
    }

    Antenna const& get_const(std::span<Antenna const> antennas, std::string const& id)
    {
        auto const it = std::ranges::find(antennas, id, [](auto& ant) { return std::visit([](auto& a) { return a.id; }, ant); });
        if (it == antennas.end()) { throw SimulationError("Could not find antenna with id '{}'", id); }
        return *it;
    }

    Antenna& get(std::span<Antenna> antennas, std::string const& id)
    {
        // std::as_const converts std::span<Antenna> -> std::span<Antenna const>
        // const_cast safe here because the original span contains non-const elements
        return const_cast<Antenna&>(get_const(antennas, id));
    }

} // namespace antenna
