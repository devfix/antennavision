//
// Created by Tristan Krause on 2026-07-23.
//

#pragma once
#include <ranges>

#include "components/antenna.hpp"
#include "eval/scalarfield.hpp"

namespace eval
{

    struct RxVoltageField : ComplexScalarField<RxVoltageField>
    {
        struct Context : ComplexScalarField<RxVoltageField>::Context<Context>
        {
            [[nodiscard]] explicit Context(RxVoltageField const& field, double wavelength) :
                field(field), rx_(field.rx_), wavelength_(wavelength), num_params_(field.num_params)
            {
                for (const auto* origin = antenna::get_origin(rx_); origin; origin = origin->origin) references_.push_back(*origin);
                validate();
                rx_origin_ = antenna::get_origin(rx_);
            }

            Context(Context const&) = delete;
            Context(Context&&) = delete;
            Context& operator=(Context const&) = delete;
            Context& operator=(Context&&) = delete;

            [[nodiscard]] Complex eval(Pos const& pos) const
            {
                rx_origin_->pos = pos;
                return antenna::calc_voltage_gain(field.tx_, rx_, wavelength_, field.tx_coeffs_, field.rx_coeffs_, num_params_);
            }

        private:
            void validate()
            {
                reference::resolve_origins(references_);
                antenna::resolve_origins(std::span(&rx_, 1), references_);
            }

            RxVoltageField const& field;
            antenna::Antenna rx_; /// copy of the rx antenna
            double const wavelength_;
            setup::NumParams const& num_params_;
            std::vector<reference::Reference> references_;
            reference::Reference* rx_origin_{};
        };

        [[nodiscard]] RxVoltageField( //
            antenna::Antenna const& tx,
            antenna::Antenna const& rx,
            std::span<Complex const> tx_coeffs,
            std::span<Complex const> rx_coeffs,
            setup::NumParams const& num_params //
            ) : ScalarField(num_params), tx_(tx), rx_(rx), tx_coeffs_(tx_coeffs | std::ranges::to<decltype(tx_coeffs_)>()), rx_coeffs_(rx_coeffs | std::ranges::to<decltype(rx_coeffs_)>())
        {}

    private:
        antenna::Antenna const& tx_;
        antenna::Antenna const& rx_;
        std::vector<Complex> tx_coeffs_;
        std::vector<Complex> rx_coeffs_;
    };
} // namespace eval
