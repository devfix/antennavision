//
// Created by Tristan Krause on 2026-06-03.
//

#include "math.hpp"
#include <cmath>

extern "C" {
    extern int sici ( double x, double *si, double *ci );
}

namespace math
{
    double angle_between_vectors(Vec3 vec1, Vec3 vec2)
    {
        double const norm1 = vec1.norm();
        double const norm2 = vec2.norm();
        if (norm1 < NUMERICAL_MARGIN || norm2 < NUMERICAL_MARGIN) { return 0.0; }
        vec1 /= norm1;
        vec2 /= norm2;
        return std::atan2(vec1.cross(vec2).norm(), vec1.dot(vec2));
    }

    Quaternion quaternion_from_directions(Vec3 dir_initial, Vec3 dir_target)
    {
        double const angle = angle_between_vectors(dir_initial, dir_target);
        if (std::abs(angle) < NUMERICAL_MARGIN) { return {}; } // identity quaternion

        if (std::abs(pi - std::abs(angle)) < NUMERICAL_MARGIN)
        {
            // We need to rotate around an arbitrary axis orthogonal to dir_initial.
            dir_initial = dir_initial.normalize();

            // We "search" for a viable orthogonal direction
            std::array<std::tuple<Vec3, double>, 3> dir_orts{
                    {{dir_initial.cross(Vec3(1, 0, 0)), 0}, {dir_initial.cross(Vec3(0, 1, 0)), 0}, {dir_initial.cross(Vec3(0, 0, 1)), 0}}};
            for (auto &[dir, len] : dir_orts) { len = dir.norm(); }
            auto const dir_ort_best =
                std::get<0>(*std::max_element(dir_orts.begin(), dir_orts.end(), [](std::tuple<Vec3, double> const &a, std::tuple<Vec3, double> const &b)
                                              { return std::get<1>(a) < std::get<1>(b); }));
            return {dir_ort_best.normalize(), nc::constants::pi};
        }
        else
        {
            auto dir_ort = dir_initial.cross(dir_target); // orthogonal axis
            return {dir_ort.normalize(), angle};
        }
    }

    std::pair<double, double> sici(double x)
    {
        double si, ci;
        ::sici(x, &si, &ci);
        return {si, ci};
    }
} // namespace math
