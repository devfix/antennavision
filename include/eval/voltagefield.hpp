//
// Created by Tristan Krause on 2026-07-23.
//

#pragma once
#include "eval/scalarfield.hpp"
#include "components/antenna.hpp"

struct VoltageField : ScalarField<VoltageField, complex_t>
{
    /**
      * Creates new scalar field that is the voltage field if the tx is fixed in space and the rx is moved around
      * @param tx transmitter antenna
      * @param rx receiver antenna
      * @param num_params numerical parameters
      * @return voltage field between tx and rx
      */
    [[nodiscard]] VoltageField(Antenna const& tx, Antenna& rx, setup::NumParams const& num_params) :
        ScalarField(num_params), tx(tx), rx(rx)
    {}

    [[nodiscard]] complex_t field_impl(pos_t const& pos, double wavelength) const
    {
        antenna::get_origin(rx)->pos = pos;
        return antenna::calc_voltage_gain(tx, rx, num_params);
    }

    Antenna const& tx;
    Antenna& rx;
};
