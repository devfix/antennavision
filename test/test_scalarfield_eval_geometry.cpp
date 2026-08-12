//
// Created by Tristan Krause on 2026-07-14.
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <nlohmann/json.hpp>
#include "testutil.hpp"
#include "setup/setup.hpp"
#include "eval/rxvoltagefield.hpp"
// #include <print>

// for (std::int32_t k = 0; k < positions.shape().rows; k++)
//     std::println("CHECK_THAT(std::abs(values({}, 0)), WithinAbs({:.15g}, EPSILON_ABS));", k, std::abs(values(k, 0)));
// for (std::int32_t k = 0; k < positions.shape().rows; k++)
//     std::println("CHECK_THAT(std::arg(values({}, 0)), WithinAbs({:.15g}, DELTA_PHASE));", k, std::arg(values(k, 0)));

// for (std::int32_t row = 0; row < positions.shape().rows; row++)
//     for (std::int32_t col = 0; col < positions.shape().cols; col++)
//         std::println("CHECK_THAT(std::abs(values({}, {})), WithinAbs({:.15g}, EPSILON_ABS));", row, col, std::abs(values(row, col)));
// for (std::int32_t row = 0; row < positions.shape().rows; row++)
//     for (std::int32_t col = 0; col < positions.shape().cols; col++)
//         std::println("CHECK_THAT(std::arg(values({}, {})), WithinAbs({:.15g}, DELTA_PHASE));", row, col, std::arg(values(row, col)));

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;
using geometry::Geometry;

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

