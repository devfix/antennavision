//
// Created by Tristan Krause on 2026-06-03.
//

#include "math/coords.hpp"


namespace math
{
    double angle_between_vectors(Pos vec1, Pos vec2)
    {
        double const norm1 = vec1.norm();
        double const norm2 = vec2.norm();
        if (norm1 < NUMERICAL_MARGIN || norm2 < NUMERICAL_MARGIN) { return 0.0; }
        vec1 /= norm1;
        vec2 /= norm2;
        return std::atan2(vec1.cross(vec2).norm(), vec1.dot(vec2));
    }

    Pos get_ort_dir(Pos const& dir)
    {
        auto const dir_initial = dir.normalize();

        // We need to rotate around an arbitrary axis orthogonal to dir and "search" for a viable orthogonal direction
        // We create the cross-product between dir and each unit vector, these vectors our candidates
        std::array<std::tuple<Pos, double>, 3> dir_orts{{
            {dir_initial.cross(Pos(1, 0, 0)), 0},
            {dir_initial.cross(Pos(0, 1, 0)), 0},
            {dir_initial.cross(Pos(0, 0, 1)), 0} //
        }};
        // for each candidate we determine its norm
        for (auto& [v, len] : dir_orts) { len = v.norm(); }

        // we identify the candidate with the largest norm
        auto const dir_ort_best = std::get<0>(*std::max_element(dir_orts.begin(),
            dir_orts.end(),
            [](std::tuple<Pos, double> const& a, std::tuple<Pos, double> const& b) { return std::get<1>(a) < std::get<1>(b); }));

        // normalize and return the best candidate
        return dir_ort_best.normalize();
    }

    Quaternion quaternion_from_directions(Pos dir_initial, Pos dir_target)
    {
        double const angle = angle_between_vectors(dir_initial, dir_target);

        // case 1: dir_initial and dir_target are equal -> return identity quaternion
        if (std::abs(angle) < NUMERICAL_MARGIN) return {};

        // case 2: angle == +/- pi -> the rotation can take place around any orthogonal axis by angle pi
        if (std::abs(pi - std::abs(angle)) < NUMERICAL_MARGIN) return {get_ort_dir(dir_initial), nc::constants::pi};

        // case 3: angle is not special (no edge case) -> use cross product as orthogonal axis and rotate by angle
        return {dir_initial.cross(dir_target).normalize(), angle};
    }

} // namespace math
