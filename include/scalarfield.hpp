//
// Created by core on 04.07.26.
//

#pragma once

#include <NumCpp/Functions/abs.hpp>
#include <functional>
#include <variant>
#include "math.hpp"
#include "types.hpp"

template <typename ScalarType>
struct ScalarField
{
    using field_t = std::function<ScalarType(pos_t const& pos, double wavelength)>;
    using reset_t = std::function<void()>;
    ScalarField(std::string_view id, field_t&& field, reset_t&& reset, math::NumParams const& num_params);
    ~ScalarField();

    [[nodiscard]] std::pair<pos_t, double> argmax_line_abs(pos_t const& pos_a, pos_t const& pos_b) const;

    [[nodiscard]] std::pair<pos_t, double> argmax_circle_abs(math::Circle const& circle, double angle) const;

    [[nodiscard]] std::pair<pos_t, double> calc_beamwidth(math::Circle const& circle, double ratio) const;

    [[nodiscard]] std::pair<PositionArray, std::variant<RealArray, ComplexArray>> eval_line(pos_t const& pos_start, pos_t const& pos_end) const;

    [[nodiscard]] std::pair<PositionArray, RealArray> eval_line_abs(pos_t const& pos_start, pos_t const& pos_end) const
    {
        auto const [positions, values] = eval_line(pos_start, pos_end);
        return {positions, std::visit([](auto const& v) -> RealArray { return nc::abs(v); }, values)};
    }

    [[nodiscard]] std::pair<PositionArray, std::variant<RealArray, ComplexArray>> eval_plane(math::Rectangle const& rectangle) const;

    [[nodiscard]] std::pair<PositionArray, RealArray> eval_plane_abs(math::Rectangle const& rectangle) const
    {
        auto const [positions, values] = eval_plane(rectangle);
        return {positions, std::visit([](auto const& v) -> RealArray { return nc::abs(v); }, values)};
    }

    std::string const id;
    field_t const field;
    math::NumParams const num_params;

private:
    reset_t const reset;
};

using GenericScalarField = std::variant<ScalarField<complex_t>, ScalarField<double>>;

namespace scalarfield
{
    constexpr std::string_view get_id(GenericScalarField const& generic_scalar_field)
    {
        return std::visit([](auto const& field) -> std::string_view { return field.id; }, generic_scalar_field);
    }

    constexpr math::NumParams const& get_num_params(GenericScalarField const& generic_scalar_field)
    {
        return std::visit([](auto const& field) -> math::NumParams const& { return field.num_params; }, generic_scalar_field);
    }

    inline bool is_complex(GenericScalarField const& field) { return std::get_if<ScalarField<complex_t>>(&field); }

} // namespace scalarfield
