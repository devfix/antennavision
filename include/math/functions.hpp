//
// Created by Tristan Krause on 2026-07-31.
//

#pragma once
#include <cassert>
#include <cmath>

namespace math
{
    template <typename T>
    [[nodiscard]] T constexpr square(T x)
    { return x * x; }

    /**
     * normalize index
     * @tparam X type of x
     * @tparam N type of n
     * @param x values in [0, n-1]
     * @param n integer greater unity
     * @return min(max(u,0),1) with u=x/(n-1)
     */
    template <typename X, typename N>
    [[nodiscard]] constexpr double nidx(X x, N n) noexcept
    {
        assert(n > 1 && "n must be greater than 1 to form a valid interval [0, 1]");
        if (x <= 0 or n <= 1) return 0.0;
        n -= 1;
        if (x >= n) return 1.0;
        return static_cast<double>(x) / static_cast<double>(n);
    }

    /**
     * normalize index minus one
     * @tparam X type of x
     * @tparam N type of n
     * @param x values in [0, n-1]
     * @param n integer greater unity
     * @return min(max(u,0),1) with u=(x-1)/(n-1)
     */
    template <typename X, typename N>
    [[nodiscard]] constexpr double nidxm1(X x, N n) noexcept
    {
        if (x <= 0) return 0.0;
        return nidx(x - 1, n);
    }

    /**
     * normalize index plus one
     * @tparam X type of x
     * @tparam N type of n
     * @param x values in [0, n-1]
     * @param n integer greater unity
     * @return min(max(u,0),1) with u=(x+1)/(n-1)
     */
    template <typename X, typename N>
    [[nodiscard]] constexpr double nidxp1(X x, N n) noexcept
    { return nidx(x + 1, n); }

    /**
     *
     * @tparam X type of a and b
     * @tparam T type of t
     * @param a interpolation bound a
     * @param b interpolation bound b
     * @param t value in [0,1]
     * @return a+u*(b-a) with u=min(max(t,0),1)
     */
    template <typename X, typename T>
    [[nodiscard]] constexpr X lerp(X const& a, X const& b, T t) noexcept
    {
        if (t <= static_cast<T>(0)) return a;
        if (t >= static_cast<T>(1)) return b;
        return a + t * (b - a);
    }

    [[nodiscard]] double constexpr db_from_power_ratio(double const power_ratio) { return 10.0 * std::log10(power_ratio); }

    [[nodiscard]] double constexpr power_ratio_from_db(double const db) { return std::pow(10.0, db / 10.0); }

    std::pair<double, double> sici(double x);

    double q_function(double x);
} // namespace math
