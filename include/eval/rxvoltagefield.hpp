//
// Created by Tristan Krause on 2026-07-23.
//

#pragma once
#include "components/antenna.hpp"
#include "eval/scalarfield.hpp"

struct RxVoltageField : ComplexScalarField<RxVoltageField>
{
    [[nodiscard]] RxVoltageField(antenna::Antenna const& tx, antenna::Antenna const& rx, setup::NumParams const& num_params) :
        ScalarField(num_params), tx_(tx), rx_(rx)
    {
        for (const auto* origin = antenna::get_origin(rx_); origin; origin = origin->origin) references_.push_back(*origin);
        validate();
        rx_origin_ = antenna::get_origin(rx_);
    }

    void validate()
    {
        reference::resolve_origins(references_);
        antenna::resolve_origins(std::span(&rx_, 1), references_);
    }

    [[nodiscard]] Complex field_impl(Pos const& pos, double wavelength) const
    {
        rx_origin_->pos = pos;
        return antenna::calc_voltage_gain(tx_, rx_, num_params, wavelength);
    }

private:
    antenna::Antenna const& tx_; /// const reference to original tx antenna
    antenna::Antenna rx_; /// copy of the rx antenna
    std::vector<reference::Reference> references_;
    reference::Reference* rx_origin_{};
};
