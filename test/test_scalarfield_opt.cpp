//
// Created by Tristan Krause on 2026-07-31.
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <nlohmann/json.hpp>

#include "eval/complexscalarmathfield.hpp"
#include "eval/rxvoltagefield.hpp"
#include "setup/setup.hpp"
#include "testutil.hpp"
// #include <print>

// for (std::int32_t k = 0; k < positions.shape().rows; k++)
//     std::println("CHECK_THAT(std::abs(values({}, 0)), WithinAbs({:.15g}, 1e-9));", k, std::abs(values(k, 0)));
// for (std::int32_t k = 0; k < positions.shape().rows; k++)
//     std::println("CHECK_THAT(std::arg(values({}, 0)), WithinAbs({:.15g}, 1e-6));", k, std::arg(values(k, 0)));

// for (std::int32_t row = 0; row < positions.shape().rows; row++)
//     for (std::int32_t col = 0; col < positions.shape().cols; col++)
//         std::println("CHECK_THAT(std::abs(values({}, {})), WithinAbs({:.15g}, 1e-9));", row, col, std::abs(values(row, col)));
// for (std::int32_t row = 0; row < positions.shape().rows; row++)
//     for (std::int32_t col = 0; col < positions.shape().cols; col++)
//         std::println("CHECK_THAT(std::arg(values({}, {})), WithinAbs({:.15g}, 1e-6));", row, col, std::arg(values(row, col)));

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;
using geometry::Geometry;
using eval::RxVoltageField;
using eval::ComplexScalarMathField;

namespace
{
    using std::ranges::max;
    using std::ranges::transform;

    /**
     * create vector of unity coefficients (uc)
     * @param ant antenna, used to determine correct vector size
     * @return vector of ones
     */
    std::vector<Complex> uc(antenna::Antenna const& ant) { return std::vector<Complex>(antenna::size(ant), 1.0); }

    double normalize(ComplexArray& values)
    {
        double const abs_max = std::abs(max(values, {}, [](Complex const& v) -> double { return std::abs(v); }));
        transform(values, values.begin(), [abs_max](auto v) -> Complex { return v / abs_max; }); // normalize
        return abs_max;
    }
} // namespace

ojson const SETUP_JSON = ojson::parse(R"JSON(
{
  "metadata": {
    "setup_name": "test-ula",
    "version": "0.1.3"
  },
  "variables": {
    "system_wavelength": 0.1,
    "distance": 100,
    "dipole_length_tx": "system_wavelength * 0.9",
    "dipole_length_rx": "system_wavelength * 1.1"
  },
  "sim_params": {
    "system_wavelength": "system_wavelength"
  },
  "references": [
    {
      "id": "ref_ula",
      "origin": "",
      "rot": { "roll": 0, "pitch": "0.5*pi", "yaw": 0 }
    },
    {
      "id": "ref_rx",
      "origin": "",
      "pos": ["system_wavelength", 0, 0]
    }
  ],
  "antennas": [
    {
      "type": "ULA",
      "id": "ula1",
      "ref": "ref_ula",
      "spacing": "system_wavelength * 0.5",
      "size": 16,
      "rot": { "roll": 0, "pitch": "-0.5*pi", "yaw": 0 },
      "radiator": {
        "type": "StandingWaveDipole",
        "dipole_length": "dipole_length_tx"
      }
    },
    {
      "id": "receiver",
      "ref": "ref_rx",
      "type": "StandingWaveDipole",
      "dipole_length": "dipole_length_rx"
    }
  ]
}
)JSON");

