//
// Created by Tristan Krause on 2026-07-31.
//

#pragma once
#include "eval/scalarfield.hpp"

namespace eval
{
    struct ComplexScalarMathField : ComplexScalarField<ComplexScalarMathField>
    {
        struct Context : ComplexScalarField<ComplexScalarMathField>::Context<Context>
        {
            [[nodiscard]] explicit Context(ComplexScalarMathField const& field, double wavelength) : field(field), wavelength_(wavelength) {}

            Context(Context const&) = delete;
            Context(Context&&) = delete;
            Context& operator=(Context const&) = delete;
            Context& operator=(Context&&) = delete;

            [[nodiscard]] Complex eval(Pos const& pos) const { return field.fn_(pos, wavelength_); }

        private:
            ComplexScalarMathField const& field;
            double const wavelength_;
        };

        [[nodiscard]] ComplexScalarMathField(std::function<Complex(Pos const& pos, double wavelength)> fn, setup::NumParams const& num_params) :
            ScalarField(num_params), fn_(std::move(fn))
        {}

    private:
        std::function<Complex(Pos const& pos, double wavelength)> fn_;
    };
} // namespace eval
