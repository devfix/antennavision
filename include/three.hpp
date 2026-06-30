//
// Created by Tristan Krause on 2026-06-03.
//

#pragma once

#include <fmt/color.h>
#include <vector>
#include "types.hpp"

namespace three
{
    struct Container
    {
        void add(json&& object);
        void add(std::vector<json>&& objects);
        void export_to_javascript(std::filesystem::path const& p) const;

    private:
        std::list<json> objects;
    };

    [[nodiscard]] json make_sphere(pos_t const& pos, double radius, fmt::color color = fmt::color::white, std::uint16_t segments_width = 16, std::uint16_t segments_height = 8);
    [[nodiscard]] json make_cylinder(pos_t const& pos_start, pos_t const& pos_end, double radius_start, double radius_end, fmt::color color = fmt::color::white, std::uint16_t segments_radial = 8);
    [[nodiscard]] json make_cone(pos_t const& pos_start, pos_t const& pos_end, double radius, fmt::color color = fmt::color::white, std::uint16_t segments_radial = 8);
    [[nodiscard]] json make_plane(pos_t const& pos, pos_t const& dir_target, double width, double height, double angle, fmt::color color = fmt::color::white);
    [[nodiscard]] std::vector<json> create_arrow(pos_t const& pos_start, pos_t const& pos_end, double len_head, double radius_line, double radius_head, fmt::color color = fmt::color::white);
    [[nodiscard]] std::vector<json> create_coordinate_arrows(pos_t const& pos_center, pos_t const& dir_x, pos_t const& dir_y, pos_t const& dir_z, double len_arrow);
} // namespace three
