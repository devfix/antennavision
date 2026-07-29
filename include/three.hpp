//
// Created by Tristan Krause on 2026-06-03.
//

#pragma once

#include <ansi_color.hpp>
#include <vector>
#include "color.hpp"
#include "setup/geometry.hpp"
#include "types/json.hpp"
#include "types/math.hpp"

namespace three
{
    struct Container
    {
        void add(json&& object);
        void add(std::vector<json>&& new_objects);
        void export_to_javascript(std::filesystem::path const& p) const;

    private:
        std::list<json> objects;
    };

    [[nodiscard]] json make_line(std::vector<Pos> const& points, double width, Color color = Color::white);
    [[nodiscard]] json make_line(Pos pos_a, Pos pos_b, double width, Color color = Color::white);
    [[nodiscard]] json make_sphere(Pos const& pos, double radius, Color color = Color::white, std::uint16_t segments_width = 16, std::uint16_t segments_height = 8);
    [[nodiscard]] json make_cylinder(Pos const& pos_start, Pos const& pos_end, double radius_start, double radius_end, Color color = Color::white, std::uint16_t segments_radial = 8);
    [[nodiscard]] json make_cone(Pos const& pos_start, Pos const& pos_end, double radius, Color color = Color::white, std::uint16_t segments_radial = 8);
    [[nodiscard]] json make_plane(Pos const& pos, Pos const& dir_target, double width, double height, double angle, Color color = Color::white);
    [[nodiscard]] std::vector<json> create_arrow(Pos const& pos_start, Pos const& pos_end, double len_head, double radius_line, double radius_head, Color color = Color::white);
    [[nodiscard]] std::vector<json> create_coordinate_arrows(Pos const& pos_center, Pos const& dir_x, Pos const& dir_y, Pos const& dir_z, double len_arrow);
    [[nodiscard]] std::vector<json> export_geometry(geometry::Geometry const& geo);
} // namespace three
