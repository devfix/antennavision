//
// Created by Tristan Krause on 2026-07-31.
//

#include "math/functions.hpp"
#include <numbers>

extern "C" {
    // part of the Cephes library
    extern int sici(double x, double* si, double* ci);
}

namespace math
{
    std::pair<double, double> sici(double x)
    {
        double si, ci;
        ::sici(x, &si, &ci);
        return {si, ci};
    }

    double q_function(double const x)
    {
        using std::cos;
        using std::log;
        using std::sin;
        using std::numbers::egamma;
        auto const [six, cix] = math::sici(x);
        auto const [si2x, ci2x] = math::sici(2.0 * x);
        return egamma + log(x) - cix + 0.5 * sin(x) * (si2x - 2.0 * six) + 0.5 * cos(x) * (egamma + log(0.5 * x) + ci2x - 2.0 * cix);
    }
} // namespace mathh