TEST_CASE("VoltageField eval_geometry and eval_geometry_sweep over all geometries", "[ScalarField][VoltageField][eval_geometry]")
{
    setup::Setup su(SETUP_JSON);
    auto& wavelength = su.sim_params().system_wavelength;
    auto const distance = su.get_double("distance");
    auto const& tx = su.get_antenna("ula1");
    auto& rx = su.get_antenna("receiver");

    auto voltage_field = eval::RxVoltageField(tx, rx, uc(tx), uc(rx), su.sim_params());
    auto& sim_params = voltage_field.sim_params;

    // Configure a simple sweep with 3 test frequencies/wavelengths
    auto const sweep = sweep::ListSweep{"test_sweep", {wavelength, 1.5 * wavelength, 2 * wavelength}};

    SECTION("Evaluation over Line geometry")
    {
        std::size_t const n_dim1 = 5;
        std::size_t const n_dim2 = 4;
        Geometry const line = geometry::Line("", Pos(0, distance, -0.5 * distance), Pos(0, distance, 0.5 * distance));

        SECTION("Single wavelength evaluation (eval_geometry)")
        {
            auto [positions, values] = voltage_field.eval_geometry(line, wavelength, n_dim1, n_dim2);

            // Verify array shapes: curves generate correct shape
            REQUIRE(positions.shape().rows == n_dim1);
            REQUIRE(positions.shape().cols == 1);
            REQUIRE(values.shape().rows == n_dim1);
            REQUIRE(values.shape().cols == 1);

            // Verify positions along the line
            CHECK_THAT(positions(0, 0).z, WithinRel(-50.0, EPSILON_MAG));
            CHECK_THAT(positions(1, 0).z, WithinRel(-25.0, EPSILON_MAG));
            CHECK_THAT(positions(2, 0).z, WithinRel(00.0, EPSILON_MAG));
            CHECK_THAT(positions(3, 0).z, WithinRel(25.0, EPSILON_MAG));
            CHECK_THAT(positions(4, 0).z, WithinRel(50.0, EPSILON_MAG));

            // First, find the maximum of the physical values and normalize for better comparison
            double const abs_max = normalize(values);
            CHECK_THAT(abs_max, WithinRel(0.00310138368013728, EPSILON_MAG));

            // Verify physical values along the curve
            CHECK_THAT(std::abs(values(0, 0)), WithinRel(0.0337295527213379, EPSILON_MAG));
            CHECK_THAT(std::abs(values(1, 0)), WithinRel(0.0238992910918213, EPSILON_MAG));
            CHECK_THAT(std::abs(values(2, 0)), WithinRel(1, EPSILON_MAG));
            CHECK_THAT(std::abs(values(3, 0)), WithinRel(0.0238992910918213, EPSILON_MAG));
            CHECK_THAT(std::abs(values(4, 0)), WithinRel(0.0337295527213378, EPSILON_MAG));
            CHECK_THAT(std::arg(values(0, 0)), WithinAbs(1.32786841400004, DELTA_PHASE));
            CHECK_THAT(std::arg(values(1, 0)), WithinAbs(3.05394203080116, DELTA_PHASE));
            CHECK_THAT(std::arg(values(2, 0)), WithinAbs(-1.58748534552425, DELTA_PHASE));
            CHECK_THAT(std::arg(values(3, 0)), WithinAbs(3.05394203080116, DELTA_PHASE));
            CHECK_THAT(std::arg(values(4, 0)), WithinAbs(1.32786841400004, DELTA_PHASE));
        }

        SECTION("Wavelength sweep evaluation (eval_geometry_sweep)")
        {
            auto [positions, data] = voltage_field.eval_geometry_sweep(line, sweep, n_dim1, n_dim2);

            // Verify array shapes: curves generate shape (n_linear1, 1)
            REQUIRE(positions.shape().rows == n_dim1);
            REQUIRE(positions.shape().cols == 1);
            REQUIRE(data.size() == sweep.size());
            for (std::size_t page = 0; page < sweep.size(); ++page)
            {
                REQUIRE(data[page].shape().rows == n_dim1);
                REQUIRE(data[page].shape().cols == 1);
            }

            // Verify positions along the line
            CHECK_THAT(positions(0, 0).z, WithinRel(-50.0, EPSILON_MAG));
            CHECK_THAT(positions(1, 0).z, WithinRel(-25.0, EPSILON_MAG));
            CHECK_THAT(positions(2, 0).z, WithinRel(00.0, EPSILON_MAG));
            CHECK_THAT(positions(3, 0).z, WithinRel(25.0, EPSILON_MAG));
            CHECK_THAT(positions(4, 0).z, WithinRel(50.0, EPSILON_MAG));

            // Verify page 0 (note: this page should be equal to the eval_geometry test)
            {
                auto& values = data[0];

                // First, find the maximum of the physical values and normalize for better comparison
                double const abs_max = normalize(values);
                CHECK_THAT(abs_max, WithinRel(0.00310138368013728, EPSILON_MAG));

                // Verify physical values along the curve
                CHECK_THAT(std::abs(values(0, 0)), WithinRel(0.0337295527213379, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 0)), WithinRel(0.0238992910918213, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 0)), WithinRel(1, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 0)), WithinRel(0.0238992910918213, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 0)), WithinRel(0.0337295527213378, EPSILON_MAG));
                CHECK_THAT(std::arg(values(0, 0)), WithinAbs(1.32786841400004, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 0)), WithinAbs(3.05394203080116, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 0)), WithinAbs(-1.58748534552425, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 0)), WithinAbs(3.05394203080116, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 0)), WithinAbs(1.32786841400004, DELTA_PHASE));
            }

            // Verify page 1
            {
                auto& values = data[1];

                // First, find the maximum of the physical values and normalize for better comparison
                double const abs_max = normalize(values);
                CHECK_THAT(abs_max, WithinRel(0.003411633888352, EPSILON_MAG));

                // Verify physical values along the curve
                CHECK_THAT(std::abs(values(0, 0)), WithinRel(0.0764583097513073, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 0)), WithinRel(0.171220984264941, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 0)), WithinRel(1, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 0)), WithinRel(0.171220984264941, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 0)), WithinRel(0.0764583097513073, EPSILON_MAG));
                CHECK_THAT(std::arg(values(0, 0)), WithinAbs(2.44837837459382, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 0)), WithinAbs(0.372559076982654, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 0)), WithinAbs(0.512472540046752, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 0)), WithinAbs(0.372559076982654, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 0)), WithinAbs(2.44837837459382, DELTA_PHASE));
            }
            //
            // Verify page 2
            {
                auto& values = data[2];

                // First, find the maximum of the physical values and normalize for better comparison
                double const abs_max = normalize(values);
                CHECK_THAT(abs_max, WithinRel(0.00418396928771996, EPSILON_MAG));

                // Verify physical values along the curve
                CHECK_THAT(std::abs(values(0, 0)), WithinRel(0.0727453682609543, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 0)), WithinRel(0.0279442337747555, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 0)), WithinRel(1, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 0)), WithinRel(0.0279442337747555, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 0)), WithinRel(0.0727453682609543, EPSILON_MAG));
                CHECK_THAT(std::arg(values(0, 0)), WithinAbs(1.46374975740342, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 0)), WithinAbs(2.45126705961933, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 0)), WithinAbs(-1.57914105201936, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 0)), WithinAbs(2.45126705961934, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 0)), WithinAbs(1.46374975740342, DELTA_PHASE));
            }
        }
    }

    SECTION("Evaluation over CircleArc geometry")
    {
        std::size_t const n_dim1 = 5;
        std::size_t const n_dim2 = 4;
        Geometry const arc = geometry::CircleArc("", POS_ZERO, Pos(1.0, 0.0, 0.0), Pos(0.0, distance, 0), POS_ZERO, distance, 0.5 * pi).normalized();

        SECTION("Single wavelength evaluation (eval_geometry)")
        {
            auto [positions, values] = voltage_field.eval_geometry(arc, wavelength, n_dim1, n_dim2);

            // Verify array shapes: curves generate shape (n_linear1, 1)
            REQUIRE(positions.shape().rows == n_dim1);
            REQUIRE(positions.shape().cols == 1);
            REQUIRE(values.shape().rows == n_dim1);
            REQUIRE(values.shape().cols == 1);

            // Verify position mapping along the arc (-90 deg to +90 deg)
            CHECK_THAT(positions(0, 0).z, WithinRel(-70.71067811865474084, EPSILON_MAG)); // t = 0.0 -> -pi/2
            CHECK_THAT(positions(1, 0).z, WithinRel(-38.26834323650897574, EPSILON_MAG)); // t = 0.25
            CHECK_THAT(positions(2, 0).z, WithinRel(0.0, EPSILON_MAG)); // t = 0.5 -> 0 rad
            CHECK_THAT(positions(3, 0).z, WithinRel(38.26834323650897574, EPSILON_MAG)); // t = 0.75
            CHECK_THAT(positions(4, 0).z, WithinRel(70.71067811865474084, EPSILON_MAG)); // t = 1.0 -> +pi/2

            // First, find the maximum of the physical values and normalize for better comparison
            double const abs_max = normalize(values);
            CHECK_THAT(abs_max, WithinRel(0.00310138368013728, EPSILON_MAG));

            // Verify physical values along the curve
            CHECK_THAT(std::abs(values(0, 0)), WithinRel(0.00320028288208625, EPSILON_MAG));
            CHECK_THAT(std::arg(values(0, 0)), WithinRel(1.56412519106567638, EPSILON_MAG));
            CHECK_THAT(std::abs(values(1, 0)), WithinRel(0.01116871931989499, EPSILON_MAG));
            CHECK_THAT(std::arg(values(1, 0)), WithinRel(1.4187912238163809, EPSILON_MAG));
            CHECK_THAT(std::abs(values(2, 0)), WithinRel(1.0, EPSILON_MAG));
            CHECK_THAT(std::arg(values(2, 0)), WithinAbs(-1.58748534552424547, DELTA_PHASE));
            CHECK_THAT(std::abs(values(3, 0)), WithinAbs(0.01116871931989495, DELTA_PHASE));
            CHECK_THAT(std::arg(values(3, 0)), WithinAbs(1.41879122381637823, DELTA_PHASE));
            CHECK_THAT(std::abs(values(4, 0)), WithinAbs(0.00320028288208626, DELTA_PHASE));
            CHECK_THAT(std::arg(values(4, 0)), WithinAbs(1.56412519106567482, DELTA_PHASE));
        }

        SECTION("Wavelength sweep evaluation (eval_geometry_sweep)")
        {
            auto [positions, data] = voltage_field.eval_geometry_sweep(arc, sweep, n_dim1, n_dim2);

            // Verify array shapes: curves generate shape (n_linear1, 1)
            REQUIRE(positions.shape().rows == n_dim1);
            REQUIRE(positions.shape().cols == 1);
            REQUIRE(data.size() == sweep.size());
            for (std::size_t page = 0; page < sweep.size(); ++page)
            {
                REQUIRE(data[page].shape().rows == n_dim1);
                REQUIRE(data[page].shape().cols == 1);
            }

            // Verify position mapping along the arc (-90 deg to +90 deg)
            CHECK_THAT(positions(0, 0).z, WithinRel(-70.71067811865474084, EPSILON_MAG)); // t = 0.0 -> -pi/2
            CHECK_THAT(positions(1, 0).z, WithinRel(-38.26834323650897574, EPSILON_MAG)); // t = 0.25
            CHECK_THAT(positions(2, 0).z, WithinRel(0.0, EPSILON_MAG)); // t = 0.5 -> 0 rad
            CHECK_THAT(positions(3, 0).z, WithinRel(38.26834323650897574, EPSILON_MAG)); // t = 0.75
            CHECK_THAT(positions(4, 0).z, WithinRel(70.71067811865474084, EPSILON_MAG)); // t = 1.0 -> +pi/2

            // Verify page 0 (note: this page should be equal to the eval_geometry test)
            {
                auto& values = data[0];

                // First, find the maximum of the physical values and normalize for better comparison
                double const abs_max = normalize(values);
                CHECK_THAT(abs_max, WithinRel(0.00310138368013728, EPSILON_MAG));

                // Verify physical values along the curve
                CHECK_THAT(std::abs(values(0, 0)), WithinRel(0.00320028288208625, EPSILON_MAG));
                CHECK_THAT(std::arg(values(0, 0)), WithinRel(1.56412519106567638, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 0)), WithinRel(0.01116871931989499, EPSILON_MAG));
                CHECK_THAT(std::arg(values(1, 0)), WithinRel(1.4187912238163809, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 0)), WithinRel(1.0, EPSILON_MAG));
                CHECK_THAT(std::arg(values(2, 0)), WithinAbs(-1.58748534552424547, DELTA_PHASE));
                CHECK_THAT(std::abs(values(3, 0)), WithinAbs(0.01116871931989495, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 0)), WithinAbs(1.41879122381637823, DELTA_PHASE));
                CHECK_THAT(std::abs(values(4, 0)), WithinAbs(0.00320028288208626, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 0)), WithinAbs(1.56412519106567482, DELTA_PHASE));
            }

            // Verify page 1
            {
                auto& values = data[1];

                // First, find the maximum of the physical values and normalize for better comparison
                double const abs_max = normalize(values);
                CHECK_THAT(abs_max, WithinRel(0.003411633888352, EPSILON_MAG));

                // Verify physical values along the curve
                CHECK_THAT(std::abs(values(0, 0)), WithinRel(0.01858547187923242, EPSILON_MAG));
                CHECK_THAT(std::arg(values(0, 0)), WithinRel(-2.61806589941934931, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 0)), WithinRel(0.01536786526331465, EPSILON_MAG));
                CHECK_THAT(std::arg(values(1, 0)), WithinRel(0.37770760072447473, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 0)), WithinRel(1.0, EPSILON_MAG));
                CHECK_THAT(std::arg(values(2, 0)), WithinAbs(0.51247254004675225, DELTA_PHASE));
                CHECK_THAT(std::abs(values(3, 0)), WithinAbs(0.01536786526331473, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 0)), WithinAbs(0.37770760072447612, DELTA_PHASE));
                CHECK_THAT(std::abs(values(4, 0)), WithinAbs(0.01858547187923243, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 0)), WithinAbs(-2.61806589941934886, DELTA_PHASE));
            }

            // Verify page 2
            {
                auto& values = data[2];

                // First, find the maximum of the physical values and normalize for better comparison
                double const abs_max = normalize(values);
                CHECK_THAT(abs_max, WithinRel(0.00418396928771996, EPSILON_MAG));

                // Verify physical values along the curve
                CHECK_THAT(std::abs(values(0, 0)), WithinRel(0.02389331153927321, EPSILON_MAG));
                CHECK_THAT(std::arg(values(0, 0)), WithinRel(-1.56131841583061837, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 0)), WithinRel(0.16726023674013196, EPSILON_MAG));
                CHECK_THAT(std::arg(values(1, 0)), WithinRel(1.55375648968733615, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 0)), WithinRel(1.0, EPSILON_MAG));
                CHECK_THAT(std::arg(values(2, 0)), WithinAbs(-1.5791410520193585, DELTA_PHASE));
                CHECK_THAT(std::abs(values(3, 0)), WithinAbs(0.16726023674013196, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 0)), WithinAbs(1.55375648968733637, DELTA_PHASE));
                CHECK_THAT(std::abs(values(4, 0)), WithinAbs(0.02389331153927323, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 0)), WithinAbs(-1.56131841583061881, DELTA_PHASE));
            }
        }
    }

    SECTION("Evaluation over Rectangle geometry")
    {
        std::size_t const n_dim1 = 16;
        std::size_t const n_dim2 = 8;
        Geometry const rect = geometry::Rectangle{"rect_01", {0, distance, 0}, {0, 1, 0}, {-1, 0, 0}, {0, 0, 1}, 2 * distance, distance};

        SECTION("Single wavelength evaluation (eval_geometry)")
        {
            auto [positions, values] = voltage_field.eval_geometry(rect, wavelength, n_dim1, n_dim2);

            // Verify array shapes: surfaces generate correct shape
            REQUIRE(positions.shape().rows == n_dim1);
            REQUIRE(positions.shape().cols == n_dim2);
            REQUIRE(values.shape().rows == n_dim1);
            REQUIRE(values.shape().cols == n_dim2);

            // Verify grid corner positions
            CHECK_THAT(positions(0, 0).x, WithinRel(100, EPSILON_MAG));
            CHECK_THAT(positions(0, 0).y, WithinRel(100, EPSILON_MAG));
            CHECK_THAT(positions(0, 0).z, WithinRel(-50, EPSILON_MAG));
            CHECK_THAT(positions(positions.shape().rows - 1, 0).x, WithinRel(-100, EPSILON_MAG));
            CHECK_THAT(positions(positions.shape().rows - 1, 0).y, WithinRel(100, EPSILON_MAG));
            CHECK_THAT(positions(positions.shape().rows - 1, 0).z, WithinRel(-50, EPSILON_MAG));
            CHECK_THAT(positions(0, positions.shape().cols - 1).x, WithinRel(100, EPSILON_MAG));
            CHECK_THAT(positions(0, positions.shape().cols - 1).y, WithinRel(100, EPSILON_MAG));
            CHECK_THAT(positions(0, positions.shape().cols - 1).z, WithinRel(50, EPSILON_MAG));
            CHECK_THAT(positions(positions.shape().rows - 1, positions.shape().cols - 1).x, WithinRel(-100, EPSILON_MAG));
            CHECK_THAT(positions(positions.shape().rows - 1, positions.shape().cols - 1).y, WithinRel(100, EPSILON_MAG));
            CHECK_THAT(positions(positions.shape().rows - 1, positions.shape().cols - 1).z, WithinRel(50, EPSILON_MAG));

            // Next, find the maximum of the physical values and normalize for better comparison
            double const abs_max = normalize(values);
            CHECK_THAT(abs_max, WithinRel(0.00170301857371513, EPSILON_MAG));

            // Verify physical values along the surface
            CHECK_THAT(std::abs(values(0, 0)), WithinRel(0.0811805333041281, EPSILON_MAG));
            CHECK_THAT(std::abs(values(0, 1)), WithinRel(0.0209935151691007, EPSILON_MAG));
            CHECK_THAT(std::abs(values(0, 2)), WithinRel(0.181776966584661, EPSILON_MAG));
            CHECK_THAT(std::abs(values(0, 3)), WithinRel(0.959227993440513, EPSILON_MAG));
            CHECK_THAT(std::abs(values(0, 4)), WithinRel(0.959227993440513, EPSILON_MAG));
            CHECK_THAT(std::abs(values(0, 5)), WithinRel(0.181776966584661, EPSILON_MAG));
            CHECK_THAT(std::abs(values(0, 6)), WithinRel(0.0209935151689359, EPSILON_MAG));
            CHECK_THAT(std::abs(values(0, 7)), WithinRel(0.0811805333041281, EPSILON_MAG));
            CHECK_THAT(std::abs(values(1, 0)), WithinRel(0.0455986942249276, EPSILON_MAG));
            CHECK_THAT(std::abs(values(1, 1)), WithinRel(0.0411457551690019, EPSILON_MAG));
            CHECK_THAT(std::abs(values(1, 2)), WithinRel(0.236208994298118, EPSILON_MAG));
            CHECK_THAT(std::abs(values(1, 3)), WithinRel(0.980316457457755, EPSILON_MAG));
            CHECK_THAT(std::abs(values(1, 4)), WithinRel(0.980316457457755, EPSILON_MAG));
            CHECK_THAT(std::abs(values(1, 5)), WithinRel(0.236208994298118, EPSILON_MAG));
            CHECK_THAT(std::abs(values(1, 6)), WithinRel(0.0411457551690372, EPSILON_MAG));
            CHECK_THAT(std::abs(values(1, 7)), WithinRel(0.0455986942249277, EPSILON_MAG));
            CHECK_THAT(std::abs(values(2, 0)), WithinRel(0.00257584589170965, EPSILON_MAG));
            CHECK_THAT(std::abs(values(2, 1)), WithinRel(0.0941592075283062, EPSILON_MAG));
            CHECK_THAT(std::abs(values(2, 2)), WithinRel(0.275478907113585, EPSILON_MAG));
            CHECK_THAT(std::abs(values(2, 3)), WithinRel(0.99429542171149, EPSILON_MAG));
            CHECK_THAT(std::abs(values(2, 4)), WithinRel(0.99429542171149, EPSILON_MAG));
            CHECK_THAT(std::abs(values(2, 5)), WithinRel(0.275478907113585, EPSILON_MAG));
            CHECK_THAT(std::abs(values(2, 6)), WithinRel(0.0941592075283062, EPSILON_MAG));
            CHECK_THAT(std::abs(values(2, 7)), WithinRel(0.00257584589170968, EPSILON_MAG));
            CHECK_THAT(std::abs(values(3, 0)), WithinRel(0.0357596041928141, EPSILON_MAG));
            CHECK_THAT(std::abs(values(3, 1)), WithinRel(0.128220787975769, EPSILON_MAG));
            CHECK_THAT(std::abs(values(3, 2)), WithinRel(0.294897071700718, EPSILON_MAG));
            CHECK_THAT(std::abs(values(3, 3)), WithinRel(1, EPSILON_MAG));
            CHECK_THAT(std::abs(values(3, 4)), WithinRel(1, EPSILON_MAG));
            CHECK_THAT(std::abs(values(3, 5)), WithinRel(0.294897071700718, EPSILON_MAG));
            CHECK_THAT(std::abs(values(3, 6)), WithinRel(0.128220787975793, EPSILON_MAG));
            CHECK_THAT(std::abs(values(3, 7)), WithinRel(0.0357596041928141, EPSILON_MAG));
            CHECK_THAT(std::abs(values(4, 0)), WithinRel(0.0589060465262149, EPSILON_MAG));
            CHECK_THAT(std::abs(values(4, 1)), WithinRel(0.139456471473219, EPSILON_MAG));
            CHECK_THAT(std::abs(values(4, 2)), WithinRel(0.294128532445929, EPSILON_MAG));
            CHECK_THAT(std::abs(values(4, 3)), WithinRel(0.997606657015968, EPSILON_MAG));
            CHECK_THAT(std::abs(values(4, 4)), WithinRel(0.997606657015968, EPSILON_MAG));
            CHECK_THAT(std::abs(values(4, 5)), WithinRel(0.294128532445929, EPSILON_MAG));
            CHECK_THAT(std::abs(values(4, 6)), WithinRel(0.139456471473219, EPSILON_MAG));
            CHECK_THAT(std::abs(values(4, 7)), WithinRel(0.0589060465262149, EPSILON_MAG));
            CHECK_THAT(std::abs(values(5, 0)), WithinRel(0.0665616877563288, EPSILON_MAG));
            CHECK_THAT(std::abs(values(5, 1)), WithinRel(0.132742825255253, EPSILON_MAG));
            CHECK_THAT(std::abs(values(5, 2)), WithinRel(0.27873146020983, EPSILON_MAG));
            CHECK_THAT(std::abs(values(5, 3)), WithinRel(0.989316385385742, EPSILON_MAG));
            CHECK_THAT(std::abs(values(5, 4)), WithinRel(0.989316385385742, EPSILON_MAG));
            CHECK_THAT(std::abs(values(5, 5)), WithinRel(0.27873146020983, EPSILON_MAG));
            CHECK_THAT(std::abs(values(5, 6)), WithinRel(0.13274282525528, EPSILON_MAG));
            CHECK_THAT(std::abs(values(5, 7)), WithinRel(0.0665616877563289, EPSILON_MAG));
            CHECK_THAT(std::abs(values(6, 0)), WithinRel(0.0650061963206579, EPSILON_MAG));
            CHECK_THAT(std::abs(values(6, 1)), WithinRel(0.118932954431791, EPSILON_MAG));
            CHECK_THAT(std::abs(values(6, 2)), WithinRel(0.258936075347669, EPSILON_MAG));
            CHECK_THAT(std::abs(values(6, 3)), WithinRel(0.979344504415583, EPSILON_MAG));
            CHECK_THAT(std::abs(values(6, 4)), WithinRel(0.979344504415583, EPSILON_MAG));
            CHECK_THAT(std::abs(values(6, 5)), WithinRel(0.258936075347669, EPSILON_MAG));
            CHECK_THAT(std::abs(values(6, 6)), WithinRel(0.118932954431791, EPSILON_MAG));
            CHECK_THAT(std::abs(values(6, 7)), WithinRel(0.0650061963206579, EPSILON_MAG));
            CHECK_THAT(std::abs(values(7, 0)), WithinRel(0.0619228784388384, EPSILON_MAG));
            CHECK_THAT(std::abs(values(7, 1)), WithinRel(0.108899738482795, EPSILON_MAG));
            CHECK_THAT(std::abs(values(7, 2)), WithinRel(0.245612300311409, EPSILON_MAG));
            CHECK_THAT(std::abs(values(7, 3)), WithinRel(0.972648506328136, EPSILON_MAG));
            CHECK_THAT(std::abs(values(7, 4)), WithinRel(0.972648506328136, EPSILON_MAG));
            CHECK_THAT(std::abs(values(7, 5)), WithinRel(0.245612300311409, EPSILON_MAG));
            CHECK_THAT(std::abs(values(7, 6)), WithinRel(0.108899738482732, EPSILON_MAG));
            CHECK_THAT(std::abs(values(7, 7)), WithinRel(0.0619228784388384, EPSILON_MAG));
            CHECK_THAT(std::abs(values(8, 0)), WithinRel(0.0619228784388384, EPSILON_MAG));
            CHECK_THAT(std::abs(values(8, 1)), WithinRel(0.108899738482794, EPSILON_MAG));
            CHECK_THAT(std::abs(values(8, 2)), WithinRel(0.245612300311409, EPSILON_MAG));
            CHECK_THAT(std::abs(values(8, 3)), WithinRel(0.972648506328136, EPSILON_MAG));
            CHECK_THAT(std::abs(values(8, 4)), WithinRel(0.972648506328136, EPSILON_MAG));
            CHECK_THAT(std::abs(values(8, 5)), WithinRel(0.245612300311409, EPSILON_MAG));
            CHECK_THAT(std::abs(values(8, 6)), WithinRel(0.108899738482732, EPSILON_MAG));
            CHECK_THAT(std::abs(values(8, 7)), WithinRel(0.0619228784388384, EPSILON_MAG));
            CHECK_THAT(std::abs(values(9, 0)), WithinRel(0.0650061963206579, EPSILON_MAG));
            CHECK_THAT(std::abs(values(9, 1)), WithinRel(0.118932954431791, EPSILON_MAG));
            CHECK_THAT(std::abs(values(9, 2)), WithinRel(0.258936075347669, EPSILON_MAG));
            CHECK_THAT(std::abs(values(9, 3)), WithinRel(0.979344504415583, EPSILON_MAG));
            CHECK_THAT(std::abs(values(9, 4)), WithinRel(0.979344504415583, EPSILON_MAG));
            CHECK_THAT(std::abs(values(9, 5)), WithinRel(0.258936075347669, EPSILON_MAG));
            CHECK_THAT(std::abs(values(9, 6)), WithinRel(0.118932954431791, EPSILON_MAG));
            CHECK_THAT(std::abs(values(9, 7)), WithinRel(0.0650061963206579, EPSILON_MAG));
            CHECK_THAT(std::abs(values(10, 0)), WithinRel(0.0665616877563289, EPSILON_MAG));
            CHECK_THAT(std::abs(values(10, 1)), WithinRel(0.132742825255253, EPSILON_MAG));
            CHECK_THAT(std::abs(values(10, 2)), WithinRel(0.27873146020983, EPSILON_MAG));
            CHECK_THAT(std::abs(values(10, 3)), WithinRel(0.989316385385742, EPSILON_MAG));
            CHECK_THAT(std::abs(values(10, 4)), WithinRel(0.989316385385742, EPSILON_MAG));
            CHECK_THAT(std::abs(values(10, 5)), WithinRel(0.27873146020983, EPSILON_MAG));
            CHECK_THAT(std::abs(values(10, 6)), WithinRel(0.13274282525528, EPSILON_MAG));
            CHECK_THAT(std::abs(values(10, 7)), WithinRel(0.0665616877563289, EPSILON_MAG));
            CHECK_THAT(std::abs(values(11, 0)), WithinRel(0.0589060465262149, EPSILON_MAG));
            CHECK_THAT(std::abs(values(11, 1)), WithinRel(0.139456471473219, EPSILON_MAG));
            CHECK_THAT(std::abs(values(11, 2)), WithinRel(0.294128532445929, EPSILON_MAG));
            CHECK_THAT(std::abs(values(11, 3)), WithinRel(0.997606657015968, EPSILON_MAG));
            CHECK_THAT(std::abs(values(11, 4)), WithinRel(0.997606657015968, EPSILON_MAG));
            CHECK_THAT(std::abs(values(11, 5)), WithinRel(0.294128532445929, EPSILON_MAG));
            CHECK_THAT(std::abs(values(11, 6)), WithinRel(0.139456471473219, EPSILON_MAG));
            CHECK_THAT(std::abs(values(11, 7)), WithinRel(0.058906046526215, EPSILON_MAG));
            CHECK_THAT(std::abs(values(12, 0)), WithinRel(0.0357596041928872, EPSILON_MAG));
            CHECK_THAT(std::abs(values(12, 1)), WithinRel(0.128220787975745, EPSILON_MAG));
            CHECK_THAT(std::abs(values(12, 2)), WithinRel(0.294897071701048, EPSILON_MAG));
            CHECK_THAT(std::abs(values(12, 3)), WithinRel(0.999999999999537, EPSILON_MAG));
            CHECK_THAT(std::abs(values(12, 4)), WithinRel(0.999999999999537, EPSILON_MAG));
            CHECK_THAT(std::abs(values(12, 5)), WithinRel(0.294897071701048, EPSILON_MAG));
            CHECK_THAT(std::abs(values(12, 6)), WithinRel(0.12822078797574, EPSILON_MAG));
            CHECK_THAT(std::abs(values(12, 7)), WithinRel(0.0357596041928873, EPSILON_MAG));
            CHECK_THAT(std::abs(values(13, 0)), WithinRel(0.00257584589170966, EPSILON_MAG));
            CHECK_THAT(std::abs(values(13, 1)), WithinRel(0.0941592075283062, EPSILON_MAG));
            CHECK_THAT(std::abs(values(13, 2)), WithinRel(0.275478907113585, EPSILON_MAG));
            CHECK_THAT(std::abs(values(13, 3)), WithinRel(0.99429542171149, EPSILON_MAG));
            CHECK_THAT(std::abs(values(13, 4)), WithinRel(0.99429542171149, EPSILON_MAG));
            CHECK_THAT(std::abs(values(13, 5)), WithinRel(0.275478907113585, EPSILON_MAG));
            CHECK_THAT(std::abs(values(13, 6)), WithinRel(0.0941592075283062, EPSILON_MAG));
            CHECK_THAT(std::abs(values(13, 7)), WithinRel(0.00257584589170969, EPSILON_MAG));
            CHECK_THAT(std::abs(values(14, 0)), WithinRel(0.0455986942249276, EPSILON_MAG));
            CHECK_THAT(std::abs(values(14, 1)), WithinRel(0.0411457551690019, EPSILON_MAG));
            CHECK_THAT(std::abs(values(14, 2)), WithinRel(0.236208994298118, EPSILON_MAG));
            CHECK_THAT(std::abs(values(14, 3)), WithinRel(0.980316457457755, EPSILON_MAG));
            CHECK_THAT(std::abs(values(14, 4)), WithinRel(0.980316457457755, EPSILON_MAG));
            CHECK_THAT(std::abs(values(14, 5)), WithinRel(0.236208994298118, EPSILON_MAG));
            CHECK_THAT(std::abs(values(14, 6)), WithinRel(0.0411457551690372, EPSILON_MAG));
            CHECK_THAT(std::abs(values(14, 7)), WithinRel(0.0455986942249276, EPSILON_MAG));
            CHECK_THAT(std::abs(values(15, 0)), WithinRel(0.0811805333041281, EPSILON_MAG));
            CHECK_THAT(std::abs(values(15, 1)), WithinRel(0.0209935151691007, EPSILON_MAG));
            CHECK_THAT(std::abs(values(15, 2)), WithinRel(0.181776966584661, EPSILON_MAG));
            CHECK_THAT(std::abs(values(15, 3)), WithinRel(0.959227993440513, EPSILON_MAG));
            CHECK_THAT(std::abs(values(15, 4)), WithinRel(0.959227993440513, EPSILON_MAG));
            CHECK_THAT(std::abs(values(15, 5)), WithinRel(0.181776966584661, EPSILON_MAG));
            CHECK_THAT(std::abs(values(15, 6)), WithinRel(0.0209935151689359, EPSILON_MAG));
            CHECK_THAT(std::abs(values(15, 7)), WithinRel(0.0811805333041281, EPSILON_MAG));
            CHECK_THAT(std::arg(values(0, 0)), WithinAbs(-1.59046134379987, DELTA_PHASE));
            CHECK_THAT(std::arg(values(0, 1)), WithinAbs(-2.18508268543536, DELTA_PHASE));
            CHECK_THAT(std::arg(values(0, 2)), WithinAbs(-0.724963766234791, DELTA_PHASE));
            CHECK_THAT(std::arg(values(0, 3)), WithinAbs(-1.68114156649414, DELTA_PHASE));
            CHECK_THAT(std::arg(values(0, 4)), WithinAbs(-1.68114156649414, DELTA_PHASE));
            CHECK_THAT(std::arg(values(0, 5)), WithinAbs(-0.724963766234791, DELTA_PHASE));
            CHECK_THAT(std::arg(values(0, 6)), WithinAbs(-2.18508268543734, DELTA_PHASE));
            CHECK_THAT(std::arg(values(0, 7)), WithinAbs(-1.59046134379987, DELTA_PHASE));
            CHECK_THAT(std::arg(values(1, 0)), WithinAbs(0.8988282123476, DELTA_PHASE));
            CHECK_THAT(std::arg(values(1, 1)), WithinAbs(0.57912013854105, DELTA_PHASE));
            CHECK_THAT(std::arg(values(1, 2)), WithinAbs(-1.82938225706847, DELTA_PHASE));
            CHECK_THAT(std::arg(values(1, 3)), WithinAbs(-2.9733018895961, DELTA_PHASE));
            CHECK_THAT(std::arg(values(1, 4)), WithinAbs(-2.9733018895961, DELTA_PHASE));
            CHECK_THAT(std::arg(values(1, 5)), WithinAbs(-1.82938225706847, DELTA_PHASE));
            CHECK_THAT(std::arg(values(1, 6)), WithinAbs(0.579120138543666, DELTA_PHASE));
            CHECK_THAT(std::arg(values(1, 7)), WithinAbs(0.898828212347601, DELTA_PHASE));
            CHECK_THAT(std::arg(values(2, 0)), WithinAbs(-1.53877106842563, DELTA_PHASE));
            CHECK_THAT(std::arg(values(2, 1)), WithinAbs(1.66489040853495, DELTA_PHASE));
            CHECK_THAT(std::arg(values(2, 2)), WithinAbs(-1.2999218759808, DELTA_PHASE));
            CHECK_THAT(std::arg(values(2, 3)), WithinAbs(-2.37743568424626, DELTA_PHASE));
            CHECK_THAT(std::arg(values(2, 4)), WithinAbs(-2.37743568424626, DELTA_PHASE));
            CHECK_THAT(std::arg(values(2, 5)), WithinAbs(-1.2999218759808, DELTA_PHASE));
            CHECK_THAT(std::arg(values(2, 6)), WithinAbs(1.66489040853494, DELTA_PHASE));
            CHECK_THAT(std::arg(values(2, 7)), WithinAbs(-1.53877106842561, DELTA_PHASE));
            CHECK_THAT(std::arg(values(3, 0)), WithinAbs(2.39951755383822, DELTA_PHASE));
            CHECK_THAT(std::arg(values(3, 1)), WithinAbs(0.570607536876258, DELTA_PHASE));
            CHECK_THAT(std::arg(values(3, 2)), WithinAbs(-2.9570550451856, DELTA_PHASE));
            CHECK_THAT(std::arg(values(3, 3)), WithinAbs(2.34375554995525, DELTA_PHASE));
            CHECK_THAT(std::arg(values(3, 4)), WithinAbs(2.34375554995525, DELTA_PHASE));
            CHECK_THAT(std::arg(values(3, 5)), WithinAbs(-2.9570550451856, DELTA_PHASE));
            CHECK_THAT(std::arg(values(3, 6)), WithinAbs(0.570607536876678, DELTA_PHASE));
            CHECK_THAT(std::arg(values(3, 7)), WithinAbs(2.39951755383822, DELTA_PHASE));
            CHECK_THAT(std::arg(values(4, 0)), WithinAbs(-1.73470220406938, DELTA_PHASE));
            CHECK_THAT(std::arg(values(4, 1)), WithinAbs(-0.875285092401346, DELTA_PHASE));
            CHECK_THAT(std::arg(values(4, 2)), WithinAbs(0.639784789812871, DELTA_PHASE));
            CHECK_THAT(std::arg(values(4, 3)), WithinAbs(-0.565601279784796, DELTA_PHASE));
            CHECK_THAT(std::arg(values(4, 4)), WithinAbs(-0.565601279784796, DELTA_PHASE));
            CHECK_THAT(std::arg(values(4, 5)), WithinAbs(0.639784789812871, DELTA_PHASE));
            CHECK_THAT(std::arg(values(4, 6)), WithinAbs(-0.875285092401345, DELTA_PHASE));
            CHECK_THAT(std::arg(values(4, 7)), WithinAbs(-1.73470220406938, DELTA_PHASE));
            CHECK_THAT(std::arg(values(5, 0)), WithinAbs(-2.65587357462535, DELTA_PHASE));
            CHECK_THAT(std::arg(values(5, 1)), WithinAbs(-1.30302497013105, DELTA_PHASE));
            CHECK_THAT(std::arg(values(5, 2)), WithinAbs(-2.56513148566834, DELTA_PHASE));
            CHECK_THAT(std::arg(values(5, 3)), WithinAbs(1.50288452264914, DELTA_PHASE));
            CHECK_THAT(std::arg(values(5, 4)), WithinAbs(1.50288452264914, DELTA_PHASE));
            CHECK_THAT(std::arg(values(5, 5)), WithinAbs(-2.56513148566834, DELTA_PHASE));
            CHECK_THAT(std::arg(values(5, 6)), WithinAbs(-1.30302497013065, DELTA_PHASE));
            CHECK_THAT(std::arg(values(5, 7)), WithinAbs(-2.65587357462535, DELTA_PHASE));
            CHECK_THAT(std::arg(values(6, 0)), WithinAbs(2.90989473171143, DELTA_PHASE));
            CHECK_THAT(std::arg(values(6, 1)), WithinAbs(1.33584297314295, DELTA_PHASE));
            CHECK_THAT(std::arg(values(6, 2)), WithinAbs(1.07597558528228, DELTA_PHASE));
            CHECK_THAT(std::arg(values(6, 3)), WithinAbs(2.80787643375169, DELTA_PHASE));
            CHECK_THAT(std::arg(values(6, 4)), WithinAbs(2.80787643375169, DELTA_PHASE));
            CHECK_THAT(std::arg(values(6, 5)), WithinAbs(1.07597558528228, DELTA_PHASE));
            CHECK_THAT(std::arg(values(6, 6)), WithinAbs(1.33584297314295, DELTA_PHASE));
            CHECK_THAT(std::arg(values(6, 7)), WithinAbs(2.90989473171143, DELTA_PHASE));
            CHECK_THAT(std::arg(values(7, 0)), WithinAbs(1.41636191652562, DELTA_PHASE));
            CHECK_THAT(std::arg(values(7, 1)), WithinAbs(-1.29995402322705, DELTA_PHASE));
            CHECK_THAT(std::arg(values(7, 2)), WithinAbs(2.34715161578265, DELTA_PHASE));
            CHECK_THAT(std::arg(values(7, 3)), WithinAbs(-0.0792612673224425, DELTA_PHASE));
            CHECK_THAT(std::arg(values(7, 4)), WithinAbs(-0.0792612673224425, DELTA_PHASE));
            CHECK_THAT(std::arg(values(7, 5)), WithinAbs(2.34715161578265, DELTA_PHASE));
            CHECK_THAT(std::arg(values(7, 6)), WithinAbs(-1.29995402322705, DELTA_PHASE));
            CHECK_THAT(std::arg(values(7, 7)), WithinAbs(1.41636191652563, DELTA_PHASE));
            CHECK_THAT(std::arg(values(8, 0)), WithinAbs(1.41636191652562, DELTA_PHASE));
            CHECK_THAT(std::arg(values(8, 1)), WithinAbs(-1.29995402322705, DELTA_PHASE));
            CHECK_THAT(std::arg(values(8, 2)), WithinAbs(2.34715161578265, DELTA_PHASE));
            CHECK_THAT(std::arg(values(8, 3)), WithinAbs(-0.0792612673224425, DELTA_PHASE));
            CHECK_THAT(std::arg(values(8, 4)), WithinAbs(-0.0792612673224425, DELTA_PHASE));
            CHECK_THAT(std::arg(values(8, 5)), WithinAbs(2.34715161578265, DELTA_PHASE));
            CHECK_THAT(std::arg(values(8, 6)), WithinAbs(-1.29995402322705, DELTA_PHASE));
            CHECK_THAT(std::arg(values(8, 7)), WithinAbs(1.41636191652563, DELTA_PHASE));
            CHECK_THAT(std::arg(values(9, 0)), WithinAbs(2.90989473171143, DELTA_PHASE));
            CHECK_THAT(std::arg(values(9, 1)), WithinAbs(1.33584297314295, DELTA_PHASE));
            CHECK_THAT(std::arg(values(9, 2)), WithinAbs(1.07597558528228, DELTA_PHASE));
            CHECK_THAT(std::arg(values(9, 3)), WithinAbs(2.80787643375169, DELTA_PHASE));
            CHECK_THAT(std::arg(values(9, 4)), WithinAbs(2.80787643375169, DELTA_PHASE));
            CHECK_THAT(std::arg(values(9, 5)), WithinAbs(1.07597558528228, DELTA_PHASE));
            CHECK_THAT(std::arg(values(9, 6)), WithinAbs(1.33584297314295, DELTA_PHASE));
            CHECK_THAT(std::arg(values(9, 7)), WithinAbs(2.90989473171143, DELTA_PHASE));
            CHECK_THAT(std::arg(values(10, 0)), WithinAbs(-2.65587357462535, DELTA_PHASE));
            CHECK_THAT(std::arg(values(10, 1)), WithinAbs(-1.30302497013105, DELTA_PHASE));
            CHECK_THAT(std::arg(values(10, 2)), WithinAbs(-2.56513148566834, DELTA_PHASE));
            CHECK_THAT(std::arg(values(10, 3)), WithinAbs(1.50288452264914, DELTA_PHASE));
            CHECK_THAT(std::arg(values(10, 4)), WithinAbs(1.50288452264914, DELTA_PHASE));
            CHECK_THAT(std::arg(values(10, 5)), WithinAbs(-2.56513148566834, DELTA_PHASE));
            CHECK_THAT(std::arg(values(10, 6)), WithinAbs(-1.30302497013065, DELTA_PHASE));
            CHECK_THAT(std::arg(values(10, 7)), WithinAbs(-2.65587357462535, DELTA_PHASE));
            CHECK_THAT(std::arg(values(11, 0)), WithinAbs(-1.73470220406938, DELTA_PHASE));
            CHECK_THAT(std::arg(values(11, 1)), WithinAbs(-0.875285092401346, DELTA_PHASE));
            CHECK_THAT(std::arg(values(11, 2)), WithinAbs(0.639784789812871, DELTA_PHASE));
            CHECK_THAT(std::arg(values(11, 3)), WithinAbs(-0.565601279784796, DELTA_PHASE));
            CHECK_THAT(std::arg(values(11, 4)), WithinAbs(-0.565601279784796, DELTA_PHASE));
            CHECK_THAT(std::arg(values(11, 5)), WithinAbs(0.639784789812871, DELTA_PHASE));
            CHECK_THAT(std::arg(values(11, 6)), WithinAbs(-0.875285092401345, DELTA_PHASE));
            CHECK_THAT(std::arg(values(11, 7)), WithinAbs(-1.73470220406938, DELTA_PHASE));
            CHECK_THAT(std::arg(values(12, 0)), WithinAbs(2.39951755384229, DELTA_PHASE));
            CHECK_THAT(std::arg(values(12, 1)), WithinAbs(0.570607536877226, DELTA_PHASE));
            CHECK_THAT(std::arg(values(12, 2)), WithinAbs(-2.95705504518585, DELTA_PHASE));
            CHECK_THAT(std::arg(values(12, 3)), WithinAbs(2.34375554995456, DELTA_PHASE));
            CHECK_THAT(std::arg(values(12, 4)), WithinAbs(2.34375554995456, DELTA_PHASE));
            CHECK_THAT(std::arg(values(12, 5)), WithinAbs(-2.95705504518585, DELTA_PHASE));
            CHECK_THAT(std::arg(values(12, 6)), WithinAbs(0.570607536876314, DELTA_PHASE));
            CHECK_THAT(std::arg(values(12, 7)), WithinAbs(2.39951755384229, DELTA_PHASE));
            CHECK_THAT(std::arg(values(13, 0)), WithinAbs(-1.53877106842562, DELTA_PHASE));
            CHECK_THAT(std::arg(values(13, 1)), WithinAbs(1.66489040853494, DELTA_PHASE));
            CHECK_THAT(std::arg(values(13, 2)), WithinAbs(-1.2999218759808, DELTA_PHASE));
            CHECK_THAT(std::arg(values(13, 3)), WithinAbs(-2.37743568424626, DELTA_PHASE));
            CHECK_THAT(std::arg(values(13, 4)), WithinAbs(-2.37743568424626, DELTA_PHASE));
            CHECK_THAT(std::arg(values(13, 5)), WithinAbs(-1.2999218759808, DELTA_PHASE));
            CHECK_THAT(std::arg(values(13, 6)), WithinAbs(1.66489040853494, DELTA_PHASE));
            CHECK_THAT(std::arg(values(13, 7)), WithinAbs(-1.53877106842561, DELTA_PHASE));
            CHECK_THAT(std::arg(values(14, 0)), WithinAbs(0.898828212347599, DELTA_PHASE));
            CHECK_THAT(std::arg(values(14, 1)), WithinAbs(0.57912013854105, DELTA_PHASE));
            CHECK_THAT(std::arg(values(14, 2)), WithinAbs(-1.82938225706847, DELTA_PHASE));
            CHECK_THAT(std::arg(values(14, 3)), WithinAbs(-2.9733018895961, DELTA_PHASE));
            CHECK_THAT(std::arg(values(14, 4)), WithinAbs(-2.9733018895961, DELTA_PHASE));
            CHECK_THAT(std::arg(values(14, 5)), WithinAbs(-1.82938225706848, DELTA_PHASE));
            CHECK_THAT(std::arg(values(14, 6)), WithinAbs(0.579120138543665, DELTA_PHASE));
            CHECK_THAT(std::arg(values(14, 7)), WithinAbs(0.8988282123476, DELTA_PHASE));
            CHECK_THAT(std::arg(values(15, 0)), WithinAbs(-1.59046134379987, DELTA_PHASE));
            CHECK_THAT(std::arg(values(15, 1)), WithinAbs(-2.18508268543537, DELTA_PHASE));
            CHECK_THAT(std::arg(values(15, 2)), WithinAbs(-0.724963766234791, DELTA_PHASE));
            CHECK_THAT(std::arg(values(15, 3)), WithinAbs(-1.68114156649414, DELTA_PHASE));
            CHECK_THAT(std::arg(values(15, 4)), WithinAbs(-1.68114156649414, DELTA_PHASE));
            CHECK_THAT(std::arg(values(15, 5)), WithinAbs(-0.724963766234791, DELTA_PHASE));
            CHECK_THAT(std::arg(values(15, 6)), WithinAbs(-2.18508268543733, DELTA_PHASE));
            CHECK_THAT(std::arg(values(15, 7)), WithinAbs(-1.59046134379987, DELTA_PHASE));
        }

        SECTION("Wavelength sweep evaluation (eval_geometry_sweep)")
        {
            auto [positions, data] = voltage_field.eval_geometry_sweep(rect, sweep, n_dim1, n_dim2);

            // Verify array shapes: surfaces generate correct shape
            REQUIRE(positions.shape().rows == n_dim1);
            REQUIRE(positions.shape().cols == n_dim2);
            REQUIRE(data.size() == sweep.size());
            for (std::size_t page = 0; page < sweep.size(); ++page)
            {
                REQUIRE(data[page].shape().rows == n_dim1);
                REQUIRE(data[page].shape().cols == n_dim2);
            }

            // Verify grid corner positions
            CHECK_THAT(positions(0, 0).x, WithinRel(100, EPSILON_MAG));
            CHECK_THAT(positions(0, 0).y, WithinRel(100, EPSILON_MAG));
            CHECK_THAT(positions(0, 0).z, WithinRel(-50, EPSILON_MAG));
            CHECK_THAT(positions(positions.shape().rows - 1, 0).x, WithinRel(-100, EPSILON_MAG));
            CHECK_THAT(positions(positions.shape().rows - 1, 0).y, WithinRel(100, EPSILON_MAG));
            CHECK_THAT(positions(positions.shape().rows - 1, 0).z, WithinRel(-50, EPSILON_MAG));
            CHECK_THAT(positions(0, positions.shape().cols - 1).x, WithinRel(100, EPSILON_MAG));
            CHECK_THAT(positions(0, positions.shape().cols - 1).y, WithinRel(100, EPSILON_MAG));
            CHECK_THAT(positions(0, positions.shape().cols - 1).z, WithinRel(50, EPSILON_MAG));
            CHECK_THAT(positions(positions.shape().rows - 1, positions.shape().cols - 1).x, WithinRel(-100, EPSILON_MAG));
            CHECK_THAT(positions(positions.shape().rows - 1, positions.shape().cols - 1).y, WithinRel(100, EPSILON_MAG));
            CHECK_THAT(positions(positions.shape().rows - 1, positions.shape().cols - 1).z, WithinRel(50, EPSILON_MAG));

            // Verify page 0 (note: this page should be equal to the eval_geometry test)
            {
                auto& values = data[0];

                // Next, find the maximum of the physical values and normalize for better comparison
                double const abs_max = normalize(values);
                CHECK_THAT(abs_max, WithinRel(0.00170301857371513, EPSILON_MAG));

                // Verify physical values along the surface
                CHECK_THAT(std::abs(values(0, 0)), WithinRel(0.0811805333041281, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 1)), WithinRel(0.0209935151691007, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 2)), WithinRel(0.181776966584661, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 3)), WithinRel(0.959227993440513, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 4)), WithinRel(0.959227993440513, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 5)), WithinRel(0.181776966584661, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 6)), WithinRel(0.0209935151689359, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 7)), WithinRel(0.0811805333041281, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 0)), WithinRel(0.0455986942249276, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 1)), WithinRel(0.0411457551690019, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 2)), WithinRel(0.236208994298118, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 3)), WithinRel(0.980316457457755, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 4)), WithinRel(0.980316457457755, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 5)), WithinRel(0.236208994298118, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 6)), WithinRel(0.0411457551690372, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 7)), WithinRel(0.0455986942249277, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 0)), WithinRel(0.00257584589170965, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 1)), WithinRel(0.0941592075283062, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 2)), WithinRel(0.275478907113585, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 3)), WithinRel(0.99429542171149, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 4)), WithinRel(0.99429542171149, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 5)), WithinRel(0.275478907113585, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 6)), WithinRel(0.0941592075283062, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 7)), WithinRel(0.00257584589170968, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 0)), WithinRel(0.0357596041928141, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 1)), WithinRel(0.128220787975769, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 2)), WithinRel(0.294897071700718, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 3)), WithinRel(1, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 4)), WithinRel(1, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 5)), WithinRel(0.294897071700718, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 6)), WithinRel(0.128220787975793, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 7)), WithinRel(0.0357596041928141, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 0)), WithinRel(0.0589060465262149, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 1)), WithinRel(0.139456471473219, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 2)), WithinRel(0.294128532445929, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 3)), WithinRel(0.997606657015968, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 4)), WithinRel(0.997606657015968, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 5)), WithinRel(0.294128532445929, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 6)), WithinRel(0.139456471473219, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 7)), WithinRel(0.0589060465262149, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 0)), WithinRel(0.0665616877563288, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 1)), WithinRel(0.132742825255253, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 2)), WithinRel(0.27873146020983, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 3)), WithinRel(0.989316385385742, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 4)), WithinRel(0.989316385385742, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 5)), WithinRel(0.27873146020983, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 6)), WithinRel(0.13274282525528, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 7)), WithinRel(0.0665616877563289, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 0)), WithinRel(0.0650061963206579, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 1)), WithinRel(0.118932954431791, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 2)), WithinRel(0.258936075347669, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 3)), WithinRel(0.979344504415583, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 4)), WithinRel(0.979344504415583, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 5)), WithinRel(0.258936075347669, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 6)), WithinRel(0.118932954431791, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 7)), WithinRel(0.0650061963206579, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 0)), WithinRel(0.0619228784388384, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 1)), WithinRel(0.108899738482795, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 2)), WithinRel(0.245612300311409, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 3)), WithinRel(0.972648506328136, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 4)), WithinRel(0.972648506328136, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 5)), WithinRel(0.245612300311409, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 6)), WithinRel(0.108899738482732, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 7)), WithinRel(0.0619228784388384, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 0)), WithinRel(0.0619228784388384, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 1)), WithinRel(0.108899738482794, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 2)), WithinRel(0.245612300311409, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 3)), WithinRel(0.972648506328136, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 4)), WithinRel(0.972648506328136, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 5)), WithinRel(0.245612300311409, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 6)), WithinRel(0.108899738482732, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 7)), WithinRel(0.0619228784388384, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 0)), WithinRel(0.0650061963206579, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 1)), WithinRel(0.118932954431791, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 2)), WithinRel(0.258936075347669, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 3)), WithinRel(0.979344504415583, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 4)), WithinRel(0.979344504415583, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 5)), WithinRel(0.258936075347669, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 6)), WithinRel(0.118932954431791, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 7)), WithinRel(0.0650061963206579, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 0)), WithinRel(0.0665616877563289, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 1)), WithinRel(0.132742825255253, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 2)), WithinRel(0.27873146020983, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 3)), WithinRel(0.989316385385742, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 4)), WithinRel(0.989316385385742, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 5)), WithinRel(0.27873146020983, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 6)), WithinRel(0.13274282525528, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 7)), WithinRel(0.0665616877563289, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 0)), WithinRel(0.0589060465262149, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 1)), WithinRel(0.139456471473219, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 2)), WithinRel(0.294128532445929, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 3)), WithinRel(0.997606657015968, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 4)), WithinRel(0.997606657015968, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 5)), WithinRel(0.294128532445929, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 6)), WithinRel(0.139456471473219, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 7)), WithinRel(0.058906046526215, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 0)), WithinRel(0.0357596041928872, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 1)), WithinRel(0.128220787975745, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 2)), WithinRel(0.294897071701048, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 3)), WithinRel(0.999999999999537, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 4)), WithinRel(0.999999999999537, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 5)), WithinRel(0.294897071701048, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 6)), WithinRel(0.12822078797574, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 7)), WithinRel(0.0357596041928873, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 0)), WithinRel(0.00257584589170966, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 1)), WithinRel(0.0941592075283062, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 2)), WithinRel(0.275478907113585, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 3)), WithinRel(0.99429542171149, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 4)), WithinRel(0.99429542171149, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 5)), WithinRel(0.275478907113585, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 6)), WithinRel(0.0941592075283062, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 7)), WithinRel(0.00257584589170969, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 0)), WithinRel(0.0455986942249276, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 1)), WithinRel(0.0411457551690019, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 2)), WithinRel(0.236208994298118, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 3)), WithinRel(0.980316457457755, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 4)), WithinRel(0.980316457457755, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 5)), WithinRel(0.236208994298118, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 6)), WithinRel(0.0411457551690372, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 7)), WithinRel(0.0455986942249276, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 0)), WithinRel(0.0811805333041281, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 1)), WithinRel(0.0209935151691007, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 2)), WithinRel(0.181776966584661, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 3)), WithinRel(0.959227993440513, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 4)), WithinRel(0.959227993440513, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 5)), WithinRel(0.181776966584661, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 6)), WithinRel(0.0209935151689359, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 7)), WithinRel(0.0811805333041281, EPSILON_MAG));
                CHECK_THAT(std::arg(values(0, 0)), WithinAbs(-1.59046134379987, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 1)), WithinAbs(-2.18508268543536, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 2)), WithinAbs(-0.724963766234791, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 3)), WithinAbs(-1.68114156649414, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 4)), WithinAbs(-1.68114156649414, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 5)), WithinAbs(-0.724963766234791, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 6)), WithinAbs(-2.18508268543734, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 7)), WithinAbs(-1.59046134379987, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 0)), WithinAbs(0.8988282123476, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 1)), WithinAbs(0.57912013854105, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 2)), WithinAbs(-1.82938225706847, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 3)), WithinAbs(-2.9733018895961, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 4)), WithinAbs(-2.9733018895961, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 5)), WithinAbs(-1.82938225706847, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 6)), WithinAbs(0.579120138543666, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 7)), WithinAbs(0.898828212347601, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 0)), WithinAbs(-1.53877106842563, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 1)), WithinAbs(1.66489040853495, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 2)), WithinAbs(-1.2999218759808, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 3)), WithinAbs(-2.37743568424626, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 4)), WithinAbs(-2.37743568424626, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 5)), WithinAbs(-1.2999218759808, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 6)), WithinAbs(1.66489040853494, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 7)), WithinAbs(-1.53877106842561, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 0)), WithinAbs(2.39951755383822, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 1)), WithinAbs(0.570607536876258, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 2)), WithinAbs(-2.9570550451856, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 3)), WithinAbs(2.34375554995525, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 4)), WithinAbs(2.34375554995525, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 5)), WithinAbs(-2.9570550451856, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 6)), WithinAbs(0.570607536876678, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 7)), WithinAbs(2.39951755383822, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 0)), WithinAbs(-1.73470220406938, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 1)), WithinAbs(-0.875285092401346, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 2)), WithinAbs(0.639784789812871, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 3)), WithinAbs(-0.565601279784796, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 4)), WithinAbs(-0.565601279784796, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 5)), WithinAbs(0.639784789812871, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 6)), WithinAbs(-0.875285092401345, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 7)), WithinAbs(-1.73470220406938, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 0)), WithinAbs(-2.65587357462535, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 1)), WithinAbs(-1.30302497013105, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 2)), WithinAbs(-2.56513148566834, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 3)), WithinAbs(1.50288452264914, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 4)), WithinAbs(1.50288452264914, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 5)), WithinAbs(-2.56513148566834, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 6)), WithinAbs(-1.30302497013065, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 7)), WithinAbs(-2.65587357462535, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 0)), WithinAbs(2.90989473171143, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 1)), WithinAbs(1.33584297314295, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 2)), WithinAbs(1.07597558528228, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 3)), WithinAbs(2.80787643375169, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 4)), WithinAbs(2.80787643375169, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 5)), WithinAbs(1.07597558528228, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 6)), WithinAbs(1.33584297314295, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 7)), WithinAbs(2.90989473171143, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 0)), WithinAbs(1.41636191652562, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 1)), WithinAbs(-1.29995402322705, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 2)), WithinAbs(2.34715161578265, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 3)), WithinAbs(-0.0792612673224425, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 4)), WithinAbs(-0.0792612673224425, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 5)), WithinAbs(2.34715161578265, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 6)), WithinAbs(-1.29995402322705, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 7)), WithinAbs(1.41636191652563, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 0)), WithinAbs(1.41636191652562, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 1)), WithinAbs(-1.29995402322705, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 2)), WithinAbs(2.34715161578265, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 3)), WithinAbs(-0.0792612673224425, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 4)), WithinAbs(-0.0792612673224425, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 5)), WithinAbs(2.34715161578265, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 6)), WithinAbs(-1.29995402322705, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 7)), WithinAbs(1.41636191652563, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 0)), WithinAbs(2.90989473171143, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 1)), WithinAbs(1.33584297314295, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 2)), WithinAbs(1.07597558528228, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 3)), WithinAbs(2.80787643375169, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 4)), WithinAbs(2.80787643375169, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 5)), WithinAbs(1.07597558528228, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 6)), WithinAbs(1.33584297314295, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 7)), WithinAbs(2.90989473171143, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 0)), WithinAbs(-2.65587357462535, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 1)), WithinAbs(-1.30302497013105, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 2)), WithinAbs(-2.56513148566834, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 3)), WithinAbs(1.50288452264914, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 4)), WithinAbs(1.50288452264914, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 5)), WithinAbs(-2.56513148566834, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 6)), WithinAbs(-1.30302497013065, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 7)), WithinAbs(-2.65587357462535, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 0)), WithinAbs(-1.73470220406938, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 1)), WithinAbs(-0.875285092401346, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 2)), WithinAbs(0.639784789812871, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 3)), WithinAbs(-0.565601279784796, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 4)), WithinAbs(-0.565601279784796, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 5)), WithinAbs(0.639784789812871, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 6)), WithinAbs(-0.875285092401345, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 7)), WithinAbs(-1.73470220406938, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 0)), WithinAbs(2.39951755384229, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 1)), WithinAbs(0.570607536877226, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 2)), WithinAbs(-2.95705504518585, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 3)), WithinAbs(2.34375554995456, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 4)), WithinAbs(2.34375554995456, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 5)), WithinAbs(-2.95705504518585, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 6)), WithinAbs(0.570607536876314, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 7)), WithinAbs(2.39951755384229, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 0)), WithinAbs(-1.53877106842562, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 1)), WithinAbs(1.66489040853494, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 2)), WithinAbs(-1.2999218759808, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 3)), WithinAbs(-2.37743568424626, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 4)), WithinAbs(-2.37743568424626, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 5)), WithinAbs(-1.2999218759808, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 6)), WithinAbs(1.66489040853494, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 7)), WithinAbs(-1.53877106842561, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 0)), WithinAbs(0.898828212347599, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 1)), WithinAbs(0.57912013854105, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 2)), WithinAbs(-1.82938225706847, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 3)), WithinAbs(-2.9733018895961, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 4)), WithinAbs(-2.9733018895961, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 5)), WithinAbs(-1.82938225706848, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 6)), WithinAbs(0.579120138543665, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 7)), WithinAbs(0.8988282123476, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 0)), WithinAbs(-1.59046134379987, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 1)), WithinAbs(-2.18508268543537, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 2)), WithinAbs(-0.724963766234791, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 3)), WithinAbs(-1.68114156649414, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 4)), WithinAbs(-1.68114156649414, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 5)), WithinAbs(-0.724963766234791, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 6)), WithinAbs(-2.18508268543733, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 7)), WithinAbs(-1.59046134379987, DELTA_PHASE));
            }

            // Verify page 1
            {
                auto& values = data[1];

                // Next, find the maximum of the physical values and normalize for better comparison
                double const abs_max = normalize(values);
                CHECK_THAT(abs_max, WithinRel(0.00262415150477076, EPSILON_MAG));

                // Verify physical values along the surface
                CHECK_THAT(std::abs(values(0, 0)), WithinRel(0.0813920796290424, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 1)), WithinRel(0.159917982283952, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 2)), WithinRel(0.205458647782092, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 3)), WithinRel(0.8090469509971, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 4)), WithinRel(0.8090469509971, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 5)), WithinRel(0.205458647782091, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 6)), WithinRel(0.15991798228388, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 7)), WithinRel(0.0813920796290423, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 0)), WithinRel(0.043570731989055, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 1)), WithinRel(0.180815780118988, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 2)), WithinRel(0.15471939047631, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 3)), WithinRel(0.848680945743085, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 4)), WithinRel(0.848680945743085, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 5)), WithinRel(0.15471939047631, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 6)), WithinRel(0.180815780118965, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 7)), WithinRel(0.0435707319890549, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 0)), WithinRel(0.00253488461355195, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 1)), WithinRel(0.188632002748327, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 2)), WithinRel(0.0980501762348033, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 3)), WithinRel(0.886905104645068, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 4)), WithinRel(0.886905104645068, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 5)), WithinRel(0.0980501762348032, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 6)), WithinRel(0.188632002748327, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 7)), WithinRel(0.00253488461355193, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 0)), WithinRel(0.0365040952134442, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 1)), WithinRel(0.182481715515508, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 2)), WithinRel(0.0392879112594042, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 3)), WithinRel(0.922020581974186, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 4)), WithinRel(0.922020581974186, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 5)), WithinRel(0.0392879112594042, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 6)), WithinRel(0.182481715515508, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 7)), WithinRel(0.0365040952134442, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 0)), WithinRel(0.0666287244737948, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 1)), WithinRel(0.16489049508561, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 2)), WithinRel(0.019185876248624, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 3)), WithinRel(0.952208610160764, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 4)), WithinRel(0.952208610160764, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 5)), WithinRel(0.019185876248624, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 6)), WithinRel(0.16489049508561, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 7)), WithinRel(0.0666287244737948, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 0)), WithinRel(0.0858695121111939, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 1)), WithinRel(0.141758512009754, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 2)), WithinRel(0.0662395726249229, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 3)), WithinRel(0.975866754178091, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 4)), WithinRel(0.975866754178091, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 5)), WithinRel(0.0662395726249229, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 6)), WithinRel(0.141758512009737, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 7)), WithinRel(0.0858695121111939, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 0)), WithinRel(0.0955417665208314, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 1)), WithinRel(0.12065295827092, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 2)), WithinRel(0.100767426207421, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 3)), WithinRel(0.991938924464441, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 4)), WithinRel(0.991938924464441, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 5)), WithinRel(0.100767426207421, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 6)), WithinRel(0.12065295827092, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 7)), WithinRel(0.0955417665208314, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 0)), WithinRel(0.0990390295480543, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 1)), WithinRel(0.108229371169274, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 2)), WithinRel(0.118767855264159, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 3)), WithinRel(1, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 4)), WithinRel(1, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 5)), WithinRel(0.118767855264159, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 6)), WithinRel(0.108229371169274, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 7)), WithinRel(0.0990390295480543, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 0)), WithinRel(0.0990390295480544, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 1)), WithinRel(0.108229371169274, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 2)), WithinRel(0.118767855264159, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 3)), WithinRel(1, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 4)), WithinRel(1, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 5)), WithinRel(0.118767855264159, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 6)), WithinRel(0.108229371169274, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 7)), WithinRel(0.0990390295480543, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 0)), WithinRel(0.0955417665208314, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 1)), WithinRel(0.12065295827092, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 2)), WithinRel(0.100767426207421, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 3)), WithinRel(0.991938924464441, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 4)), WithinRel(0.991938924464441, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 5)), WithinRel(0.100767426207421, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 6)), WithinRel(0.12065295827092, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 7)), WithinRel(0.0955417665208314, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 0)), WithinRel(0.0858695121111939, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 1)), WithinRel(0.141758512009754, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 2)), WithinRel(0.0662395726249229, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 3)), WithinRel(0.975866754178091, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 4)), WithinRel(0.975866754178091, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 5)), WithinRel(0.0662395726249229, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 6)), WithinRel(0.141758512009737, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 7)), WithinRel(0.0858695121111939, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 0)), WithinRel(0.0666287244737948, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 1)), WithinRel(0.16489049508561, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 2)), WithinRel(0.019185876248624, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 3)), WithinRel(0.952208610160764, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 4)), WithinRel(0.952208610160764, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 5)), WithinRel(0.019185876248624, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 6)), WithinRel(0.16489049508561, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 7)), WithinRel(0.0666287244737948, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 0)), WithinRel(0.0365040952133544, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 1)), WithinRel(0.182481715515543, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 2)), WithinRel(0.0392879112593979, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 3)), WithinRel(0.922020581974036, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 4)), WithinRel(0.922020581974036, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 5)), WithinRel(0.0392879112593979, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 6)), WithinRel(0.1824817155155, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 7)), WithinRel(0.0365040952133545, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 0)), WithinRel(0.00253488461355196, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 1)), WithinRel(0.188632002748327, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 2)), WithinRel(0.0980501762348032, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 3)), WithinRel(0.886905104645069, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 4)), WithinRel(0.886905104645068, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 5)), WithinRel(0.0980501762348032, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 6)), WithinRel(0.188632002748327, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 7)), WithinRel(0.00253488461355191, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 0)), WithinRel(0.043570731989055, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 1)), WithinRel(0.180815780118988, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 2)), WithinRel(0.15471939047631, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 3)), WithinRel(0.848680945743085, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 4)), WithinRel(0.848680945743085, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 5)), WithinRel(0.15471939047631, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 6)), WithinRel(0.180815780118965, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 7)), WithinRel(0.0435707319890549, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 0)), WithinRel(0.0813920796290424, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 1)), WithinRel(0.159917982283952, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 2)), WithinRel(0.205458647782092, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 3)), WithinRel(0.8090469509971, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 4)), WithinRel(0.8090469509971, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 5)), WithinRel(0.205458647782091, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 6)), WithinRel(0.15991798228388, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 7)), WithinRel(0.0813920796290423, EPSILON_MAG));
                CHECK_THAT(std::arg(values(0, 0)), WithinAbs(1.5660297421716, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 1)), WithinAbs(-1.02342533187269, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 2)), WithinAbs(1.1393340071669, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 3)), WithinAbs(-1.64573752094675, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 4)), WithinAbs(-1.64573752094675, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 5)), WithinAbs(1.1393340071669, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 6)), WithinAbs(-1.02342533187256, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 7)), WithinAbs(1.5660297421716, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 0)), WithinAbs(1.13662676122023, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 1)), WithinAbs(3.04174809369909, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 2)), WithinAbs(0.408142059494028, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 3)), WithinAbs(1.68124791287845, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 4)), WithinAbs(1.68124791287845, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 5)), WithinAbs(0.408142059494028, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 6)), WithinAbs(3.04174809369858, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 7)), WithinAbs(1.13662676122023, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 0)), WithinAbs(-0.281782173001943, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 1)), WithinAbs(-0.44585260614164, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 2)), WithinAbs(-1.31038369616287, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 3)), WithinAbs(-2.11076064133287, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 4)), WithinAbs(-2.11076064133287, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 5)), WithinAbs(-1.31038369616287, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 6)), WithinAbs(-0.44585260614164, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 7)), WithinAbs(-0.28178217300192, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 0)), WithinAbs(-1.03214774294244, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 1)), WithinAbs(0.914750636729227, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 2)), WithinAbs(1.88056378952507, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 3)), WithinAbs(-1.05826267843879, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 4)), WithinAbs(-1.05826267843879, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 5)), WithinAbs(1.88056378952508, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 6)), WithinAbs(0.914750636729228, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 7)), WithinAbs(-1.03214774294244, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 0)), WithinAbs(0.406633098776391, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 1)), WithinAbs(-0.0500930861027495, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 2)), WithinAbs(-1.52278451940661, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 3)), WithinAbs(-2.99849032627672, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 4)), WithinAbs(-2.99849032627672, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 5)), WithinAbs(-1.52278451940661, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 6)), WithinAbs(-0.0500930861027497, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 7)), WithinAbs(0.406633098776391, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 0)), WithinAbs(-0.206461548927317, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 1)), WithinAbs(1.75954197966185, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 2)), WithinAbs(0.799886570466575, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 3)), WithinAbs(2.56860641118644, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 4)), WithinAbs(2.56860641118644, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 5)), WithinAbs(0.799886570466576, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 6)), WithinAbs(1.75954197966149, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 7)), WithinAbs(-0.206461548927317, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 0)), WithinAbs(-2.77947442039561, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 1)), WithinAbs(-0.668997750346929, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 2)), WithinAbs(-3.02092983736254, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 3)), WithinAbs(1.34361210956709, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 4)), WithinAbs(1.34361210956709, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 5)), WithinAbs(-3.02092983736254, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 6)), WithinAbs(-0.668997750346928, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 7)), WithinAbs(-2.77947442039561, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 0)), WithinAbs(0.413072121693463, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 1)), WithinAbs(-0.330919849876266, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 2)), WithinAbs(-0.0696652751859242, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 3)), WithinAbs(-0.581499217679268, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 4)), WithinAbs(-0.581499217679268, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 5)), WithinAbs(-0.0696652751859251, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 6)), WithinAbs(-0.330919849876267, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 7)), WithinAbs(0.413072121693464, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 0)), WithinAbs(0.413072121693463, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 1)), WithinAbs(-0.330919849876267, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 2)), WithinAbs(-0.0696652751859242, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 3)), WithinAbs(-0.581499217679268, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 4)), WithinAbs(-0.581499217679268, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 5)), WithinAbs(-0.0696652751859251, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 6)), WithinAbs(-0.330919849876267, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 7)), WithinAbs(0.413072121693464, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 0)), WithinAbs(-2.77947442039561, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 1)), WithinAbs(-0.668997750346928, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 2)), WithinAbs(-3.02092983736254, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 3)), WithinAbs(1.34361210956709, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 4)), WithinAbs(1.34361210956709, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 5)), WithinAbs(-3.02092983736254, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 6)), WithinAbs(-0.668997750346928, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 7)), WithinAbs(-2.77947442039561, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 0)), WithinAbs(-0.206461548927317, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 1)), WithinAbs(1.75954197966185, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 2)), WithinAbs(0.799886570466575, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 3)), WithinAbs(2.56860641118644, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 4)), WithinAbs(2.56860641118644, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 5)), WithinAbs(0.799886570466576, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 6)), WithinAbs(1.75954197966149, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 7)), WithinAbs(-0.206461548927317, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 0)), WithinAbs(0.40663309877639, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 1)), WithinAbs(-0.0500930861027495, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 2)), WithinAbs(-1.52278451940661, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 3)), WithinAbs(-2.99849032627672, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 4)), WithinAbs(-2.99849032627672, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 5)), WithinAbs(-1.52278451940661, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 6)), WithinAbs(-0.0500930861027495, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 7)), WithinAbs(0.40663309877639, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 0)), WithinAbs(-1.03214774294059, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 1)), WithinAbs(0.91475063672914, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 2)), WithinAbs(1.88056378952585, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 3)), WithinAbs(-1.05826267843925, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 4)), WithinAbs(-1.05826267843925, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 5)), WithinAbs(1.88056378952585, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 6)), WithinAbs(0.914750636729289, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 7)), WithinAbs(-1.03214774294059, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 0)), WithinAbs(-0.281782173001936, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 1)), WithinAbs(-0.44585260614164, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 2)), WithinAbs(-1.31038369616287, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 3)), WithinAbs(-2.11076064133287, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 4)), WithinAbs(-2.11076064133287, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 5)), WithinAbs(-1.31038369616287, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 6)), WithinAbs(-0.44585260614164, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 7)), WithinAbs(-0.281782173001923, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 0)), WithinAbs(1.13662676122023, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 1)), WithinAbs(3.04174809369909, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 2)), WithinAbs(0.408142059494028, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 3)), WithinAbs(1.68124791287845, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 4)), WithinAbs(1.68124791287845, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 5)), WithinAbs(0.408142059494027, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 6)), WithinAbs(3.04174809369858, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 7)), WithinAbs(1.13662676122023, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 0)), WithinAbs(1.5660297421716, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 1)), WithinAbs(-1.02342533187269, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 2)), WithinAbs(1.13933400716691, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 3)), WithinAbs(-1.64573752094675, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 4)), WithinAbs(-1.64573752094675, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 5)), WithinAbs(1.1393340071669, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 6)), WithinAbs(-1.02342533187256, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 7)), WithinAbs(1.5660297421716, DELTA_PHASE));
            }

            // Verify page 2
            {
                auto& values = data[2];

                // Next, find the maximum of the physical values and normalize for better comparison
                double const abs_max = normalize(values);
                CHECK_THAT(abs_max, WithinRel(0.00360682665176666, EPSILON_MAG));

                // Verify physical values along the surface
                CHECK_THAT(std::abs(values(0, 0)), WithinRel(0.136351738833879, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 1)), WithinRel(0.0156554679377567, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 2)), WithinRel(0.397490031436339, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 3)), WithinRel(0.762783961753825, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 4)), WithinRel(0.762783961753825, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 5)), WithinRel(0.397490031436339, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 6)), WithinRel(0.0156554679377761, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 7)), WithinRel(0.13635173883388, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 0)), WithinRel(0.14857503965707, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 1)), WithinRel(0.0312673485306491, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 2)), WithinRel(0.376578411378417, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 3)), WithinRel(0.806714274219831, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 4)), WithinRel(0.806714274219831, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 5)), WithinRel(0.376578411378417, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 6)), WithinRel(0.0312673485306451, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 7)), WithinRel(0.14857503965707, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 0)), WithinRel(0.15070288882525, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 1)), WithinRel(0.0765947669904878, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 2)), WithinRel(0.348634440864513, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 3)), WithinRel(0.850848809701595, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 4)), WithinRel(0.850848809701595, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 5)), WithinRel(0.348634440864513, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 6)), WithinRel(0.0765947669904879, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 7)), WithinRel(0.15070288882525, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 0)), WithinRel(0.14295381470043, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 1)), WithinRel(0.117354855779035, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 2)), WithinRel(0.314840404215442, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 3)), WithinRel(0.893433711187075, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 4)), WithinRel(0.893433711187075, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 5)), WithinRel(0.314840404215442, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 6)), WithinRel(0.117354855779031, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 7)), WithinRel(0.14295381470043, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 0)), WithinRel(0.127883027223684, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 1)), WithinRel(0.150260701398678, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 2)), WithinRel(0.278065746582953, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 3)), WithinRel(0.932188646845362, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 4)), WithinRel(0.932188646845362, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 5)), WithinRel(0.278065746582953, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 6)), WithinRel(0.150260701398678, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 7)), WithinRel(0.127883027223684, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 0)), WithinRel(0.110001257985308, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 1)), WithinRel(0.173533460612235, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 2)), WithinRel(0.242902564630109, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 3)), WithinRel(0.964497645940459, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 4)), WithinRel(0.964497645940459, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 5)), WithinRel(0.242902564630109, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 6)), WithinRel(0.173533460612229, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 7)), WithinRel(0.110001257985308, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 0)), WithinRel(0.094473506141054, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 1)), WithinRel(0.187426546889668, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 2)), WithinRel(0.214959331864622, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 3)), WithinRel(0.987787141496915, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 4)), WithinRel(0.987787141496915, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 5)), WithinRel(0.214959331864622, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 6)), WithinRel(0.187426546889668, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 7)), WithinRel(0.094473506141054, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 0)), WithinRel(0.0855555739512042, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 1)), WithinRel(0.193613808063871, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 2)), WithinRel(0.199423301961654, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 3)), WithinRel(1, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 4)), WithinRel(1, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 5)), WithinRel(0.199423301961654, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 6)), WithinRel(0.193613808063838, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 7)), WithinRel(0.0855555739512041, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 0)), WithinRel(0.0855555739512042, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 1)), WithinRel(0.193613808063871, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 2)), WithinRel(0.199423301961654, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 3)), WithinRel(1, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 4)), WithinRel(1, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 5)), WithinRel(0.199423301961654, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 6)), WithinRel(0.193613808063838, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 7)), WithinRel(0.0855555739512041, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 0)), WithinRel(0.094473506141054, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 1)), WithinRel(0.187426546889668, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 2)), WithinRel(0.214959331864622, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 3)), WithinRel(0.987787141496915, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 4)), WithinRel(0.987787141496915, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 5)), WithinRel(0.214959331864622, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 6)), WithinRel(0.187426546889668, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 7)), WithinRel(0.094473506141054, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 0)), WithinRel(0.110001257985308, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 1)), WithinRel(0.173533460612235, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 2)), WithinRel(0.242902564630109, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 3)), WithinRel(0.964497645940459, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 4)), WithinRel(0.964497645940459, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 5)), WithinRel(0.242902564630109, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 6)), WithinRel(0.173533460612229, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 7)), WithinRel(0.110001257985308, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 0)), WithinRel(0.127883027223684, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 1)), WithinRel(0.150260701398678, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 2)), WithinRel(0.278065746582953, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 3)), WithinRel(0.932188646845362, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 4)), WithinRel(0.932188646845362, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 5)), WithinRel(0.278065746582953, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 6)), WithinRel(0.150260701398678, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 7)), WithinRel(0.127883027223684, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 0)), WithinRel(0.142953814700409, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 1)), WithinRel(0.117354855779051, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 2)), WithinRel(0.314840404215458, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 3)), WithinRel(0.893433711186985, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 4)), WithinRel(0.893433711186985, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 5)), WithinRel(0.314840404215458, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 6)), WithinRel(0.117354855779004, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 7)), WithinRel(0.142953814700408, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 0)), WithinRel(0.15070288882525, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 1)), WithinRel(0.0765947669904878, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 2)), WithinRel(0.348634440864513, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 3)), WithinRel(0.850848809701595, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 4)), WithinRel(0.850848809701595, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 5)), WithinRel(0.348634440864513, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 6)), WithinRel(0.0765947669904879, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 7)), WithinRel(0.15070288882525, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 0)), WithinRel(0.14857503965707, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 1)), WithinRel(0.0312673485306491, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 2)), WithinRel(0.376578411378417, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 3)), WithinRel(0.806714274219831, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 4)), WithinRel(0.806714274219831, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 5)), WithinRel(0.376578411378417, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 6)), WithinRel(0.0312673485306451, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 7)), WithinRel(0.14857503965707, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 0)), WithinRel(0.136351738833879, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 1)), WithinRel(0.0156554679377567, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 2)), WithinRel(0.397490031436339, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 3)), WithinRel(0.762783961753825, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 4)), WithinRel(0.762783961753825, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 5)), WithinRel(0.397490031436339, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 6)), WithinRel(0.0156554679377761, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 7)), WithinRel(0.13635173883388, EPSILON_MAG));
                CHECK_THAT(std::arg(values(0, 0)), WithinAbs(1.55239300789175, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 1)), WithinAbs(2.97398471370171, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 2)), WithinAbs(-2.68954196355744, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 3)), WithinAbs(-1.62730398817709, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 4)), WithinAbs(-1.62730398817709, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 5)), WithinAbs(-2.68954196355744, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 6)), WithinAbs(2.97398471370042, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 7)), WithinAbs(1.55239300789175, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 0)), WithinAbs(-0.350198689109576, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 1)), WithinAbs(-0.561001726224911, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 2)), WithinAbs(3.03807656147935, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 3)), WithinAbs(0.867862641187321, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 4)), WithinAbs(0.867862641187321, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 5)), WithinAbs(3.03807656147935, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 6)), WithinAbs(-0.561001726226291, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 7)), WithinAbs(-0.350198689109575, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 0)), WithinAbs(-1.83005923463768, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 1)), WithinAbs(0.0220812220548482, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 2)), WithinAbs(-2.98127085760698, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 3)), WithinAbs(-1.97623246327732, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 4)), WithinAbs(-1.97623246327732, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 5)), WithinAbs(-2.98127085760698, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 6)), WithinAbs(0.0220812220548478, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 7)), WithinAbs(-1.83005923463768, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 0)), WithinAbs(-1.13562679905633, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 1)), WithinAbs(2.62588304315354, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 2)), WithinAbs(-0.667604247344914, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 3)), WithinAbs(-2.75775985524043, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 4)), WithinAbs(-2.75775985524043, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 5)), WithinAbs(-0.667604247344914, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 6)), WithinAbs(2.62588304315334, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 7)), WithinAbs(-1.13562679905633, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 0)), WithinAbs(3.07351007973182, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 1)), WithinAbs(1.90631943349166, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 2)), WithinAbs(-2.0089493832073, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 3)), WithinAbs(-1.07145356107594, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 4)), WithinAbs(-1.07145356107594, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 5)), WithinAbs(-2.0089493832073, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 6)), WithinAbs(1.90631943349166, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 7)), WithinAbs(3.07351007973182, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 0)), WithinAbs(-0.529553924296922, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 1)), WithinAbs(-1.44822308307812, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 2)), WithinAbs(-0.467093517661823, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 3)), WithinAbs(3.10375378688958, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 4)), WithinAbs(3.10375378688958, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 5)), WithinAbs(-0.467093517661823, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 6)), WithinAbs(-1.44822308307826, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 7)), WithinAbs(-0.529553924296922, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 0)), WithinAbs(2.25392287848855, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 1)), WithinAbs(-0.129160285081671, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 2)), WithinAbs(-1.78515010878263, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 3)), WithinAbs(-2.52747702795532, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 4)), WithinAbs(-2.52747702795532, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 5)), WithinAbs(-1.78515010878263, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 6)), WithinAbs(-0.129160285081671, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 7)), WithinAbs(2.25392287848855, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 0)), WithinAbs(1.50788027264301, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 1)), WithinAbs(1.69386996651941, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 2)), WithinAbs(1.99409812030865, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 3)), WithinAbs(2.3118192521272, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 4)), WithinAbs(2.3118192521272, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 5)), WithinAbs(1.99409812030865, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 6)), WithinAbs(1.69386996651957, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 7)), WithinAbs(1.50788027264301, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 0)), WithinAbs(1.50788027264301, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 1)), WithinAbs(1.69386996651941, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 2)), WithinAbs(1.99409812030865, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 3)), WithinAbs(2.3118192521272, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 4)), WithinAbs(2.3118192521272, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 5)), WithinAbs(1.99409812030865, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 6)), WithinAbs(1.69386996651957, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 7)), WithinAbs(1.50788027264301, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 0)), WithinAbs(2.25392287848855, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 1)), WithinAbs(-0.129160285081671, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 2)), WithinAbs(-1.78515010878263, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 3)), WithinAbs(-2.52747702795532, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 4)), WithinAbs(-2.52747702795532, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 5)), WithinAbs(-1.78515010878263, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 6)), WithinAbs(-0.129160285081671, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 7)), WithinAbs(2.25392287848855, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 0)), WithinAbs(-0.529553924296922, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 1)), WithinAbs(-1.44822308307812, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 2)), WithinAbs(-0.467093517661823, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 3)), WithinAbs(3.10375378688958, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 4)), WithinAbs(3.10375378688958, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 5)), WithinAbs(-0.467093517661823, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 6)), WithinAbs(-1.44822308307826, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 7)), WithinAbs(-0.529553924296922, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 0)), WithinAbs(3.07351007973182, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 1)), WithinAbs(1.90631943349166, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 2)), WithinAbs(-2.0089493832073, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 3)), WithinAbs(-1.07145356107594, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 4)), WithinAbs(-1.07145356107594, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 5)), WithinAbs(-2.0089493832073, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 6)), WithinAbs(1.90631943349166, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 7)), WithinAbs(3.07351007973182, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 0)), WithinAbs(-1.13562679905683, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 1)), WithinAbs(2.62588304315341, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 2)), WithinAbs(-0.667604247345041, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 3)), WithinAbs(-2.75775985524077, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 4)), WithinAbs(-2.75775985524077, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 5)), WithinAbs(-0.667604247345041, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 6)), WithinAbs(2.62588304315341, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 7)), WithinAbs(-1.13562679905683, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 0)), WithinAbs(-1.83005923463768, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 1)), WithinAbs(0.0220812220548485, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 2)), WithinAbs(-2.98127085760698, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 3)), WithinAbs(-1.97623246327732, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 4)), WithinAbs(-1.97623246327732, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 5)), WithinAbs(-2.98127085760698, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 6)), WithinAbs(0.0220812220548478, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 7)), WithinAbs(-1.83005923463768, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 0)), WithinAbs(-0.350198689109576, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 1)), WithinAbs(-0.56100172622491, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 2)), WithinAbs(3.03807656147935, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 3)), WithinAbs(0.867862641187321, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 4)), WithinAbs(0.867862641187321, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 5)), WithinAbs(3.03807656147935, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 6)), WithinAbs(-0.561001726226293, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 7)), WithinAbs(-0.350198689109575, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 0)), WithinAbs(1.55239300789175, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 1)), WithinAbs(2.97398471370171, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 2)), WithinAbs(-2.68954196355744, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 3)), WithinAbs(-1.62730398817709, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 4)), WithinAbs(-1.62730398817709, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 5)), WithinAbs(-2.68954196355744, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 6)), WithinAbs(2.97398471370042, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 7)), WithinAbs(1.55239300789175, DELTA_PHASE));
            }
        }
    }

    SECTION("Evaluation over SphericalRectangle geometry")
    {
        std::size_t const n_dim1 = 16;
        std::size_t const n_dim2 = 8;
        Geometry const rect = geometry::SphericalRectangle{"rect_01", {0, 0, 0}, {0, 1, 0}, {-1, 0, 0}, {0, 0, 1}, distance, pi / 4, pi / 2};

        SECTION("Single wavelength evaluation (eval_geometry)")
        {
            auto [positions, values] = voltage_field.eval_geometry(rect, wavelength, n_dim1, n_dim2);

            // Verify array shapes: surfaces generate correct shape
            REQUIRE(positions.shape().rows == n_dim1);
            REQUIRE(positions.shape().cols == n_dim2);
            REQUIRE(values.shape().rows == n_dim1);
            REQUIRE(values.shape().cols == n_dim2);

            // Verify grid corner positions
            CHECK_THAT(positions(0, 0).x, WithinRel(65.32814824381881635, EPSILON_MAG));
            CHECK_THAT(positions(0, 0).y, WithinRel(65.32814824381883057, EPSILON_MAG));
            CHECK_THAT(positions(0, 0).z, WithinRel(-38.26834323650897574, EPSILON_MAG));
            CHECK_THAT(positions(positions.shape().rows - 1, 0).x, WithinRel(-65.32814824381881635, EPSILON_MAG));
            CHECK_THAT(positions(positions.shape().rows - 1, 0).y, WithinRel(65.32814824381883057, EPSILON_MAG));
            CHECK_THAT(positions(positions.shape().rows - 1, 0).z, WithinRel(-38.26834323650897574, EPSILON_MAG));
            CHECK_THAT(positions(0, positions.shape().cols - 1).x, WithinRel(65.32814824381881635, EPSILON_MAG));
            CHECK_THAT(positions(0, positions.shape().cols - 1).y, WithinRel(65.32814824381883057, EPSILON_MAG));
            CHECK_THAT(positions(0, positions.shape().cols - 1).z, WithinRel(38.26834323650897574, EPSILON_MAG));
            CHECK_THAT(positions(positions.shape().rows - 1, positions.shape().cols - 1).x, WithinRel(-65.32814824381881635, EPSILON_MAG));
            CHECK_THAT(positions(positions.shape().rows - 1, positions.shape().cols - 1).y, WithinRel(65.32814824381883057, EPSILON_MAG));
            CHECK_THAT(positions(positions.shape().rows - 1, positions.shape().cols - 1).z, WithinRel(38.26834323650897574, EPSILON_MAG));

            // Next, find the maximum of the physical values and normalize for better comparison
            double const abs_max = normalize(values);
            CHECK_THAT(abs_max, WithinRel(0.00214713792372675, EPSILON_MAG));

            // Verify physical values along the surface
            CHECK_THAT(std::abs(values(0, 0)), WithinRel(0.0161323981305048, EPSILON_MAG));
            CHECK_THAT(std::abs(values(0, 1)), WithinRel(0.0966088241309794, EPSILON_MAG));
            CHECK_THAT(std::abs(values(0, 2)), WithinRel(0.270909628866502, EPSILON_MAG));
            CHECK_THAT(std::abs(values(0, 3)), WithinRel(0.999999999999391, EPSILON_MAG));
            CHECK_THAT(std::abs(values(0, 4)), WithinRel(0.999999999999391, EPSILON_MAG));
            CHECK_THAT(std::abs(values(0, 5)), WithinRel(0.270909628866501, EPSILON_MAG));
            CHECK_THAT(std::abs(values(0, 6)), WithinRel(0.0966088241310958, EPSILON_MAG));
            CHECK_THAT(std::abs(values(0, 7)), WithinRel(0.0161323981305049, EPSILON_MAG));
            CHECK_THAT(std::abs(values(1, 0)), WithinRel(0.0161323981305376, EPSILON_MAG));
            CHECK_THAT(std::abs(values(1, 1)), WithinRel(0.0966088241308526, EPSILON_MAG));
            CHECK_THAT(std::abs(values(1, 2)), WithinRel(0.270909628866573, EPSILON_MAG));
            CHECK_THAT(std::abs(values(1, 3)), WithinRel(0.999999999999534, EPSILON_MAG));
            CHECK_THAT(std::abs(values(1, 4)), WithinRel(0.999999999999555, EPSILON_MAG));
            CHECK_THAT(std::abs(values(1, 5)), WithinRel(0.270909628866573, EPSILON_MAG));
            CHECK_THAT(std::abs(values(1, 6)), WithinRel(0.0966088241308649, EPSILON_MAG));
            CHECK_THAT(std::abs(values(1, 7)), WithinRel(0.0161323981305376, EPSILON_MAG));
            CHECK_THAT(std::abs(values(2, 0)), WithinRel(0.0161323981306128, EPSILON_MAG));
            CHECK_THAT(std::abs(values(2, 1)), WithinRel(0.0966088241309214, EPSILON_MAG));
            CHECK_THAT(std::abs(values(2, 2)), WithinRel(0.270909628866805, EPSILON_MAG));
            CHECK_THAT(std::abs(values(2, 3)), WithinRel(0.999999999999486, EPSILON_MAG));
            CHECK_THAT(std::abs(values(2, 4)), WithinRel(0.999999999999487, EPSILON_MAG));
            CHECK_THAT(std::abs(values(2, 5)), WithinRel(0.270909628866805, EPSILON_MAG));
            CHECK_THAT(std::abs(values(2, 6)), WithinRel(0.0966088241309214, EPSILON_MAG));
            CHECK_THAT(std::abs(values(2, 7)), WithinRel(0.0161323981306127, EPSILON_MAG));
            CHECK_THAT(std::abs(values(3, 0)), WithinRel(0.0161323981306627, EPSILON_MAG));
            CHECK_THAT(std::abs(values(3, 1)), WithinRel(0.0966088241310159, EPSILON_MAG));
            CHECK_THAT(std::abs(values(3, 2)), WithinRel(0.27090962886659, EPSILON_MAG));
            CHECK_THAT(std::abs(values(3, 3)), WithinRel(0.999999999999756, EPSILON_MAG));
            CHECK_THAT(std::abs(values(3, 4)), WithinRel(0.999999999999756, EPSILON_MAG));
            CHECK_THAT(std::abs(values(3, 5)), WithinRel(0.27090962886659, EPSILON_MAG));
            CHECK_THAT(std::abs(values(3, 6)), WithinRel(0.096608824131016, EPSILON_MAG));
            CHECK_THAT(std::abs(values(3, 7)), WithinRel(0.0161323981306626, EPSILON_MAG));
            CHECK_THAT(std::abs(values(4, 0)), WithinRel(0.0161323981305155, EPSILON_MAG));
            CHECK_THAT(std::abs(values(4, 1)), WithinRel(0.0966088241308565, EPSILON_MAG));
            CHECK_THAT(std::abs(values(4, 2)), WithinRel(0.270909628866603, EPSILON_MAG));
            CHECK_THAT(std::abs(values(4, 3)), WithinRel(0.999999999999401, EPSILON_MAG));
            CHECK_THAT(std::abs(values(4, 4)), WithinRel(0.999999999999401, EPSILON_MAG));
            CHECK_THAT(std::abs(values(4, 5)), WithinRel(0.270909628866603, EPSILON_MAG));
            CHECK_THAT(std::abs(values(4, 6)), WithinRel(0.0966088241308739, EPSILON_MAG));
            CHECK_THAT(std::abs(values(4, 7)), WithinRel(0.0161323981305156, EPSILON_MAG));
            CHECK_THAT(std::abs(values(5, 0)), WithinRel(0.0161323981307005, EPSILON_MAG));
            CHECK_THAT(std::abs(values(5, 1)), WithinRel(0.0966088241308201, EPSILON_MAG));
            CHECK_THAT(std::abs(values(5, 2)), WithinRel(0.270909628866586, EPSILON_MAG));
            CHECK_THAT(std::abs(values(5, 3)), WithinRel(0.99999999999957, EPSILON_MAG));
            CHECK_THAT(std::abs(values(5, 4)), WithinRel(0.99999999999957, EPSILON_MAG));
            CHECK_THAT(std::abs(values(5, 5)), WithinRel(0.270909628866585, EPSILON_MAG));
            CHECK_THAT(std::abs(values(5, 6)), WithinRel(0.0966088241308295, EPSILON_MAG));
            CHECK_THAT(std::abs(values(5, 7)), WithinRel(0.0161323981307005, EPSILON_MAG));
            CHECK_THAT(std::abs(values(6, 0)), WithinRel(0.0161323981305657, EPSILON_MAG));
            CHECK_THAT(std::abs(values(6, 1)), WithinRel(0.0966088241307718, EPSILON_MAG));
            CHECK_THAT(std::abs(values(6, 2)), WithinRel(0.270909628866542, EPSILON_MAG));
            CHECK_THAT(std::abs(values(6, 3)), WithinRel(0.999999999999859, EPSILON_MAG));
            CHECK_THAT(std::abs(values(6, 4)), WithinRel(1, EPSILON_MAG));
            CHECK_THAT(std::abs(values(6, 5)), WithinRel(0.270909628866541, EPSILON_MAG));
            CHECK_THAT(std::abs(values(6, 6)), WithinRel(0.0966088241308371, EPSILON_MAG));
            CHECK_THAT(std::abs(values(6, 7)), WithinRel(0.0161323981305657, EPSILON_MAG));
            CHECK_THAT(std::abs(values(7, 0)), WithinRel(0.0161323981305536, EPSILON_MAG));
            CHECK_THAT(std::abs(values(7, 1)), WithinRel(0.0966088241310956, EPSILON_MAG));
            CHECK_THAT(std::abs(values(7, 2)), WithinRel(0.270909628866735, EPSILON_MAG));
            CHECK_THAT(std::abs(values(7, 3)), WithinRel(0.999999999999692, EPSILON_MAG));
            CHECK_THAT(std::abs(values(7, 4)), WithinRel(0.999999999999692, EPSILON_MAG));
            CHECK_THAT(std::abs(values(7, 5)), WithinRel(0.270909628866735, EPSILON_MAG));
            CHECK_THAT(std::abs(values(7, 6)), WithinRel(0.0966088241310184, EPSILON_MAG));
            CHECK_THAT(std::abs(values(7, 7)), WithinRel(0.0161323981305537, EPSILON_MAG));
            CHECK_THAT(std::abs(values(8, 0)), WithinRel(0.0161323981305536, EPSILON_MAG));
            CHECK_THAT(std::abs(values(8, 1)), WithinRel(0.0966088241310956, EPSILON_MAG));
            CHECK_THAT(std::abs(values(8, 2)), WithinRel(0.270909628866735, EPSILON_MAG));
            CHECK_THAT(std::abs(values(8, 3)), WithinRel(0.999999999999692, EPSILON_MAG));
            CHECK_THAT(std::abs(values(8, 4)), WithinRel(0.999999999999692, EPSILON_MAG));
            CHECK_THAT(std::abs(values(8, 5)), WithinRel(0.270909628866735, EPSILON_MAG));
            CHECK_THAT(std::abs(values(8, 6)), WithinRel(0.0966088241310184, EPSILON_MAG));
            CHECK_THAT(std::abs(values(8, 7)), WithinRel(0.0161323981305537, EPSILON_MAG));
            CHECK_THAT(std::abs(values(9, 0)), WithinRel(0.0161323981305657, EPSILON_MAG));
            CHECK_THAT(std::abs(values(9, 1)), WithinRel(0.0966088241307718, EPSILON_MAG));
            CHECK_THAT(std::abs(values(9, 2)), WithinRel(0.270909628866542, EPSILON_MAG));
            CHECK_THAT(std::abs(values(9, 3)), WithinRel(0.999999999999859, EPSILON_MAG));
            CHECK_THAT(std::abs(values(9, 4)), WithinRel(1, EPSILON_MAG));
            CHECK_THAT(std::abs(values(9, 5)), WithinRel(0.270909628866542, EPSILON_MAG));
            CHECK_THAT(std::abs(values(9, 6)), WithinRel(0.0966088241308371, EPSILON_MAG));
            CHECK_THAT(std::abs(values(9, 7)), WithinRel(0.0161323981305657, EPSILON_MAG));
            CHECK_THAT(std::abs(values(10, 0)), WithinRel(0.0161323981307004, EPSILON_MAG));
            CHECK_THAT(std::abs(values(10, 1)), WithinRel(0.0966088241308201, EPSILON_MAG));
            CHECK_THAT(std::abs(values(10, 2)), WithinRel(0.270909628866586, EPSILON_MAG));
            CHECK_THAT(std::abs(values(10, 3)), WithinRel(0.99999999999957, EPSILON_MAG));
            CHECK_THAT(std::abs(values(10, 4)), WithinRel(0.99999999999957, EPSILON_MAG));
            CHECK_THAT(std::abs(values(10, 5)), WithinRel(0.270909628866585, EPSILON_MAG));
            CHECK_THAT(std::abs(values(10, 6)), WithinRel(0.0966088241308295, EPSILON_MAG));
            CHECK_THAT(std::abs(values(10, 7)), WithinRel(0.0161323981307004, EPSILON_MAG));
            CHECK_THAT(std::abs(values(11, 0)), WithinRel(0.0161323981306158, EPSILON_MAG));
            CHECK_THAT(std::abs(values(11, 1)), WithinRel(0.0966088241310356, EPSILON_MAG));
            CHECK_THAT(std::abs(values(11, 2)), WithinRel(0.270909628866517, EPSILON_MAG));
            CHECK_THAT(std::abs(values(11, 3)), WithinRel(0.999999999999496, EPSILON_MAG));
            CHECK_THAT(std::abs(values(11, 4)), WithinRel(0.999999999999496, EPSILON_MAG));
            CHECK_THAT(std::abs(values(11, 5)), WithinRel(0.270909628866517, EPSILON_MAG));
            CHECK_THAT(std::abs(values(11, 6)), WithinRel(0.0966088241309495, EPSILON_MAG));
            CHECK_THAT(std::abs(values(11, 7)), WithinRel(0.0161323981306157, EPSILON_MAG));
            CHECK_THAT(std::abs(values(12, 0)), WithinRel(0.0161323981306627, EPSILON_MAG));
            CHECK_THAT(std::abs(values(12, 1)), WithinRel(0.096608824131016, EPSILON_MAG));
            CHECK_THAT(std::abs(values(12, 2)), WithinRel(0.27090962886659, EPSILON_MAG));
            CHECK_THAT(std::abs(values(12, 3)), WithinRel(0.999999999999756, EPSILON_MAG));
            CHECK_THAT(std::abs(values(12, 4)), WithinRel(0.999999999999756, EPSILON_MAG));
            CHECK_THAT(std::abs(values(12, 5)), WithinRel(0.27090962886659, EPSILON_MAG));
            CHECK_THAT(std::abs(values(12, 6)), WithinRel(0.096608824131016, EPSILON_MAG));
            CHECK_THAT(std::abs(values(12, 7)), WithinRel(0.0161323981306626, EPSILON_MAG));
            CHECK_THAT(std::abs(values(13, 0)), WithinRel(0.0161323981306128, EPSILON_MAG));
            CHECK_THAT(std::abs(values(13, 1)), WithinRel(0.0966088241309214, EPSILON_MAG));
            CHECK_THAT(std::abs(values(13, 2)), WithinRel(0.270909628866805, EPSILON_MAG));
            CHECK_THAT(std::abs(values(13, 3)), WithinRel(0.999999999999486, EPSILON_MAG));
            CHECK_THAT(std::abs(values(13, 4)), WithinRel(0.999999999999487, EPSILON_MAG));
            CHECK_THAT(std::abs(values(13, 5)), WithinRel(0.270909628866805, EPSILON_MAG));
            CHECK_THAT(std::abs(values(13, 6)), WithinRel(0.0966088241309214, EPSILON_MAG));
            CHECK_THAT(std::abs(values(13, 7)), WithinRel(0.0161323981306128, EPSILON_MAG));
            CHECK_THAT(std::abs(values(14, 0)), WithinRel(0.0161323981305375, EPSILON_MAG));
            CHECK_THAT(std::abs(values(14, 1)), WithinRel(0.0966088241308526, EPSILON_MAG));
            CHECK_THAT(std::abs(values(14, 2)), WithinRel(0.270909628866573, EPSILON_MAG));
            CHECK_THAT(std::abs(values(14, 3)), WithinRel(0.999999999999534, EPSILON_MAG));
            CHECK_THAT(std::abs(values(14, 4)), WithinRel(0.999999999999555, EPSILON_MAG));
            CHECK_THAT(std::abs(values(14, 5)), WithinRel(0.270909628866572, EPSILON_MAG));
            CHECK_THAT(std::abs(values(14, 6)), WithinRel(0.0966088241308648, EPSILON_MAG));
            CHECK_THAT(std::abs(values(14, 7)), WithinRel(0.0161323981305376, EPSILON_MAG));
            CHECK_THAT(std::abs(values(15, 0)), WithinRel(0.0161323981305049, EPSILON_MAG));
            CHECK_THAT(std::abs(values(15, 1)), WithinRel(0.0966088241309794, EPSILON_MAG));
            CHECK_THAT(std::abs(values(15, 2)), WithinRel(0.270909628866502, EPSILON_MAG));
            CHECK_THAT(std::abs(values(15, 3)), WithinRel(0.999999999999391, EPSILON_MAG));
            CHECK_THAT(std::abs(values(15, 4)), WithinRel(0.999999999999391, EPSILON_MAG));
            CHECK_THAT(std::abs(values(15, 5)), WithinRel(0.270909628866501, EPSILON_MAG));
            CHECK_THAT(std::abs(values(15, 6)), WithinRel(0.0966088241310958, EPSILON_MAG));
            CHECK_THAT(std::abs(values(15, 7)), WithinRel(0.0161323981305048, EPSILON_MAG));
            CHECK_THAT(std::arg(values(0, 0)), WithinAbs(1.41879122381658, DELTA_PHASE));
            CHECK_THAT(std::arg(values(0, 1)), WithinAbs(-1.64229502822274, DELTA_PHASE));
            CHECK_THAT(std::arg(values(0, 2)), WithinAbs(1.51299993708328, DELTA_PHASE));
            CHECK_THAT(std::arg(values(0, 3)), WithinAbs(-1.58094085237358, DELTA_PHASE));
            CHECK_THAT(std::arg(values(0, 4)), WithinAbs(-1.58094085237358, DELTA_PHASE));
            CHECK_THAT(std::arg(values(0, 5)), WithinAbs(1.51299993708328, DELTA_PHASE));
            CHECK_THAT(std::arg(values(0, 6)), WithinAbs(-1.64229502822321, DELTA_PHASE));
            CHECK_THAT(std::arg(values(0, 7)), WithinAbs(1.41879122381658, DELTA_PHASE));
            CHECK_THAT(std::arg(values(1, 0)), WithinAbs(1.41879122381387, DELTA_PHASE));
            CHECK_THAT(std::arg(values(1, 1)), WithinAbs(-1.64229502822221, DELTA_PHASE));
            CHECK_THAT(std::arg(values(1, 2)), WithinAbs(1.51299993708435, DELTA_PHASE));
            CHECK_THAT(std::arg(values(1, 3)), WithinAbs(-1.58094085237376, DELTA_PHASE));
            CHECK_THAT(std::arg(values(1, 4)), WithinAbs(-1.58094085237368, DELTA_PHASE));
            CHECK_THAT(std::arg(values(1, 5)), WithinAbs(1.51299993708435, DELTA_PHASE));
            CHECK_THAT(std::arg(values(1, 6)), WithinAbs(-1.64229502822161, DELTA_PHASE));
            CHECK_THAT(std::arg(values(1, 7)), WithinAbs(1.41879122381387, DELTA_PHASE));
            CHECK_THAT(std::arg(values(2, 0)), WithinAbs(1.41879122381051, DELTA_PHASE));
            CHECK_THAT(std::arg(values(2, 1)), WithinAbs(-1.64229502822366, DELTA_PHASE));
            CHECK_THAT(std::arg(values(2, 2)), WithinAbs(1.51299993708404, DELTA_PHASE));
            CHECK_THAT(std::arg(values(2, 3)), WithinAbs(-1.58094085237337, DELTA_PHASE));
            CHECK_THAT(std::arg(values(2, 4)), WithinAbs(-1.58094085237337, DELTA_PHASE));
            CHECK_THAT(std::arg(values(2, 5)), WithinAbs(1.51299993708404, DELTA_PHASE));
            CHECK_THAT(std::arg(values(2, 6)), WithinAbs(-1.64229502822366, DELTA_PHASE));
            CHECK_THAT(std::arg(values(2, 7)), WithinAbs(1.4187912238105, DELTA_PHASE));
            CHECK_THAT(std::arg(values(3, 0)), WithinAbs(1.41879122381355, DELTA_PHASE));
            CHECK_THAT(std::arg(values(3, 1)), WithinAbs(-1.64229502822457, DELTA_PHASE));
            CHECK_THAT(std::arg(values(3, 2)), WithinAbs(1.51299993708421, DELTA_PHASE));
            CHECK_THAT(std::arg(values(3, 3)), WithinAbs(-1.58094085237409, DELTA_PHASE));
            CHECK_THAT(std::arg(values(3, 4)), WithinAbs(-1.58094085237409, DELTA_PHASE));
            CHECK_THAT(std::arg(values(3, 5)), WithinAbs(1.51299993708421, DELTA_PHASE));
            CHECK_THAT(std::arg(values(3, 6)), WithinAbs(-1.64229502822457, DELTA_PHASE));
            CHECK_THAT(std::arg(values(3, 7)), WithinAbs(1.41879122381355, DELTA_PHASE));
            CHECK_THAT(std::arg(values(4, 0)), WithinAbs(1.41879122380683, DELTA_PHASE));
            CHECK_THAT(std::arg(values(4, 1)), WithinAbs(-1.64229502822128, DELTA_PHASE));
            CHECK_THAT(std::arg(values(4, 2)), WithinAbs(1.51299993708441, DELTA_PHASE));
            CHECK_THAT(std::arg(values(4, 3)), WithinAbs(-1.58094085237337, DELTA_PHASE));
            CHECK_THAT(std::arg(values(4, 4)), WithinAbs(-1.58094085237337, DELTA_PHASE));
            CHECK_THAT(std::arg(values(4, 5)), WithinAbs(1.51299993708441, DELTA_PHASE));
            CHECK_THAT(std::arg(values(4, 6)), WithinAbs(-1.64229502822293, DELTA_PHASE));
            CHECK_THAT(std::arg(values(4, 7)), WithinAbs(1.41879122380683, DELTA_PHASE));
            CHECK_THAT(std::arg(values(5, 0)), WithinAbs(1.41879122381167, DELTA_PHASE));
            CHECK_THAT(std::arg(values(5, 1)), WithinAbs(-1.6422950282215, DELTA_PHASE));
            CHECK_THAT(std::arg(values(5, 2)), WithinAbs(1.51299993708512, DELTA_PHASE));
            CHECK_THAT(std::arg(values(5, 3)), WithinAbs(-1.58094085237398, DELTA_PHASE));
            CHECK_THAT(std::arg(values(5, 4)), WithinAbs(-1.58094085237398, DELTA_PHASE));
            CHECK_THAT(std::arg(values(5, 5)), WithinAbs(1.51299993708512, DELTA_PHASE));
            CHECK_THAT(std::arg(values(5, 6)), WithinAbs(-1.64229502822211, DELTA_PHASE));
            CHECK_THAT(std::arg(values(5, 7)), WithinAbs(1.41879122381167, DELTA_PHASE));
            CHECK_THAT(std::arg(values(6, 0)), WithinAbs(1.41879122381508, DELTA_PHASE));
            CHECK_THAT(std::arg(values(6, 1)), WithinAbs(-1.64229502822325, DELTA_PHASE));
            CHECK_THAT(std::arg(values(6, 2)), WithinAbs(1.51299993708512, DELTA_PHASE));
            CHECK_THAT(std::arg(values(6, 3)), WithinAbs(-1.58094085237371, DELTA_PHASE));
            CHECK_THAT(std::arg(values(6, 4)), WithinAbs(-1.58094085237351, DELTA_PHASE));
            CHECK_THAT(std::arg(values(6, 5)), WithinAbs(1.51299993708512, DELTA_PHASE));
            CHECK_THAT(std::arg(values(6, 6)), WithinAbs(-1.64229502822365, DELTA_PHASE));
            CHECK_THAT(std::arg(values(6, 7)), WithinAbs(1.41879122381508, DELTA_PHASE));
            CHECK_THAT(std::arg(values(7, 0)), WithinAbs(1.41879122380825, DELTA_PHASE));
            CHECK_THAT(std::arg(values(7, 1)), WithinAbs(-1.6422950282223, DELTA_PHASE));
            CHECK_THAT(std::arg(values(7, 2)), WithinAbs(1.51299993708358, DELTA_PHASE));
            CHECK_THAT(std::arg(values(7, 3)), WithinAbs(-1.5809408523736, DELTA_PHASE));
            CHECK_THAT(std::arg(values(7, 4)), WithinAbs(-1.5809408523736, DELTA_PHASE));
            CHECK_THAT(std::arg(values(7, 5)), WithinAbs(1.51299993708358, DELTA_PHASE));
            CHECK_THAT(std::arg(values(7, 6)), WithinAbs(-1.64229502822322, DELTA_PHASE));
            CHECK_THAT(std::arg(values(7, 7)), WithinAbs(1.41879122380825, DELTA_PHASE));
            CHECK_THAT(std::arg(values(8, 0)), WithinAbs(1.41879122380825, DELTA_PHASE));
            CHECK_THAT(std::arg(values(8, 1)), WithinAbs(-1.6422950282223, DELTA_PHASE));
            CHECK_THAT(std::arg(values(8, 2)), WithinAbs(1.51299993708358, DELTA_PHASE));
            CHECK_THAT(std::arg(values(8, 3)), WithinAbs(-1.5809408523736, DELTA_PHASE));
            CHECK_THAT(std::arg(values(8, 4)), WithinAbs(-1.5809408523736, DELTA_PHASE));
            CHECK_THAT(std::arg(values(8, 5)), WithinAbs(1.51299993708358, DELTA_PHASE));
            CHECK_THAT(std::arg(values(8, 6)), WithinAbs(-1.64229502822322, DELTA_PHASE));
            CHECK_THAT(std::arg(values(8, 7)), WithinAbs(1.41879122380825, DELTA_PHASE));
            CHECK_THAT(std::arg(values(9, 0)), WithinAbs(1.41879122381508, DELTA_PHASE));
            CHECK_THAT(std::arg(values(9, 1)), WithinAbs(-1.64229502822325, DELTA_PHASE));
            CHECK_THAT(std::arg(values(9, 2)), WithinAbs(1.51299993708512, DELTA_PHASE));
            CHECK_THAT(std::arg(values(9, 3)), WithinAbs(-1.58094085237371, DELTA_PHASE));
            CHECK_THAT(std::arg(values(9, 4)), WithinAbs(-1.58094085237351, DELTA_PHASE));
            CHECK_THAT(std::arg(values(9, 5)), WithinAbs(1.51299993708512, DELTA_PHASE));
            CHECK_THAT(std::arg(values(9, 6)), WithinAbs(-1.64229502822365, DELTA_PHASE));
            CHECK_THAT(std::arg(values(9, 7)), WithinAbs(1.41879122381508, DELTA_PHASE));
            CHECK_THAT(std::arg(values(10, 0)), WithinAbs(1.41879122381167, DELTA_PHASE));
            CHECK_THAT(std::arg(values(10, 1)), WithinAbs(-1.6422950282215, DELTA_PHASE));
            CHECK_THAT(std::arg(values(10, 2)), WithinAbs(1.51299993708512, DELTA_PHASE));
            CHECK_THAT(std::arg(values(10, 3)), WithinAbs(-1.58094085237398, DELTA_PHASE));
            CHECK_THAT(std::arg(values(10, 4)), WithinAbs(-1.58094085237398, DELTA_PHASE));
            CHECK_THAT(std::arg(values(10, 5)), WithinAbs(1.51299993708512, DELTA_PHASE));
            CHECK_THAT(std::arg(values(10, 6)), WithinAbs(-1.64229502822211, DELTA_PHASE));
            CHECK_THAT(std::arg(values(10, 7)), WithinAbs(1.41879122381167, DELTA_PHASE));
            CHECK_THAT(std::arg(values(11, 0)), WithinAbs(1.41879122381584, DELTA_PHASE));
            CHECK_THAT(std::arg(values(11, 1)), WithinAbs(-1.64229502822229, DELTA_PHASE));
            CHECK_THAT(std::arg(values(11, 2)), WithinAbs(1.51299993708506, DELTA_PHASE));
            CHECK_THAT(std::arg(values(11, 3)), WithinAbs(-1.58094085237298, DELTA_PHASE));
            CHECK_THAT(std::arg(values(11, 4)), WithinAbs(-1.58094085237298, DELTA_PHASE));
            CHECK_THAT(std::arg(values(11, 5)), WithinAbs(1.51299993708506, DELTA_PHASE));
            CHECK_THAT(std::arg(values(11, 6)), WithinAbs(-1.64229502822068, DELTA_PHASE));
            CHECK_THAT(std::arg(values(11, 7)), WithinAbs(1.41879122381584, DELTA_PHASE));
            CHECK_THAT(std::arg(values(12, 0)), WithinAbs(1.41879122381355, DELTA_PHASE));
            CHECK_THAT(std::arg(values(12, 1)), WithinAbs(-1.64229502822457, DELTA_PHASE));
            CHECK_THAT(std::arg(values(12, 2)), WithinAbs(1.51299993708421, DELTA_PHASE));
            CHECK_THAT(std::arg(values(12, 3)), WithinAbs(-1.58094085237409, DELTA_PHASE));
            CHECK_THAT(std::arg(values(12, 4)), WithinAbs(-1.58094085237409, DELTA_PHASE));
            CHECK_THAT(std::arg(values(12, 5)), WithinAbs(1.51299993708421, DELTA_PHASE));
            CHECK_THAT(std::arg(values(12, 6)), WithinAbs(-1.64229502822457, DELTA_PHASE));
            CHECK_THAT(std::arg(values(12, 7)), WithinAbs(1.41879122381355, DELTA_PHASE));
            CHECK_THAT(std::arg(values(13, 0)), WithinAbs(1.4187912238105, DELTA_PHASE));
            CHECK_THAT(std::arg(values(13, 1)), WithinAbs(-1.64229502822366, DELTA_PHASE));
            CHECK_THAT(std::arg(values(13, 2)), WithinAbs(1.51299993708404, DELTA_PHASE));
            CHECK_THAT(std::arg(values(13, 3)), WithinAbs(-1.58094085237337, DELTA_PHASE));
            CHECK_THAT(std::arg(values(13, 4)), WithinAbs(-1.58094085237337, DELTA_PHASE));
            CHECK_THAT(std::arg(values(13, 5)), WithinAbs(1.51299993708404, DELTA_PHASE));
            CHECK_THAT(std::arg(values(13, 6)), WithinAbs(-1.64229502822366, DELTA_PHASE));
            CHECK_THAT(std::arg(values(13, 7)), WithinAbs(1.4187912238105, DELTA_PHASE));
            CHECK_THAT(std::arg(values(14, 0)), WithinAbs(1.41879122381387, DELTA_PHASE));
            CHECK_THAT(std::arg(values(14, 1)), WithinAbs(-1.6422950282222, DELTA_PHASE));
            CHECK_THAT(std::arg(values(14, 2)), WithinAbs(1.51299993708435, DELTA_PHASE));
            CHECK_THAT(std::arg(values(14, 3)), WithinAbs(-1.58094085237376, DELTA_PHASE));
            CHECK_THAT(std::arg(values(14, 4)), WithinAbs(-1.58094085237368, DELTA_PHASE));
            CHECK_THAT(std::arg(values(14, 5)), WithinAbs(1.51299993708435, DELTA_PHASE));
            CHECK_THAT(std::arg(values(14, 6)), WithinAbs(-1.64229502822161, DELTA_PHASE));
            CHECK_THAT(std::arg(values(14, 7)), WithinAbs(1.41879122381387, DELTA_PHASE));
            CHECK_THAT(std::arg(values(15, 0)), WithinAbs(1.41879122381658, DELTA_PHASE));
            CHECK_THAT(std::arg(values(15, 1)), WithinAbs(-1.64229502822274, DELTA_PHASE));
            CHECK_THAT(std::arg(values(15, 2)), WithinAbs(1.51299993708328, DELTA_PHASE));
            CHECK_THAT(std::arg(values(15, 3)), WithinAbs(-1.58094085237358, DELTA_PHASE));
            CHECK_THAT(std::arg(values(15, 4)), WithinAbs(-1.58094085237358, DELTA_PHASE));
            CHECK_THAT(std::arg(values(15, 5)), WithinAbs(1.51299993708328, DELTA_PHASE));
            CHECK_THAT(std::arg(values(15, 6)), WithinAbs(-1.64229502822321, DELTA_PHASE));
            CHECK_THAT(std::arg(values(15, 7)), WithinAbs(1.41879122381658, DELTA_PHASE));
        }

        SECTION("Wavelength sweep evaluation (eval_geometry_sweep)")
        {
            auto [positions, data] = voltage_field.eval_geometry_sweep(rect, sweep, n_dim1, n_dim2);

            // Verify array shapes: surfaces generate correct shape
            REQUIRE(positions.shape().rows == n_dim1);
            REQUIRE(positions.shape().cols == n_dim2);
            REQUIRE(data.size() == sweep.size());
            for (std::size_t page = 0; page < sweep.size(); ++page)
            {
                REQUIRE(data[page].shape().rows == n_dim1);
                REQUIRE(data[page].shape().cols == n_dim2);
            }

            // Verify grid corner positions
            CHECK_THAT(positions(0, 0).x, WithinRel(65.32814824381881635, EPSILON_MAG));
            CHECK_THAT(positions(0, 0).y, WithinRel(65.32814824381883057, EPSILON_MAG));
            CHECK_THAT(positions(0, 0).z, WithinRel(-38.26834323650897574, EPSILON_MAG));
            CHECK_THAT(positions(positions.shape().rows - 1, 0).x, WithinRel(-65.32814824381881635, EPSILON_MAG));
            CHECK_THAT(positions(positions.shape().rows - 1, 0).y, WithinRel(65.32814824381883057, EPSILON_MAG));
            CHECK_THAT(positions(positions.shape().rows - 1, 0).z, WithinRel(-38.26834323650897574, EPSILON_MAG));
            CHECK_THAT(positions(0, positions.shape().cols - 1).x, WithinRel(65.32814824381881635, EPSILON_MAG));
            CHECK_THAT(positions(0, positions.shape().cols - 1).y, WithinRel(65.32814824381883057, EPSILON_MAG));
            CHECK_THAT(positions(0, positions.shape().cols - 1).z, WithinRel(38.26834323650897574, EPSILON_MAG));
            CHECK_THAT(positions(positions.shape().rows - 1, positions.shape().cols - 1).x, WithinRel(-65.32814824381881635, EPSILON_MAG));
            CHECK_THAT(positions(positions.shape().rows - 1, positions.shape().cols - 1).y, WithinRel(65.32814824381883057, EPSILON_MAG));
            CHECK_THAT(positions(positions.shape().rows - 1, positions.shape().cols - 1).z, WithinRel(38.26834323650897574, EPSILON_MAG));

            // Verify page 0 (note: this page should be equal to the eval_geometry test)
            {
                auto& values = data[0];

                // Next, find the maximum of the physical values and normalize for better comparison
                double const abs_max = normalize(values);
                CHECK_THAT(abs_max, WithinRel(0.00214713792372675, EPSILON_MAG));

                // Verify physical values along the surface
                CHECK_THAT(std::abs(values(0, 0)), WithinRel(0.0161323981305048, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 1)), WithinRel(0.0966088241309794, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 2)), WithinRel(0.270909628866502, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 3)), WithinRel(0.999999999999391, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 4)), WithinRel(0.999999999999391, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 5)), WithinRel(0.270909628866501, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 6)), WithinRel(0.0966088241310958, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 7)), WithinRel(0.0161323981305049, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 0)), WithinRel(0.0161323981305376, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 1)), WithinRel(0.0966088241308526, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 2)), WithinRel(0.270909628866573, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 3)), WithinRel(0.999999999999534, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 4)), WithinRel(0.999999999999555, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 5)), WithinRel(0.270909628866573, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 6)), WithinRel(0.0966088241308649, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 7)), WithinRel(0.0161323981305376, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 0)), WithinRel(0.0161323981306128, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 1)), WithinRel(0.0966088241309214, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 2)), WithinRel(0.270909628866805, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 3)), WithinRel(0.999999999999486, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 4)), WithinRel(0.999999999999487, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 5)), WithinRel(0.270909628866805, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 6)), WithinRel(0.0966088241309214, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 7)), WithinRel(0.0161323981306127, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 0)), WithinRel(0.0161323981306627, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 1)), WithinRel(0.0966088241310159, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 2)), WithinRel(0.27090962886659, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 3)), WithinRel(0.999999999999756, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 4)), WithinRel(0.999999999999756, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 5)), WithinRel(0.27090962886659, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 6)), WithinRel(0.096608824131016, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 7)), WithinRel(0.0161323981306626, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 0)), WithinRel(0.0161323981305155, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 1)), WithinRel(0.0966088241308565, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 2)), WithinRel(0.270909628866603, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 3)), WithinRel(0.999999999999401, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 4)), WithinRel(0.999999999999401, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 5)), WithinRel(0.270909628866603, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 6)), WithinRel(0.0966088241308739, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 7)), WithinRel(0.0161323981305156, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 0)), WithinRel(0.0161323981307005, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 1)), WithinRel(0.0966088241308201, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 2)), WithinRel(0.270909628866586, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 3)), WithinRel(0.99999999999957, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 4)), WithinRel(0.99999999999957, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 5)), WithinRel(0.270909628866585, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 6)), WithinRel(0.0966088241308295, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 7)), WithinRel(0.0161323981307005, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 0)), WithinRel(0.0161323981305657, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 1)), WithinRel(0.0966088241307718, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 2)), WithinRel(0.270909628866542, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 3)), WithinRel(0.999999999999859, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 4)), WithinRel(1, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 5)), WithinRel(0.270909628866541, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 6)), WithinRel(0.0966088241308371, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 7)), WithinRel(0.0161323981305657, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 0)), WithinRel(0.0161323981305536, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 1)), WithinRel(0.0966088241310956, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 2)), WithinRel(0.270909628866735, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 3)), WithinRel(0.999999999999692, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 4)), WithinRel(0.999999999999692, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 5)), WithinRel(0.270909628866735, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 6)), WithinRel(0.0966088241310184, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 7)), WithinRel(0.0161323981305537, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 0)), WithinRel(0.0161323981305536, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 1)), WithinRel(0.0966088241310956, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 2)), WithinRel(0.270909628866735, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 3)), WithinRel(0.999999999999692, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 4)), WithinRel(0.999999999999692, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 5)), WithinRel(0.270909628866735, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 6)), WithinRel(0.0966088241310184, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 7)), WithinRel(0.0161323981305537, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 0)), WithinRel(0.0161323981305657, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 1)), WithinRel(0.0966088241307718, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 2)), WithinRel(0.270909628866542, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 3)), WithinRel(0.999999999999859, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 4)), WithinRel(1, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 5)), WithinRel(0.270909628866542, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 6)), WithinRel(0.0966088241308371, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 7)), WithinRel(0.0161323981305657, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 0)), WithinRel(0.0161323981307004, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 1)), WithinRel(0.0966088241308201, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 2)), WithinRel(0.270909628866586, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 3)), WithinRel(0.99999999999957, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 4)), WithinRel(0.99999999999957, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 5)), WithinRel(0.270909628866585, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 6)), WithinRel(0.0966088241308295, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 7)), WithinRel(0.0161323981307004, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 0)), WithinRel(0.0161323981306158, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 1)), WithinRel(0.0966088241310356, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 2)), WithinRel(0.270909628866517, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 3)), WithinRel(0.999999999999496, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 4)), WithinRel(0.999999999999496, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 5)), WithinRel(0.270909628866517, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 6)), WithinRel(0.0966088241309495, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 7)), WithinRel(0.0161323981306157, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 0)), WithinRel(0.0161323981306627, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 1)), WithinRel(0.096608824131016, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 2)), WithinRel(0.27090962886659, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 3)), WithinRel(0.999999999999756, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 4)), WithinRel(0.999999999999756, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 5)), WithinRel(0.27090962886659, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 6)), WithinRel(0.096608824131016, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 7)), WithinRel(0.0161323981306626, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 0)), WithinRel(0.0161323981306128, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 1)), WithinRel(0.0966088241309214, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 2)), WithinRel(0.270909628866805, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 3)), WithinRel(0.999999999999486, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 4)), WithinRel(0.999999999999487, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 5)), WithinRel(0.270909628866805, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 6)), WithinRel(0.0966088241309214, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 7)), WithinRel(0.0161323981306128, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 0)), WithinRel(0.0161323981305375, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 1)), WithinRel(0.0966088241308526, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 2)), WithinRel(0.270909628866573, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 3)), WithinRel(0.999999999999534, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 4)), WithinRel(0.999999999999555, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 5)), WithinRel(0.270909628866572, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 6)), WithinRel(0.0966088241308648, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 7)), WithinRel(0.0161323981305376, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 0)), WithinRel(0.0161323981305049, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 1)), WithinRel(0.0966088241309794, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 2)), WithinRel(0.270909628866502, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 3)), WithinRel(0.999999999999391, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 4)), WithinRel(0.999999999999391, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 5)), WithinRel(0.270909628866501, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 6)), WithinRel(0.0966088241310958, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 7)), WithinRel(0.0161323981305048, EPSILON_MAG));
                CHECK_THAT(std::arg(values(0, 0)), WithinAbs(1.41879122381658, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 1)), WithinAbs(-1.64229502822274, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 2)), WithinAbs(1.51299993708328, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 3)), WithinAbs(-1.58094085237358, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 4)), WithinAbs(-1.58094085237358, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 5)), WithinAbs(1.51299993708328, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 6)), WithinAbs(-1.64229502822321, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 7)), WithinAbs(1.41879122381658, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 0)), WithinAbs(1.41879122381387, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 1)), WithinAbs(-1.64229502822221, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 2)), WithinAbs(1.51299993708435, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 3)), WithinAbs(-1.58094085237376, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 4)), WithinAbs(-1.58094085237368, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 5)), WithinAbs(1.51299993708435, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 6)), WithinAbs(-1.64229502822161, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 7)), WithinAbs(1.41879122381387, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 0)), WithinAbs(1.41879122381051, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 1)), WithinAbs(-1.64229502822366, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 2)), WithinAbs(1.51299993708404, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 3)), WithinAbs(-1.58094085237337, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 4)), WithinAbs(-1.58094085237337, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 5)), WithinAbs(1.51299993708404, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 6)), WithinAbs(-1.64229502822366, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 7)), WithinAbs(1.4187912238105, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 0)), WithinAbs(1.41879122381355, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 1)), WithinAbs(-1.64229502822457, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 2)), WithinAbs(1.51299993708421, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 3)), WithinAbs(-1.58094085237409, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 4)), WithinAbs(-1.58094085237409, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 5)), WithinAbs(1.51299993708421, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 6)), WithinAbs(-1.64229502822457, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 7)), WithinAbs(1.41879122381355, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 0)), WithinAbs(1.41879122380683, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 1)), WithinAbs(-1.64229502822128, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 2)), WithinAbs(1.51299993708441, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 3)), WithinAbs(-1.58094085237337, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 4)), WithinAbs(-1.58094085237337, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 5)), WithinAbs(1.51299993708441, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 6)), WithinAbs(-1.64229502822293, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 7)), WithinAbs(1.41879122380683, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 0)), WithinAbs(1.41879122381167, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 1)), WithinAbs(-1.6422950282215, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 2)), WithinAbs(1.51299993708512, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 3)), WithinAbs(-1.58094085237398, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 4)), WithinAbs(-1.58094085237398, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 5)), WithinAbs(1.51299993708512, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 6)), WithinAbs(-1.64229502822211, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 7)), WithinAbs(1.41879122381167, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 0)), WithinAbs(1.41879122381508, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 1)), WithinAbs(-1.64229502822325, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 2)), WithinAbs(1.51299993708512, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 3)), WithinAbs(-1.58094085237371, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 4)), WithinAbs(-1.58094085237351, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 5)), WithinAbs(1.51299993708512, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 6)), WithinAbs(-1.64229502822365, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 7)), WithinAbs(1.41879122381508, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 0)), WithinAbs(1.41879122380825, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 1)), WithinAbs(-1.6422950282223, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 2)), WithinAbs(1.51299993708358, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 3)), WithinAbs(-1.5809408523736, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 4)), WithinAbs(-1.5809408523736, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 5)), WithinAbs(1.51299993708358, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 6)), WithinAbs(-1.64229502822322, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 7)), WithinAbs(1.41879122380825, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 0)), WithinAbs(1.41879122380825, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 1)), WithinAbs(-1.6422950282223, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 2)), WithinAbs(1.51299993708358, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 3)), WithinAbs(-1.5809408523736, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 4)), WithinAbs(-1.5809408523736, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 5)), WithinAbs(1.51299993708358, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 6)), WithinAbs(-1.64229502822322, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 7)), WithinAbs(1.41879122380825, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 0)), WithinAbs(1.41879122381508, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 1)), WithinAbs(-1.64229502822325, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 2)), WithinAbs(1.51299993708512, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 3)), WithinAbs(-1.58094085237371, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 4)), WithinAbs(-1.58094085237351, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 5)), WithinAbs(1.51299993708512, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 6)), WithinAbs(-1.64229502822365, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 7)), WithinAbs(1.41879122381508, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 0)), WithinAbs(1.41879122381167, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 1)), WithinAbs(-1.6422950282215, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 2)), WithinAbs(1.51299993708512, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 3)), WithinAbs(-1.58094085237398, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 4)), WithinAbs(-1.58094085237398, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 5)), WithinAbs(1.51299993708512, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 6)), WithinAbs(-1.64229502822211, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 7)), WithinAbs(1.41879122381167, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 0)), WithinAbs(1.41879122381584, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 1)), WithinAbs(-1.64229502822229, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 2)), WithinAbs(1.51299993708506, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 3)), WithinAbs(-1.58094085237298, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 4)), WithinAbs(-1.58094085237298, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 5)), WithinAbs(1.51299993708506, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 6)), WithinAbs(-1.64229502822068, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 7)), WithinAbs(1.41879122381584, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 0)), WithinAbs(1.41879122381355, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 1)), WithinAbs(-1.64229502822457, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 2)), WithinAbs(1.51299993708421, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 3)), WithinAbs(-1.58094085237409, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 4)), WithinAbs(-1.58094085237409, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 5)), WithinAbs(1.51299993708421, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 6)), WithinAbs(-1.64229502822457, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 7)), WithinAbs(1.41879122381355, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 0)), WithinAbs(1.4187912238105, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 1)), WithinAbs(-1.64229502822366, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 2)), WithinAbs(1.51299993708404, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 3)), WithinAbs(-1.58094085237337, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 4)), WithinAbs(-1.58094085237337, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 5)), WithinAbs(1.51299993708404, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 6)), WithinAbs(-1.64229502822366, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 7)), WithinAbs(1.4187912238105, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 0)), WithinAbs(1.41879122381387, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 1)), WithinAbs(-1.6422950282222, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 2)), WithinAbs(1.51299993708435, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 3)), WithinAbs(-1.58094085237376, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 4)), WithinAbs(-1.58094085237368, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 5)), WithinAbs(1.51299993708435, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 6)), WithinAbs(-1.64229502822161, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 7)), WithinAbs(1.41879122381387, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 0)), WithinAbs(1.41879122381658, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 1)), WithinAbs(-1.64229502822274, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 2)), WithinAbs(1.51299993708328, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 3)), WithinAbs(-1.58094085237358, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 4)), WithinAbs(-1.58094085237358, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 5)), WithinAbs(1.51299993708328, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 6)), WithinAbs(-1.64229502822321, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 7)), WithinAbs(1.41879122381658, DELTA_PHASE));
            }

            // Verify page 1
            {
                auto& values = data[1];

                // Next, find the maximum of the physical values and normalize for better comparison
                double const abs_max = normalize(values);
                CHECK_THAT(abs_max, WithinRel(0.00291527234595958, EPSILON_MAG));

                // Verify physical values along the surface
                CHECK_THAT(std::abs(values(0, 0)), WithinRel(0.0179844363415044, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 1)), WithinRel(0.219074952276568, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 2)), WithinRel(0.130694634215046, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 3)), WithinRel(0.99999999999972, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 4)), WithinRel(0.99999999999972, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 5)), WithinRel(0.130694634215046, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 6)), WithinRel(0.219074952276612, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 7)), WithinRel(0.0179844363415045, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 0)), WithinRel(0.017984436341519, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 1)), WithinRel(0.219074952276687, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 2)), WithinRel(0.130694634214944, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 3)), WithinRel(0.999999999999826, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 4)), WithinRel(0.999999999999837, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 5)), WithinRel(0.130694634214944, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 6)), WithinRel(0.21907495227674, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 7)), WithinRel(0.0179844363415191, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 0)), WithinRel(0.0179844363415261, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 1)), WithinRel(0.21907495227676, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 2)), WithinRel(0.130694634214872, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 3)), WithinRel(0.999999999999829, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 4)), WithinRel(0.999999999999829, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 5)), WithinRel(0.130694634214872, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 6)), WithinRel(0.21907495227676, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 7)), WithinRel(0.0179844363415262, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 0)), WithinRel(0.0179844363415625, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 1)), WithinRel(0.21907495227653, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 2)), WithinRel(0.130694634215113, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 3)), WithinRel(0.999999999999958, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 4)), WithinRel(0.999999999999958, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 5)), WithinRel(0.130694634215112, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 6)), WithinRel(0.21907495227653, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 7)), WithinRel(0.0179844363415626, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 0)), WithinRel(0.0179844363414722, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 1)), WithinRel(0.219074952276715, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 2)), WithinRel(0.130694634214987, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 3)), WithinRel(0.999999999999749, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 4)), WithinRel(0.999999999999749, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 5)), WithinRel(0.130694634214987, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 6)), WithinRel(0.219074952276657, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 7)), WithinRel(0.0179844363414723, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 0)), WithinRel(0.0179844363415668, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 1)), WithinRel(0.219074952276781, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 2)), WithinRel(0.130694634215074, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 3)), WithinRel(0.999999999999841, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 4)), WithinRel(0.999999999999841, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 5)), WithinRel(0.130694634215074, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 6)), WithinRel(0.219074952276729, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 7)), WithinRel(0.0179844363415668, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 0)), WithinRel(0.0179844363414931, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 1)), WithinRel(0.219074952276746, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 2)), WithinRel(0.130694634214992, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 3)), WithinRel(0.999999999999974, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 4)), WithinRel(1, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 5)), WithinRel(0.130694634214992, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 6)), WithinRel(0.219074952276694, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 7)), WithinRel(0.0179844363414932, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 0)), WithinRel(0.0179844363414607, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 1)), WithinRel(0.219074952276704, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 2)), WithinRel(0.130694634214834, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 3)), WithinRel(0.999999999999837, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 4)), WithinRel(0.999999999999837, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 5)), WithinRel(0.130694634214834, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 6)), WithinRel(0.219074952276674, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 7)), WithinRel(0.0179844363414608, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 0)), WithinRel(0.0179844363414607, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 1)), WithinRel(0.219074952276704, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 2)), WithinRel(0.130694634214834, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 3)), WithinRel(0.999999999999837, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 4)), WithinRel(0.999999999999837, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 5)), WithinRel(0.130694634214834, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 6)), WithinRel(0.219074952276674, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 7)), WithinRel(0.0179844363414608, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 0)), WithinRel(0.0179844363414931, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 1)), WithinRel(0.219074952276746, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 2)), WithinRel(0.130694634214992, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 3)), WithinRel(0.999999999999974, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 4)), WithinRel(1, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 5)), WithinRel(0.130694634214992, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 6)), WithinRel(0.219074952276694, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 7)), WithinRel(0.0179844363414932, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 0)), WithinRel(0.0179844363415668, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 1)), WithinRel(0.219074952276781, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 2)), WithinRel(0.130694634215074, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 3)), WithinRel(0.999999999999841, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 4)), WithinRel(0.999999999999841, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 5)), WithinRel(0.130694634215074, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 6)), WithinRel(0.219074952276729, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 7)), WithinRel(0.0179844363415669, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 0)), WithinRel(0.0179844363414709, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 1)), WithinRel(0.21907495227672, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 2)), WithinRel(0.130694634215029, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 3)), WithinRel(0.999999999999804, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 4)), WithinRel(0.999999999999804, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 5)), WithinRel(0.130694634215029, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 6)), WithinRel(0.219074952276756, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 7)), WithinRel(0.017984436341471, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 0)), WithinRel(0.0179844363415625, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 1)), WithinRel(0.21907495227653, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 2)), WithinRel(0.130694634215113, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 3)), WithinRel(0.999999999999958, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 4)), WithinRel(0.999999999999958, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 5)), WithinRel(0.130694634215112, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 6)), WithinRel(0.21907495227653, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 7)), WithinRel(0.0179844363415626, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 0)), WithinRel(0.0179844363415261, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 1)), WithinRel(0.21907495227676, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 2)), WithinRel(0.130694634214872, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 3)), WithinRel(0.999999999999829, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 4)), WithinRel(0.999999999999829, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 5)), WithinRel(0.130694634214872, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 6)), WithinRel(0.21907495227676, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 7)), WithinRel(0.0179844363415262, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 0)), WithinRel(0.017984436341519, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 1)), WithinRel(0.219074952276687, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 2)), WithinRel(0.130694634214944, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 3)), WithinRel(0.999999999999826, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 4)), WithinRel(0.999999999999837, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 5)), WithinRel(0.130694634214944, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 6)), WithinRel(0.21907495227674, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 7)), WithinRel(0.0179844363415191, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 0)), WithinRel(0.0179844363415044, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 1)), WithinRel(0.219074952276568, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 2)), WithinRel(0.130694634215046, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 3)), WithinRel(0.99999999999972, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 4)), WithinRel(0.99999999999972, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 5)), WithinRel(0.130694634215046, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 6)), WithinRel(0.219074952276612, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 7)), WithinRel(0.0179844363415045, EPSILON_MAG));
                CHECK_THAT(std::arg(values(0, 0)), WithinAbs(0.377707600726311, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 1)), WithinAbs(-2.64634107528761, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 2)), WithinAbs(0.575625867868319, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 3)), WithinAbs(0.514275616713248, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 4)), WithinAbs(0.514275616713248, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 5)), WithinAbs(0.575625867868318, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 6)), WithinAbs(-2.64634107528697, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 7)), WithinAbs(0.377707600726312, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 0)), WithinAbs(0.377707600730267, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 1)), WithinAbs(-2.64634107528804, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 2)), WithinAbs(0.575625867868742, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 3)), WithinAbs(0.514275616713096, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 4)), WithinAbs(0.514275616713161, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 5)), WithinAbs(0.575625867868742, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 6)), WithinAbs(-2.64634107528794, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 7)), WithinAbs(0.37770760073027, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 0)), WithinAbs(0.377707600727274, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 1)), WithinAbs(-2.64634107528773, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 2)), WithinAbs(0.575625867868465, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 3)), WithinAbs(0.514275616713431, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 4)), WithinAbs(0.514275616713431, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 5)), WithinAbs(0.575625867868464, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 6)), WithinAbs(-2.64634107528773, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 7)), WithinAbs(0.377707600727276, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 0)), WithinAbs(0.377707600724902, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 1)), WithinAbs(-2.64634107528776, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 2)), WithinAbs(0.575625867868195, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 3)), WithinAbs(0.514275616712955, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 4)), WithinAbs(0.514275616712955, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 5)), WithinAbs(0.575625867868194, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 6)), WithinAbs(-2.64634107528776, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 7)), WithinAbs(0.3777076007249, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 0)), WithinAbs(0.377707600735099, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 1)), WithinAbs(-2.6463410752884, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 2)), WithinAbs(0.575625867868665, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 3)), WithinAbs(0.514275616713339, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 4)), WithinAbs(0.514275616713339, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 5)), WithinAbs(0.575625867868664, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 6)), WithinAbs(-2.64634107528799, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 7)), WithinAbs(0.377707600735099, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 0)), WithinAbs(0.37770760073266, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 1)), WithinAbs(-2.64634107528749, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 2)), WithinAbs(0.575625867867775, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 3)), WithinAbs(0.514275616712961, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 4)), WithinAbs(0.514275616712961, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 5)), WithinAbs(0.575625867867774, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 6)), WithinAbs(-2.64634107528738, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 7)), WithinAbs(0.377707600732659, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 0)), WithinAbs(0.377707600727765, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 1)), WithinAbs(-2.64634107528739, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 2)), WithinAbs(0.575625867867103, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 3)), WithinAbs(0.514275616713199, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 4)), WithinAbs(0.514275616713259, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 5)), WithinAbs(0.575625867867103, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 6)), WithinAbs(-2.64634107528728, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 7)), WithinAbs(0.377707600727765, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 0)), WithinAbs(0.377707600731951, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 1)), WithinAbs(-2.64634107528807, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 2)), WithinAbs(0.575625867867917, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 3)), WithinAbs(0.51427561671336, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 4)), WithinAbs(0.51427561671336, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 5)), WithinAbs(0.575625867867916, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 6)), WithinAbs(-2.64634107528785, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 7)), WithinAbs(0.377707600731954, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 0)), WithinAbs(0.377707600731951, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 1)), WithinAbs(-2.64634107528807, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 2)), WithinAbs(0.575625867867917, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 3)), WithinAbs(0.51427561671336, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 4)), WithinAbs(0.51427561671336, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 5)), WithinAbs(0.575625867867916, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 6)), WithinAbs(-2.64634107528785, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 7)), WithinAbs(0.377707600731954, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 0)), WithinAbs(0.377707600727764, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 1)), WithinAbs(-2.64634107528739, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 2)), WithinAbs(0.575625867867103, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 3)), WithinAbs(0.514275616713199, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 4)), WithinAbs(0.514275616713259, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 5)), WithinAbs(0.575625867867103, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 6)), WithinAbs(-2.64634107528728, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 7)), WithinAbs(0.377707600727763, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 0)), WithinAbs(0.377707600732658, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 1)), WithinAbs(-2.64634107528749, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 2)), WithinAbs(0.575625867867775, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 3)), WithinAbs(0.514275616712961, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 4)), WithinAbs(0.514275616712961, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 5)), WithinAbs(0.575625867867774, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 6)), WithinAbs(-2.64634107528738, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 7)), WithinAbs(0.377707600732659, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 0)), WithinAbs(0.37770760072846, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 1)), WithinAbs(-2.64634107528704, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 2)), WithinAbs(0.575625867869018, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 3)), WithinAbs(0.514275616713732, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 4)), WithinAbs(0.514275616713732, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 5)), WithinAbs(0.575625867869018, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 6)), WithinAbs(-2.64634107528754, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 7)), WithinAbs(0.37770760072846, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 0)), WithinAbs(0.377707600724902, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 1)), WithinAbs(-2.64634107528776, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 2)), WithinAbs(0.575625867868195, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 3)), WithinAbs(0.514275616712955, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 4)), WithinAbs(0.514275616712955, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 5)), WithinAbs(0.575625867868194, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 6)), WithinAbs(-2.64634107528776, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 7)), WithinAbs(0.377707600724902, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 0)), WithinAbs(0.377707600727274, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 1)), WithinAbs(-2.64634107528773, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 2)), WithinAbs(0.575625867868465, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 3)), WithinAbs(0.514275616713431, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 4)), WithinAbs(0.514275616713431, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 5)), WithinAbs(0.575625867868464, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 6)), WithinAbs(-2.64634107528773, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 7)), WithinAbs(0.377707600727276, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 0)), WithinAbs(0.377707600730268, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 1)), WithinAbs(-2.64634107528804, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 2)), WithinAbs(0.575625867868742, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 3)), WithinAbs(0.514275616713096, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 4)), WithinAbs(0.514275616713161, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 5)), WithinAbs(0.575625867868742, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 6)), WithinAbs(-2.64634107528794, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 7)), WithinAbs(0.377707600730268, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 0)), WithinAbs(0.377707600726309, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 1)), WithinAbs(-2.64634107528761, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 2)), WithinAbs(0.575625867868319, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 3)), WithinAbs(0.514275616713248, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 4)), WithinAbs(0.514275616713248, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 5)), WithinAbs(0.575625867868319, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 6)), WithinAbs(-2.64634107528697, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 7)), WithinAbs(0.377707600726311, DELTA_PHASE));
            }

            // Verify page 2
            {
                auto& values = data[2];

                // Next, find the maximum of the physical values and normalize for better comparison
                double const abs_max = normalize(values);
                CHECK_THAT(abs_max, WithinRel(0.0038297001770499, EPSILON_MAG));

                // Verify physical values along the surface
                CHECK_THAT(std::abs(values(0, 0)), WithinRel(0.18273276267708, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 1)), WithinRel(0.0933965070365704, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 2)), WithinRel(0.429713741670144, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 3)), WithinRel(0.999999999999868, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 4)), WithinRel(0.999999999999868, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 5)), WithinRel(0.429713741670144, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 6)), WithinRel(0.0933965070366334, EPSILON_MAG));
                CHECK_THAT(std::abs(values(0, 7)), WithinRel(0.18273276267708, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 0)), WithinRel(0.182732762677051, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 1)), WithinRel(0.0933965070366252, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 2)), WithinRel(0.42971374167009, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 3)), WithinRel(0.999999999999903, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 4)), WithinRel(0.999999999999907, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 5)), WithinRel(0.42971374167009, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 6)), WithinRel(0.0933965070366272, EPSILON_MAG));
                CHECK_THAT(std::abs(values(1, 7)), WithinRel(0.182732762677051, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 0)), WithinRel(0.182732762677067, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 1)), WithinRel(0.0933965070366563, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 2)), WithinRel(0.429713741670057, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 3)), WithinRel(0.999999999999891, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 4)), WithinRel(0.999999999999891, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 5)), WithinRel(0.429713741670057, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 6)), WithinRel(0.0933965070366563, EPSILON_MAG));
                CHECK_THAT(std::abs(values(2, 7)), WithinRel(0.182732762677067, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 0)), WithinRel(0.182732762676932, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 1)), WithinRel(0.0933965070365914, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 2)), WithinRel(0.429713741670191, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 3)), WithinRel(0.999999999999951, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 4)), WithinRel(0.999999999999951, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 5)), WithinRel(0.429713741670191, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 6)), WithinRel(0.0933965070365914, EPSILON_MAG));
                CHECK_THAT(std::abs(values(3, 7)), WithinRel(0.182732762676932, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 0)), WithinRel(0.182732762677024, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 1)), WithinRel(0.0933965070366629, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 2)), WithinRel(0.429713741670126, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 3)), WithinRel(0.999999999999872, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 4)), WithinRel(0.999999999999872, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 5)), WithinRel(0.429713741670126, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 6)), WithinRel(0.093396507036612, EPSILON_MAG));
                CHECK_THAT(std::abs(values(4, 7)), WithinRel(0.182732762677024, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 0)), WithinRel(0.182732762677045, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 1)), WithinRel(0.0933965070366875, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 2)), WithinRel(0.429713741670141, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 3)), WithinRel(0.999999999999911, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 4)), WithinRel(0.999999999999911, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 5)), WithinRel(0.429713741670141, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 6)), WithinRel(0.0933965070366601, EPSILON_MAG));
                CHECK_THAT(std::abs(values(5, 7)), WithinRel(0.182732762677045, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 0)), WithinRel(0.18273276267704, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 1)), WithinRel(0.0933965070367514, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 2)), WithinRel(0.429713741670093, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 3)), WithinRel(0.999999999999972, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 4)), WithinRel(1, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 5)), WithinRel(0.429713741670093, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 6)), WithinRel(0.0933965070367087, EPSILON_MAG));
                CHECK_THAT(std::abs(values(6, 7)), WithinRel(0.18273276267704, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 0)), WithinRel(0.182732762677094, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 1)), WithinRel(0.0933965070367039, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 2)), WithinRel(0.429713741670054, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 3)), WithinRel(0.999999999999931, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 4)), WithinRel(0.999999999999931, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 5)), WithinRel(0.429713741670054, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 6)), WithinRel(0.0933965070366516, EPSILON_MAG));
                CHECK_THAT(std::abs(values(7, 7)), WithinRel(0.182732762677094, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 0)), WithinRel(0.182732762677094, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 1)), WithinRel(0.0933965070367039, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 2)), WithinRel(0.429713741670054, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 3)), WithinRel(0.999999999999931, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 4)), WithinRel(0.999999999999931, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 5)), WithinRel(0.429713741670054, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 6)), WithinRel(0.0933965070366516, EPSILON_MAG));
                CHECK_THAT(std::abs(values(8, 7)), WithinRel(0.182732762677094, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 0)), WithinRel(0.18273276267704, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 1)), WithinRel(0.0933965070367514, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 2)), WithinRel(0.429713741670093, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 3)), WithinRel(0.999999999999972, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 4)), WithinRel(1, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 5)), WithinRel(0.429713741670093, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 6)), WithinRel(0.0933965070367087, EPSILON_MAG));
                CHECK_THAT(std::abs(values(9, 7)), WithinRel(0.18273276267704, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 0)), WithinRel(0.182732762677045, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 1)), WithinRel(0.0933965070366875, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 2)), WithinRel(0.429713741670141, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 3)), WithinRel(0.999999999999911, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 4)), WithinRel(0.999999999999911, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 5)), WithinRel(0.429713741670141, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 6)), WithinRel(0.0933965070366601, EPSILON_MAG));
                CHECK_THAT(std::abs(values(10, 7)), WithinRel(0.182732762677045, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 0)), WithinRel(0.182732762677025, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 1)), WithinRel(0.0933965070366845, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 2)), WithinRel(0.429713741670111, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 3)), WithinRel(0.999999999999891, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 4)), WithinRel(0.999999999999891, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 5)), WithinRel(0.429713741670111, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 6)), WithinRel(0.0933965070367077, EPSILON_MAG));
                CHECK_THAT(std::abs(values(11, 7)), WithinRel(0.182732762677025, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 0)), WithinRel(0.182732762676932, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 1)), WithinRel(0.0933965070365915, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 2)), WithinRel(0.429713741670191, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 3)), WithinRel(0.999999999999951, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 4)), WithinRel(0.999999999999951, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 5)), WithinRel(0.429713741670191, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 6)), WithinRel(0.0933965070365914, EPSILON_MAG));
                CHECK_THAT(std::abs(values(12, 7)), WithinRel(0.182732762676932, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 0)), WithinRel(0.182732762677067, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 1)), WithinRel(0.0933965070366563, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 2)), WithinRel(0.429713741670057, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 3)), WithinRel(0.999999999999891, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 4)), WithinRel(0.999999999999891, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 5)), WithinRel(0.429713741670057, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 6)), WithinRel(0.0933965070366563, EPSILON_MAG));
                CHECK_THAT(std::abs(values(13, 7)), WithinRel(0.182732762677067, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 0)), WithinRel(0.182732762677051, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 1)), WithinRel(0.0933965070366252, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 2)), WithinRel(0.42971374167009, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 3)), WithinRel(0.999999999999903, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 4)), WithinRel(0.999999999999907, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 5)), WithinRel(0.42971374167009, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 6)), WithinRel(0.0933965070366271, EPSILON_MAG));
                CHECK_THAT(std::abs(values(14, 7)), WithinRel(0.182732762677051, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 0)), WithinRel(0.18273276267708, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 1)), WithinRel(0.0933965070365703, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 2)), WithinRel(0.429713741670144, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 3)), WithinRel(0.999999999999868, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 4)), WithinRel(0.999999999999868, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 5)), WithinRel(0.429713741670144, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 6)), WithinRel(0.0933965070366334, EPSILON_MAG));
                CHECK_THAT(std::abs(values(15, 7)), WithinRel(0.18273276267708, EPSILON_MAG));
                CHECK_THAT(std::arg(values(0, 0)), WithinAbs(1.55375648968762, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 1)), WithinAbs(1.50296496872655, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 2)), WithinAbs(-1.56778243604873, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 3)), WithinAbs(-1.57833127447549, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 4)), WithinAbs(-1.57833127447549, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 5)), WithinAbs(-1.56778243604873, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 6)), WithinAbs(1.50296496872701, DELTA_PHASE));
                CHECK_THAT(std::arg(values(0, 7)), WithinAbs(1.55375648968762, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 0)), WithinAbs(1.55375648968767, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 1)), WithinAbs(1.50296496872625, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 2)), WithinAbs(-1.56778243604838, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 3)), WithinAbs(-1.57833127447559, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 4)), WithinAbs(-1.57833127447556, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 5)), WithinAbs(-1.56778243604838, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 6)), WithinAbs(1.50296496872654, DELTA_PHASE));
                CHECK_THAT(std::arg(values(1, 7)), WithinAbs(1.55375648968767, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 0)), WithinAbs(1.55375648968793, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 1)), WithinAbs(1.50296496872634, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 2)), WithinAbs(-1.56778243604858, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 3)), WithinAbs(-1.5783312744754, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 4)), WithinAbs(-1.5783312744754, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 5)), WithinAbs(-1.56778243604858, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 6)), WithinAbs(1.50296496872634, DELTA_PHASE));
                CHECK_THAT(std::arg(values(2, 7)), WithinAbs(1.55375648968793, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 0)), WithinAbs(1.5537564896873, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 1)), WithinAbs(1.50296496872593, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 2)), WithinAbs(-1.56778243604858, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 3)), WithinAbs(-1.57833127447572, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 4)), WithinAbs(-1.57833127447572, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 5)), WithinAbs(-1.56778243604858, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 6)), WithinAbs(1.50296496872593, DELTA_PHASE));
                CHECK_THAT(std::arg(values(3, 7)), WithinAbs(1.5537564896873, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 0)), WithinAbs(1.55375648968684, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 1)), WithinAbs(1.50296496872599, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 2)), WithinAbs(-1.56778243604852, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 3)), WithinAbs(-1.57833127447542, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 4)), WithinAbs(-1.57833127447542, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 5)), WithinAbs(-1.56778243604852, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 6)), WithinAbs(1.50296496872616, DELTA_PHASE));
                CHECK_THAT(std::arg(values(4, 7)), WithinAbs(1.55375648968684, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 0)), WithinAbs(1.5537564896872, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 1)), WithinAbs(1.50296496872672, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 2)), WithinAbs(-1.5677824360485, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 3)), WithinAbs(-1.57833127447573, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 4)), WithinAbs(-1.57833127447573, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 5)), WithinAbs(-1.5677824360485, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 6)), WithinAbs(1.50296496872668, DELTA_PHASE));
                CHECK_THAT(std::arg(values(5, 7)), WithinAbs(1.5537564896872, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 0)), WithinAbs(1.55375648968717, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 1)), WithinAbs(1.50296496872661, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 2)), WithinAbs(-1.56778243604868, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 3)), WithinAbs(-1.57833127447557, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 4)), WithinAbs(-1.57833127447548, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 5)), WithinAbs(-1.56778243604869, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 6)), WithinAbs(1.50296496872633, DELTA_PHASE));
                CHECK_THAT(std::arg(values(6, 7)), WithinAbs(1.55375648968717, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 0)), WithinAbs(1.55375648968761, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 1)), WithinAbs(1.50296496872594, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 2)), WithinAbs(-1.56778243604873, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 3)), WithinAbs(-1.57833127447549, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 4)), WithinAbs(-1.57833127447549, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 5)), WithinAbs(-1.56778243604873, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 6)), WithinAbs(1.50296496872613, DELTA_PHASE));
                CHECK_THAT(std::arg(values(7, 7)), WithinAbs(1.55375648968761, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 0)), WithinAbs(1.55375648968761, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 1)), WithinAbs(1.50296496872594, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 2)), WithinAbs(-1.56778243604873, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 3)), WithinAbs(-1.57833127447549, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 4)), WithinAbs(-1.57833127447549, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 5)), WithinAbs(-1.56778243604873, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 6)), WithinAbs(1.50296496872613, DELTA_PHASE));
                CHECK_THAT(std::arg(values(8, 7)), WithinAbs(1.55375648968761, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 0)), WithinAbs(1.55375648968717, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 1)), WithinAbs(1.50296496872661, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 2)), WithinAbs(-1.56778243604869, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 3)), WithinAbs(-1.57833127447557, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 4)), WithinAbs(-1.57833127447548, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 5)), WithinAbs(-1.56778243604869, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 6)), WithinAbs(1.50296496872633, DELTA_PHASE));
                CHECK_THAT(std::arg(values(9, 7)), WithinAbs(1.55375648968717, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 0)), WithinAbs(1.5537564896872, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 1)), WithinAbs(1.50296496872672, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 2)), WithinAbs(-1.5677824360485, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 3)), WithinAbs(-1.57833127447573, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 4)), WithinAbs(-1.57833127447573, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 5)), WithinAbs(-1.5677824360485, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 6)), WithinAbs(1.50296496872668, DELTA_PHASE));
                CHECK_THAT(std::arg(values(10, 7)), WithinAbs(1.5537564896872, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 0)), WithinAbs(1.55375648968751, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 1)), WithinAbs(1.5029649687272, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 2)), WithinAbs(-1.56778243604821, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 3)), WithinAbs(-1.57833127447522, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 4)), WithinAbs(-1.57833127447522, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 5)), WithinAbs(-1.56778243604821, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 6)), WithinAbs(1.50296496872635, DELTA_PHASE));
                CHECK_THAT(std::arg(values(11, 7)), WithinAbs(1.55375648968751, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 0)), WithinAbs(1.5537564896873, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 1)), WithinAbs(1.50296496872593, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 2)), WithinAbs(-1.56778243604858, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 3)), WithinAbs(-1.57833127447572, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 4)), WithinAbs(-1.57833127447572, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 5)), WithinAbs(-1.56778243604858, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 6)), WithinAbs(1.50296496872593, DELTA_PHASE));
                CHECK_THAT(std::arg(values(12, 7)), WithinAbs(1.5537564896873, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 0)), WithinAbs(1.55375648968793, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 1)), WithinAbs(1.50296496872634, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 2)), WithinAbs(-1.56778243604858, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 3)), WithinAbs(-1.5783312744754, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 4)), WithinAbs(-1.5783312744754, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 5)), WithinAbs(-1.56778243604858, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 6)), WithinAbs(1.50296496872634, DELTA_PHASE));
                CHECK_THAT(std::arg(values(13, 7)), WithinAbs(1.55375648968793, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 0)), WithinAbs(1.55375648968767, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 1)), WithinAbs(1.50296496872625, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 2)), WithinAbs(-1.56778243604838, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 3)), WithinAbs(-1.57833127447559, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 4)), WithinAbs(-1.57833127447556, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 5)), WithinAbs(-1.56778243604838, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 6)), WithinAbs(1.50296496872654, DELTA_PHASE));
                CHECK_THAT(std::arg(values(14, 7)), WithinAbs(1.55375648968767, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 0)), WithinAbs(1.55375648968762, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 1)), WithinAbs(1.50296496872655, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 2)), WithinAbs(-1.56778243604873, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 3)), WithinAbs(-1.57833127447549, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 4)), WithinAbs(-1.57833127447549, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 5)), WithinAbs(-1.56778243604873, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 6)), WithinAbs(1.50296496872701, DELTA_PHASE));
                CHECK_THAT(std::arg(values(15, 7)), WithinAbs(1.55375648968762, DELTA_PHASE));
            }
        }
    }
}