TEST_CASE("ArgMax returns the correct position", "[ScalarField][VoltageField][ArgMax]")
{
    std::size_t const n_dim1 = 101;
    SECTION("Correct Maximum on geometry::Line")
    {
        setup::Setup su(SETUP_JSON);
        su.export_to_three("/home/core");
        auto& wavelength = su.sim_params().system_wavelength;
        auto const distance = su.get_double("distance");
        auto const& tx = su.get_antenna("ula1");
        auto& rx = su.get_antenna("receiver");

        auto voltage_field = RxVoltageField(tx, rx, uc(tx), uc(rx), su.sim_params(), AppParams{});
        {
            auto line = geometry::Line("", Pos(0, distance, -0.5 * distance), Pos(0, distance, 0.5 * distance));
            auto result = voltage_field.argmax_curve_abs(line, wavelength, n_dim1);
            CHECK_THAT(result.pos.x, WithinAbs(0.0, DELTA_DISTANCE));
            CHECK_THAT(result.pos.y, WithinAbs(100.0, DELTA_DISTANCE));
            CHECK_THAT(result.pos.z, WithinAbs(0, DELTA_DISTANCE));
        }
    }

    SECTION("Correct Maximum on geometry::CircleArc")
    {
        setup::Setup su(SETUP_JSON);
        auto& wavelength = su.sim_params().system_wavelength;
        auto const distance = su.get_double("distance");
        auto const& tx = su.get_antenna("ula1");
        auto& rx = su.get_antenna("receiver");

        auto voltage_field = RxVoltageField(tx, rx, uc(tx), uc(rx), su.sim_params(), AppParams{});
        {
            auto arc = geometry::CircleArc("", POS_ZERO, Pos(1.0, 0.0, 0.0), Pos(0.0, distance, 0), POS_ZERO, distance, 0.5 * pi).normalized();
            auto result = voltage_field.argmax_curve_abs(arc, wavelength, n_dim1);
            CHECK_THAT(result.pos.x, WithinAbs(0.0, DELTA_DISTANCE));
            CHECK_THAT(result.pos.y, WithinAbs(100.0, DELTA_DISTANCE));
            CHECK_THAT(result.pos.z, WithinAbs(0, DELTA_DISTANCE));
        }
    }
}

TEST_CASE("beamwidth", "[ScalarField][VoltageField][beamwidth]")
{
    setup::Setup su(SETUP_JSON);
    auto& wavelength = su.sim_params().system_wavelength;
    auto const distance = su.get_double("distance");
    auto const& tx = su.get_antenna("ula1");
    auto& rx = su.get_antenna("receiver");

    constexpr std::size_t N_POINTS = 101;

    auto voltage_field = RxVoltageField(tx, rx, uc(tx), uc(rx), su.sim_params(), AppParams{});
    {
        auto arc = geometry::CircleArc("", POS_ZERO, Pos(1.0, 0.0, 0.0), Pos(0.0, 1.0, 0.0), POS_ZERO, distance, 0.5 * pi).normalized();
        auto [pos_beam, beamwidth] = voltage_field.calc_beamwidth(arc, wavelength, sqrt2_2, N_POINTS);
        CHECK_THAT(beamwidth, WithinAbs(0.10915247360799513, DELTA_PHASE));
    }
}

TEST_CASE("Optimization Algorithms", "[ScalarField][ComplexMathField][Optimization Algorithms]")
{
    constexpr auto pos_peak = Pos(1, 2, 0);
    ComplexScalarMathField field(
        [pos_peak](Pos const& pos, [[maybe_unused]] double wavelength) -> Complex
        {
            auto const p = pos - pos_peak;
            double const r = p.norm();
            return std::exp(-r / 10.0) * std::cos(pi * p.x) * std::cos(2 * pi * p.y);
        },
        setup::SimParams{});

    geometry::Rectangle const rect("rect", POS_ZERO, Pos(0, 0, 1), Pos(1, 0, 0), Pos(0, 1, 0), 5, 5);

    SECTION("Correct Maximum on geometry::Rectangle")
    {
        constexpr std::size_t N = 33;  // we use this "strange" number to guarantee that we don't hit the maximum exactly during the pre-scan
        auto const [t1, t2, pos, peak] = field.argmax_surface_abs(rect, 0, N, N);
        CHECK_THAT(t1, WithinAbs(pos_peak.x / rect.width() + 0.5, DELTA_DISTANCE));
        CHECK_THAT(t2, WithinAbs(pos_peak.y / rect.height() + 0.5, DELTA_DISTANCE));
        CHECK_THAT((pos - pos_peak).norm(), WithinAbs(0.0, DELTA_DISTANCE));
        CHECK_THAT(peak, WithinRel(1.0, EPSILON_MAG));
    }

    SECTION("Correct Isolines")
    {
        constexpr std::size_t N = 33;  // we use this "strange" number to guarantee that we don't hit the maximum exactly during the pre-scan
        field.trace_isolines(rect, 0, sqrt2_2, N, N);
    }
}

