//
// Created by Tristan Krause on 2026-07-14.
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <nlohmann/json.hpp>
#include "eval/rxvoltagefield.hpp"
#include "setup/setup.hpp"
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
using geometry::Geometry;

namespace
{
    using std::ranges::max;
    using std::ranges::transform;

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
    "setup_name": "test-ula"
  },
  "num_params": {
    "system_wavelength": 0.1
  },
  "variables": {
    "distance": 100,
    "dipole_length_tx": "system_wavelength * 0.9",
    "dipole_length_rx": "system_wavelength * 1.1"
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
    auto& wavelength = su.num_params().system_wavelength;
    auto const distance = su.get_double("distance");
    auto const& tx = su.get_antenna("ula1");
    auto& rx = su.get_antenna("receiver");

    auto voltage_field = eval::RxVoltageField(tx, rx, su.num_params());
    auto& num_params = voltage_field.num_params;

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
            CHECK_THAT(positions(0, 0).z, WithinAbs(-50.0, 1e-9));
            CHECK_THAT(positions(1, 0).z, WithinAbs(-25.0, 1e-9));
            CHECK_THAT(positions(2, 0).z, WithinAbs(00.0, 1e-9));
            CHECK_THAT(positions(3, 0).z, WithinAbs(25.0, 1e-9));
            CHECK_THAT(positions(4, 0).z, WithinAbs(50.0, 1e-9));

            // First, find the maximum of the physical values and normalize for better comparison
            double const abs_max = normalize(values);
            CHECK_THAT(abs_max, WithinAbs(0.00816237224653571, 1e-9));

            // Verify physical values along the curve
            CHECK_THAT(std::abs(values(0, 0)), WithinAbs(0.0337295527213379, 1e-9));
            CHECK_THAT(std::abs(values(1, 0)), WithinAbs(0.0238992910918213, 1e-9));
            CHECK_THAT(std::abs(values(2, 0)), WithinAbs(1, 1e-9));
            CHECK_THAT(std::abs(values(3, 0)), WithinAbs(0.0238992910918213, 1e-9));
            CHECK_THAT(std::abs(values(4, 0)), WithinAbs(0.0337295527213378, 1e-9));
            CHECK_THAT(std::arg(values(0, 0)), WithinAbs(1.32786841400004, 1e-9));
            CHECK_THAT(std::arg(values(1, 0)), WithinAbs(3.05394203080116, 1e-9));
            CHECK_THAT(std::arg(values(2, 0)), WithinAbs(-1.58748534552425, 1e-9));
            CHECK_THAT(std::arg(values(3, 0)), WithinAbs(3.05394203080116, 1e-9));
            CHECK_THAT(std::arg(values(4, 0)), WithinAbs(1.32786841400004, 1e-9));
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
            CHECK_THAT(positions(0, 0).z, WithinAbs(-50.0, 1e-9));
            CHECK_THAT(positions(1, 0).z, WithinAbs(-25.0, 1e-9));
            CHECK_THAT(positions(2, 0).z, WithinAbs(00.0, 1e-9));
            CHECK_THAT(positions(3, 0).z, WithinAbs(25.0, 1e-9));
            CHECK_THAT(positions(4, 0).z, WithinAbs(50.0, 1e-9));

            // Verify page 0 (note: this page should be equal to the eval_geometry test)
            {
                auto& values = data[0];

                // First, find the maximum of the physical values and normalize for better comparison
                double const abs_max = normalize(values);
                CHECK_THAT(abs_max, WithinAbs(0.00816237224653571, 1e-9));

                // Verify physical values along the curve
                CHECK_THAT(std::abs(values(0, 0)), WithinAbs(0.0337295527213379, 1e-9));
                CHECK_THAT(std::abs(values(1, 0)), WithinAbs(0.0238992910918213, 1e-9));
                CHECK_THAT(std::abs(values(2, 0)), WithinAbs(1, 1e-9));
                CHECK_THAT(std::abs(values(3, 0)), WithinAbs(0.0238992910918213, 1e-9));
                CHECK_THAT(std::abs(values(4, 0)), WithinAbs(0.0337295527213378, 1e-9));
                CHECK_THAT(std::arg(values(0, 0)), WithinAbs(1.32786841400004, 1e-9));
                CHECK_THAT(std::arg(values(1, 0)), WithinAbs(3.05394203080116, 1e-9));
                CHECK_THAT(std::arg(values(2, 0)), WithinAbs(-1.58748534552425, 1e-9));
                CHECK_THAT(std::arg(values(3, 0)), WithinAbs(3.05394203080116, 1e-9));
                CHECK_THAT(std::arg(values(4, 0)), WithinAbs(1.32786841400004, 1e-9));
            }

            // Verify page 1
            {
                auto& values = data[1];

                // First, find the maximum of the physical values and normalize for better comparison
                double const abs_max = normalize(values);
                CHECK_THAT(abs_max, WithinAbs(0.02659651050178366, 1e-9));

                // Verify physical values along the curve
                CHECK_THAT(std::abs(values(0, 0)), WithinAbs(0.0764583097513073, 1e-9));
                CHECK_THAT(std::abs(values(1, 0)), WithinAbs(0.171220984264941, 1e-9));
                CHECK_THAT(std::abs(values(2, 0)), WithinAbs(1, 1e-9));
                CHECK_THAT(std::abs(values(3, 0)), WithinAbs(0.171220984264941, 1e-9));
                CHECK_THAT(std::abs(values(4, 0)), WithinAbs(0.0764583097513073, 1e-9));
                CHECK_THAT(std::arg(values(0, 0)), WithinAbs(2.44837837459382, 1e-9));
                CHECK_THAT(std::arg(values(1, 0)), WithinAbs(0.372559076982654, 1e-9));
                CHECK_THAT(std::arg(values(2, 0)), WithinAbs(0.512472540046752, 1e-9));
                CHECK_THAT(std::arg(values(3, 0)), WithinAbs(0.372559076982654, 1e-9));
                CHECK_THAT(std::arg(values(4, 0)), WithinAbs(2.44837837459382, 1e-9));
            }
            //
            // Verify page 2
            {
                auto& values = data[2];

                // First, find the maximum of the physical values and normalize for better comparison
                double const abs_max = normalize(values);
                CHECK_THAT(abs_max, WithinAbs(0.04531595494707993, 1e-9));

                // Verify physical values along the curve
                CHECK_THAT(std::abs(values(0, 0)), WithinAbs(0.0727453682609543, 1e-9));
                CHECK_THAT(std::abs(values(1, 0)), WithinAbs(0.0279442337747555, 1e-9));
                CHECK_THAT(std::abs(values(2, 0)), WithinAbs(1, 1e-9));
                CHECK_THAT(std::abs(values(3, 0)), WithinAbs(0.0279442337747555, 1e-9));
                CHECK_THAT(std::abs(values(4, 0)), WithinAbs(0.0727453682609543, 1e-9));
                CHECK_THAT(std::arg(values(0, 0)), WithinAbs(1.46374975740342, 1e-9));
                CHECK_THAT(std::arg(values(1, 0)), WithinAbs(2.45126705961933, 1e-9));
                CHECK_THAT(std::arg(values(2, 0)), WithinAbs(-1.57914105201936, 1e-9));
                CHECK_THAT(std::arg(values(3, 0)), WithinAbs(2.45126705961934, 1e-9));
                CHECK_THAT(std::arg(values(4, 0)), WithinAbs(1.46374975740342, 1e-9));
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
            CHECK_THAT(positions(0, 0).z, WithinAbs(-70.71067811865474084, 1e-9)); // t = 0.0 -> -pi/2
            CHECK_THAT(positions(1, 0).z, WithinAbs(-38.26834323650897574, 1e-9)); // t = 0.25
            CHECK_THAT(positions(2, 0).z, WithinAbs(0.0, 1e-9)); // t = 0.5 -> 0 rad
            CHECK_THAT(positions(3, 0).z, WithinAbs(38.26834323650897574, 1e-9)); // t = 0.75
            CHECK_THAT(positions(4, 0).z, WithinAbs(70.71067811865474084, 1e-9)); // t = 1.0 -> +pi/2

            // First, find the maximum of the physical values and normalize for better comparison
            double const abs_max = normalize(values);
            CHECK_THAT(abs_max, WithinAbs(0.00816237224653571, 1e-9));

            // Verify physical values along the curve
            CHECK_THAT(std::abs(values(0, 0)), WithinAbs(0.00320028288208625, 1e-9));
            CHECK_THAT(std::arg(values(0, 0)), WithinAbs(1.56412519106567638, 1e-9));
            CHECK_THAT(std::abs(values(1, 0)), WithinAbs(0.01116871931989499, 1e-9));
            CHECK_THAT(std::arg(values(1, 0)), WithinAbs(1.4187912238163809, 1e-9));
            CHECK_THAT(std::abs(values(2, 0)), WithinAbs(1.0, 1e-9));
            CHECK_THAT(std::arg(values(2, 0)), WithinAbs(-1.58748534552424547, 1e-9));
            CHECK_THAT(std::abs(values(3, 0)), WithinAbs(0.01116871931989495, 1e-9));
            CHECK_THAT(std::arg(values(3, 0)), WithinAbs(1.41879122381637823, 1e-9));
            CHECK_THAT(std::abs(values(4, 0)), WithinAbs(0.00320028288208626, 1e-9));
            CHECK_THAT(std::arg(values(4, 0)), WithinAbs(1.56412519106567482, 1e-9));
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
            CHECK_THAT(positions(0, 0).z, WithinAbs(-70.71067811865474084, 1e-9)); // t = 0.0 -> -pi/2
            CHECK_THAT(positions(1, 0).z, WithinAbs(-38.26834323650897574, 1e-9)); // t = 0.25
            CHECK_THAT(positions(2, 0).z, WithinAbs(0.0, 1e-9)); // t = 0.5 -> 0 rad
            CHECK_THAT(positions(3, 0).z, WithinAbs(38.26834323650897574, 1e-9)); // t = 0.75
            CHECK_THAT(positions(4, 0).z, WithinAbs(70.71067811865474084, 1e-9)); // t = 1.0 -> +pi/2

            // Verify page 0 (note: this page should be equal to the eval_geometry test)
            {
                auto& values = data[0];

                // First, find the maximum of the physical values and normalize for better comparison
                double const abs_max = normalize(values);
                CHECK_THAT(abs_max, WithinAbs(0.00816237224653571, 1e-9));

                // Verify physical values along the curve
                CHECK_THAT(std::abs(values(0, 0)), WithinAbs(0.00320028288208625, 1e-9));
                CHECK_THAT(std::arg(values(0, 0)), WithinAbs(1.56412519106567638, 1e-9));
                CHECK_THAT(std::abs(values(1, 0)), WithinAbs(0.01116871931989499, 1e-9));
                CHECK_THAT(std::arg(values(1, 0)), WithinAbs(1.4187912238163809, 1e-9));
                CHECK_THAT(std::abs(values(2, 0)), WithinAbs(1.0, 1e-9));
                CHECK_THAT(std::arg(values(2, 0)), WithinAbs(-1.58748534552424547, 1e-9));
                CHECK_THAT(std::abs(values(3, 0)), WithinAbs(0.01116871931989495, 1e-9));
                CHECK_THAT(std::arg(values(3, 0)), WithinAbs(1.41879122381637823, 1e-9));
                CHECK_THAT(std::abs(values(4, 0)), WithinAbs(0.00320028288208626, 1e-9));
                CHECK_THAT(std::arg(values(4, 0)), WithinAbs(1.56412519106567482, 1e-9));
            }

            // Verify page 1
            {
                auto& values = data[1];

                // First, find the maximum of the physical values and normalize for better comparison
                double const abs_max = normalize(values);
                CHECK_THAT(abs_max, WithinAbs(0.02659651050178366, 1e-9));

                // Verify physical values along the curve
                CHECK_THAT(std::abs(values(0, 0)), WithinAbs(0.01858547187923242, 1e-9));
                CHECK_THAT(std::arg(values(0, 0)), WithinAbs(-2.61806589941934931, 1e-9));
                CHECK_THAT(std::abs(values(1, 0)), WithinAbs(0.01536786526331465, 1e-9));
                CHECK_THAT(std::arg(values(1, 0)), WithinAbs(0.37770760072447473, 1e-9));
                CHECK_THAT(std::abs(values(2, 0)), WithinAbs(1.0, 1e-9));
                CHECK_THAT(std::arg(values(2, 0)), WithinAbs(0.51247254004675225, 1e-9));
                CHECK_THAT(std::abs(values(3, 0)), WithinAbs(0.01536786526331473, 1e-9));
                CHECK_THAT(std::arg(values(3, 0)), WithinAbs(0.37770760072447612, 1e-9));
                CHECK_THAT(std::abs(values(4, 0)), WithinAbs(0.01858547187923243, 1e-9));
                CHECK_THAT(std::arg(values(4, 0)), WithinAbs(-2.61806589941934886, 1e-9));
            }

            // Verify page 2
            {
                auto& values = data[2];

                // First, find the maximum of the physical values and normalize for better comparison
                double const abs_max = normalize(values);
                CHECK_THAT(abs_max, WithinAbs(0.04531595494707993, 1e-9));

                // Verify physical values along the curve
                CHECK_THAT(std::abs(values(0, 0)), WithinAbs(0.02389331153927321, 1e-9));
                CHECK_THAT(std::arg(values(0, 0)), WithinAbs(-1.56131841583061837, 1e-9));
                CHECK_THAT(std::abs(values(1, 0)), WithinAbs(0.16726023674013196, 1e-9));
                CHECK_THAT(std::arg(values(1, 0)), WithinAbs(1.55375648968733615, 1e-9));
                CHECK_THAT(std::abs(values(2, 0)), WithinAbs(1.0, 1e-9));
                CHECK_THAT(std::arg(values(2, 0)), WithinAbs(-1.5791410520193585, 1e-9));
                CHECK_THAT(std::abs(values(3, 0)), WithinAbs(0.16726023674013196, 1e-9));
                CHECK_THAT(std::arg(values(3, 0)), WithinAbs(1.55375648968733637, 1e-9));
                CHECK_THAT(std::abs(values(4, 0)), WithinAbs(0.02389331153927323, 1e-9));
                CHECK_THAT(std::arg(values(4, 0)), WithinAbs(-1.56131841583061881, 1e-9));
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
            CHECK_THAT(positions(0, 0).x, WithinAbs(100, 1e-9));
            CHECK_THAT(positions(0, 0).y, WithinAbs(100, 1e-9));
            CHECK_THAT(positions(0, 0).z, WithinAbs(-50, 1e-9));
            CHECK_THAT(positions(positions.shape().rows - 1, 0).x, WithinAbs(-100, 1e-9));
            CHECK_THAT(positions(positions.shape().rows - 1, 0).y, WithinAbs(100, 1e-9));
            CHECK_THAT(positions(positions.shape().rows - 1, 0).z, WithinAbs(-50, 1e-9));
            CHECK_THAT(positions(0, positions.shape().cols - 1).x, WithinAbs(100, 1e-9));
            CHECK_THAT(positions(0, positions.shape().cols - 1).y, WithinAbs(100, 1e-9));
            CHECK_THAT(positions(0, positions.shape().cols - 1).z, WithinAbs(50, 1e-9));
            CHECK_THAT(positions(positions.shape().rows - 1, positions.shape().cols - 1).x, WithinAbs(-100, 1e-9));
            CHECK_THAT(positions(positions.shape().rows - 1, positions.shape().cols - 1).y, WithinAbs(100, 1e-9));
            CHECK_THAT(positions(positions.shape().rows - 1, positions.shape().cols - 1).z, WithinAbs(50, 1e-9));

            // Next, find the maximum of the physical values and normalize for better comparison
            double const abs_max = normalize(values);
            CHECK_THAT(abs_max, WithinAbs(0.00448208702149741, 1e-9));

            // Verify physical values along the surface
            CHECK_THAT(std::abs(values(0, 0)), WithinAbs(0.0811805333041281, 1e-9));
            CHECK_THAT(std::abs(values(0, 1)), WithinAbs(0.0209935151691007, 1e-9));
            CHECK_THAT(std::abs(values(0, 2)), WithinAbs(0.181776966584661, 1e-9));
            CHECK_THAT(std::abs(values(0, 3)), WithinAbs(0.959227993440513, 1e-9));
            CHECK_THAT(std::abs(values(0, 4)), WithinAbs(0.959227993440513, 1e-9));
            CHECK_THAT(std::abs(values(0, 5)), WithinAbs(0.181776966584661, 1e-9));
            CHECK_THAT(std::abs(values(0, 6)), WithinAbs(0.0209935151689359, 1e-9));
            CHECK_THAT(std::abs(values(0, 7)), WithinAbs(0.0811805333041281, 1e-9));
            CHECK_THAT(std::abs(values(1, 0)), WithinAbs(0.0455986942249276, 1e-9));
            CHECK_THAT(std::abs(values(1, 1)), WithinAbs(0.0411457551690019, 1e-9));
            CHECK_THAT(std::abs(values(1, 2)), WithinAbs(0.236208994298118, 1e-9));
            CHECK_THAT(std::abs(values(1, 3)), WithinAbs(0.980316457457755, 1e-9));
            CHECK_THAT(std::abs(values(1, 4)), WithinAbs(0.980316457457755, 1e-9));
            CHECK_THAT(std::abs(values(1, 5)), WithinAbs(0.236208994298118, 1e-9));
            CHECK_THAT(std::abs(values(1, 6)), WithinAbs(0.0411457551690372, 1e-9));
            CHECK_THAT(std::abs(values(1, 7)), WithinAbs(0.0455986942249277, 1e-9));
            CHECK_THAT(std::abs(values(2, 0)), WithinAbs(0.00257584589170965, 1e-9));
            CHECK_THAT(std::abs(values(2, 1)), WithinAbs(0.0941592075283062, 1e-9));
            CHECK_THAT(std::abs(values(2, 2)), WithinAbs(0.275478907113585, 1e-9));
            CHECK_THAT(std::abs(values(2, 3)), WithinAbs(0.99429542171149, 1e-9));
            CHECK_THAT(std::abs(values(2, 4)), WithinAbs(0.99429542171149, 1e-9));
            CHECK_THAT(std::abs(values(2, 5)), WithinAbs(0.275478907113585, 1e-9));
            CHECK_THAT(std::abs(values(2, 6)), WithinAbs(0.0941592075283062, 1e-9));
            CHECK_THAT(std::abs(values(2, 7)), WithinAbs(0.00257584589170968, 1e-9));
            CHECK_THAT(std::abs(values(3, 0)), WithinAbs(0.0357596041928141, 1e-9));
            CHECK_THAT(std::abs(values(3, 1)), WithinAbs(0.128220787975769, 1e-9));
            CHECK_THAT(std::abs(values(3, 2)), WithinAbs(0.294897071700718, 1e-9));
            CHECK_THAT(std::abs(values(3, 3)), WithinAbs(1, 1e-9));
            CHECK_THAT(std::abs(values(3, 4)), WithinAbs(1, 1e-9));
            CHECK_THAT(std::abs(values(3, 5)), WithinAbs(0.294897071700718, 1e-9));
            CHECK_THAT(std::abs(values(3, 6)), WithinAbs(0.128220787975793, 1e-9));
            CHECK_THAT(std::abs(values(3, 7)), WithinAbs(0.0357596041928141, 1e-9));
            CHECK_THAT(std::abs(values(4, 0)), WithinAbs(0.0589060465262149, 1e-9));
            CHECK_THAT(std::abs(values(4, 1)), WithinAbs(0.139456471473219, 1e-9));
            CHECK_THAT(std::abs(values(4, 2)), WithinAbs(0.294128532445929, 1e-9));
            CHECK_THAT(std::abs(values(4, 3)), WithinAbs(0.997606657015968, 1e-9));
            CHECK_THAT(std::abs(values(4, 4)), WithinAbs(0.997606657015968, 1e-9));
            CHECK_THAT(std::abs(values(4, 5)), WithinAbs(0.294128532445929, 1e-9));
            CHECK_THAT(std::abs(values(4, 6)), WithinAbs(0.139456471473219, 1e-9));
            CHECK_THAT(std::abs(values(4, 7)), WithinAbs(0.0589060465262149, 1e-9));
            CHECK_THAT(std::abs(values(5, 0)), WithinAbs(0.0665616877563288, 1e-9));
            CHECK_THAT(std::abs(values(5, 1)), WithinAbs(0.132742825255253, 1e-9));
            CHECK_THAT(std::abs(values(5, 2)), WithinAbs(0.27873146020983, 1e-9));
            CHECK_THAT(std::abs(values(5, 3)), WithinAbs(0.989316385385742, 1e-9));
            CHECK_THAT(std::abs(values(5, 4)), WithinAbs(0.989316385385742, 1e-9));
            CHECK_THAT(std::abs(values(5, 5)), WithinAbs(0.27873146020983, 1e-9));
            CHECK_THAT(std::abs(values(5, 6)), WithinAbs(0.13274282525528, 1e-9));
            CHECK_THAT(std::abs(values(5, 7)), WithinAbs(0.0665616877563289, 1e-9));
            CHECK_THAT(std::abs(values(6, 0)), WithinAbs(0.0650061963206579, 1e-9));
            CHECK_THAT(std::abs(values(6, 1)), WithinAbs(0.118932954431791, 1e-9));
            CHECK_THAT(std::abs(values(6, 2)), WithinAbs(0.258936075347669, 1e-9));
            CHECK_THAT(std::abs(values(6, 3)), WithinAbs(0.979344504415583, 1e-9));
            CHECK_THAT(std::abs(values(6, 4)), WithinAbs(0.979344504415583, 1e-9));
            CHECK_THAT(std::abs(values(6, 5)), WithinAbs(0.258936075347669, 1e-9));
            CHECK_THAT(std::abs(values(6, 6)), WithinAbs(0.118932954431791, 1e-9));
            CHECK_THAT(std::abs(values(6, 7)), WithinAbs(0.0650061963206579, 1e-9));
            CHECK_THAT(std::abs(values(7, 0)), WithinAbs(0.0619228784388384, 1e-9));
            CHECK_THAT(std::abs(values(7, 1)), WithinAbs(0.108899738482795, 1e-9));
            CHECK_THAT(std::abs(values(7, 2)), WithinAbs(0.245612300311409, 1e-9));
            CHECK_THAT(std::abs(values(7, 3)), WithinAbs(0.972648506328136, 1e-9));
            CHECK_THAT(std::abs(values(7, 4)), WithinAbs(0.972648506328136, 1e-9));
            CHECK_THAT(std::abs(values(7, 5)), WithinAbs(0.245612300311409, 1e-9));
            CHECK_THAT(std::abs(values(7, 6)), WithinAbs(0.108899738482732, 1e-9));
            CHECK_THAT(std::abs(values(7, 7)), WithinAbs(0.0619228784388384, 1e-9));
            CHECK_THAT(std::abs(values(8, 0)), WithinAbs(0.0619228784388384, 1e-9));
            CHECK_THAT(std::abs(values(8, 1)), WithinAbs(0.108899738482794, 1e-9));
            CHECK_THAT(std::abs(values(8, 2)), WithinAbs(0.245612300311409, 1e-9));
            CHECK_THAT(std::abs(values(8, 3)), WithinAbs(0.972648506328136, 1e-9));
            CHECK_THAT(std::abs(values(8, 4)), WithinAbs(0.972648506328136, 1e-9));
            CHECK_THAT(std::abs(values(8, 5)), WithinAbs(0.245612300311409, 1e-9));
            CHECK_THAT(std::abs(values(8, 6)), WithinAbs(0.108899738482732, 1e-9));
            CHECK_THAT(std::abs(values(8, 7)), WithinAbs(0.0619228784388384, 1e-9));
            CHECK_THAT(std::abs(values(9, 0)), WithinAbs(0.0650061963206579, 1e-9));
            CHECK_THAT(std::abs(values(9, 1)), WithinAbs(0.118932954431791, 1e-9));
            CHECK_THAT(std::abs(values(9, 2)), WithinAbs(0.258936075347669, 1e-9));
            CHECK_THAT(std::abs(values(9, 3)), WithinAbs(0.979344504415583, 1e-9));
            CHECK_THAT(std::abs(values(9, 4)), WithinAbs(0.979344504415583, 1e-9));
            CHECK_THAT(std::abs(values(9, 5)), WithinAbs(0.258936075347669, 1e-9));
            CHECK_THAT(std::abs(values(9, 6)), WithinAbs(0.118932954431791, 1e-9));
            CHECK_THAT(std::abs(values(9, 7)), WithinAbs(0.0650061963206579, 1e-9));
            CHECK_THAT(std::abs(values(10, 0)), WithinAbs(0.0665616877563289, 1e-9));
            CHECK_THAT(std::abs(values(10, 1)), WithinAbs(0.132742825255253, 1e-9));
            CHECK_THAT(std::abs(values(10, 2)), WithinAbs(0.27873146020983, 1e-9));
            CHECK_THAT(std::abs(values(10, 3)), WithinAbs(0.989316385385742, 1e-9));
            CHECK_THAT(std::abs(values(10, 4)), WithinAbs(0.989316385385742, 1e-9));
            CHECK_THAT(std::abs(values(10, 5)), WithinAbs(0.27873146020983, 1e-9));
            CHECK_THAT(std::abs(values(10, 6)), WithinAbs(0.13274282525528, 1e-9));
            CHECK_THAT(std::abs(values(10, 7)), WithinAbs(0.0665616877563289, 1e-9));
            CHECK_THAT(std::abs(values(11, 0)), WithinAbs(0.0589060465262149, 1e-9));
            CHECK_THAT(std::abs(values(11, 1)), WithinAbs(0.139456471473219, 1e-9));
            CHECK_THAT(std::abs(values(11, 2)), WithinAbs(0.294128532445929, 1e-9));
            CHECK_THAT(std::abs(values(11, 3)), WithinAbs(0.997606657015968, 1e-9));
            CHECK_THAT(std::abs(values(11, 4)), WithinAbs(0.997606657015968, 1e-9));
            CHECK_THAT(std::abs(values(11, 5)), WithinAbs(0.294128532445929, 1e-9));
            CHECK_THAT(std::abs(values(11, 6)), WithinAbs(0.139456471473219, 1e-9));
            CHECK_THAT(std::abs(values(11, 7)), WithinAbs(0.058906046526215, 1e-9));
            CHECK_THAT(std::abs(values(12, 0)), WithinAbs(0.0357596041928872, 1e-9));
            CHECK_THAT(std::abs(values(12, 1)), WithinAbs(0.128220787975745, 1e-9));
            CHECK_THAT(std::abs(values(12, 2)), WithinAbs(0.294897071701048, 1e-9));
            CHECK_THAT(std::abs(values(12, 3)), WithinAbs(0.999999999999537, 1e-9));
            CHECK_THAT(std::abs(values(12, 4)), WithinAbs(0.999999999999537, 1e-9));
            CHECK_THAT(std::abs(values(12, 5)), WithinAbs(0.294897071701048, 1e-9));
            CHECK_THAT(std::abs(values(12, 6)), WithinAbs(0.12822078797574, 1e-9));
            CHECK_THAT(std::abs(values(12, 7)), WithinAbs(0.0357596041928873, 1e-9));
            CHECK_THAT(std::abs(values(13, 0)), WithinAbs(0.00257584589170966, 1e-9));
            CHECK_THAT(std::abs(values(13, 1)), WithinAbs(0.0941592075283062, 1e-9));
            CHECK_THAT(std::abs(values(13, 2)), WithinAbs(0.275478907113585, 1e-9));
            CHECK_THAT(std::abs(values(13, 3)), WithinAbs(0.99429542171149, 1e-9));
            CHECK_THAT(std::abs(values(13, 4)), WithinAbs(0.99429542171149, 1e-9));
            CHECK_THAT(std::abs(values(13, 5)), WithinAbs(0.275478907113585, 1e-9));
            CHECK_THAT(std::abs(values(13, 6)), WithinAbs(0.0941592075283062, 1e-9));
            CHECK_THAT(std::abs(values(13, 7)), WithinAbs(0.00257584589170969, 1e-9));
            CHECK_THAT(std::abs(values(14, 0)), WithinAbs(0.0455986942249276, 1e-9));
            CHECK_THAT(std::abs(values(14, 1)), WithinAbs(0.0411457551690019, 1e-9));
            CHECK_THAT(std::abs(values(14, 2)), WithinAbs(0.236208994298118, 1e-9));
            CHECK_THAT(std::abs(values(14, 3)), WithinAbs(0.980316457457755, 1e-9));
            CHECK_THAT(std::abs(values(14, 4)), WithinAbs(0.980316457457755, 1e-9));
            CHECK_THAT(std::abs(values(14, 5)), WithinAbs(0.236208994298118, 1e-9));
            CHECK_THAT(std::abs(values(14, 6)), WithinAbs(0.0411457551690372, 1e-9));
            CHECK_THAT(std::abs(values(14, 7)), WithinAbs(0.0455986942249276, 1e-9));
            CHECK_THAT(std::abs(values(15, 0)), WithinAbs(0.0811805333041281, 1e-9));
            CHECK_THAT(std::abs(values(15, 1)), WithinAbs(0.0209935151691007, 1e-9));
            CHECK_THAT(std::abs(values(15, 2)), WithinAbs(0.181776966584661, 1e-9));
            CHECK_THAT(std::abs(values(15, 3)), WithinAbs(0.959227993440513, 1e-9));
            CHECK_THAT(std::abs(values(15, 4)), WithinAbs(0.959227993440513, 1e-9));
            CHECK_THAT(std::abs(values(15, 5)), WithinAbs(0.181776966584661, 1e-9));
            CHECK_THAT(std::abs(values(15, 6)), WithinAbs(0.0209935151689359, 1e-9));
            CHECK_THAT(std::abs(values(15, 7)), WithinAbs(0.0811805333041281, 1e-9));
            CHECK_THAT(std::arg(values(0, 0)), WithinAbs(-1.59046134379987, 1e-6));
            CHECK_THAT(std::arg(values(0, 1)), WithinAbs(-2.18508268543536, 1e-6));
            CHECK_THAT(std::arg(values(0, 2)), WithinAbs(-0.724963766234791, 1e-6));
            CHECK_THAT(std::arg(values(0, 3)), WithinAbs(-1.68114156649414, 1e-6));
            CHECK_THAT(std::arg(values(0, 4)), WithinAbs(-1.68114156649414, 1e-6));
            CHECK_THAT(std::arg(values(0, 5)), WithinAbs(-0.724963766234791, 1e-6));
            CHECK_THAT(std::arg(values(0, 6)), WithinAbs(-2.18508268543734, 1e-6));
            CHECK_THAT(std::arg(values(0, 7)), WithinAbs(-1.59046134379987, 1e-6));
            CHECK_THAT(std::arg(values(1, 0)), WithinAbs(0.8988282123476, 1e-6));
            CHECK_THAT(std::arg(values(1, 1)), WithinAbs(0.57912013854105, 1e-6));
            CHECK_THAT(std::arg(values(1, 2)), WithinAbs(-1.82938225706847, 1e-6));
            CHECK_THAT(std::arg(values(1, 3)), WithinAbs(-2.9733018895961, 1e-6));
            CHECK_THAT(std::arg(values(1, 4)), WithinAbs(-2.9733018895961, 1e-6));
            CHECK_THAT(std::arg(values(1, 5)), WithinAbs(-1.82938225706847, 1e-6));
            CHECK_THAT(std::arg(values(1, 6)), WithinAbs(0.579120138543666, 1e-6));
            CHECK_THAT(std::arg(values(1, 7)), WithinAbs(0.898828212347601, 1e-6));
            CHECK_THAT(std::arg(values(2, 0)), WithinAbs(-1.53877106842563, 1e-6));
            CHECK_THAT(std::arg(values(2, 1)), WithinAbs(1.66489040853495, 1e-6));
            CHECK_THAT(std::arg(values(2, 2)), WithinAbs(-1.2999218759808, 1e-6));
            CHECK_THAT(std::arg(values(2, 3)), WithinAbs(-2.37743568424626, 1e-6));
            CHECK_THAT(std::arg(values(2, 4)), WithinAbs(-2.37743568424626, 1e-6));
            CHECK_THAT(std::arg(values(2, 5)), WithinAbs(-1.2999218759808, 1e-6));
            CHECK_THAT(std::arg(values(2, 6)), WithinAbs(1.66489040853494, 1e-6));
            CHECK_THAT(std::arg(values(2, 7)), WithinAbs(-1.53877106842561, 1e-6));
            CHECK_THAT(std::arg(values(3, 0)), WithinAbs(2.39951755383822, 1e-6));
            CHECK_THAT(std::arg(values(3, 1)), WithinAbs(0.570607536876258, 1e-6));
            CHECK_THAT(std::arg(values(3, 2)), WithinAbs(-2.9570550451856, 1e-6));
            CHECK_THAT(std::arg(values(3, 3)), WithinAbs(2.34375554995525, 1e-6));
            CHECK_THAT(std::arg(values(3, 4)), WithinAbs(2.34375554995525, 1e-6));
            CHECK_THAT(std::arg(values(3, 5)), WithinAbs(-2.9570550451856, 1e-6));
            CHECK_THAT(std::arg(values(3, 6)), WithinAbs(0.570607536876678, 1e-6));
            CHECK_THAT(std::arg(values(3, 7)), WithinAbs(2.39951755383822, 1e-6));
            CHECK_THAT(std::arg(values(4, 0)), WithinAbs(-1.73470220406938, 1e-6));
            CHECK_THAT(std::arg(values(4, 1)), WithinAbs(-0.875285092401346, 1e-6));
            CHECK_THAT(std::arg(values(4, 2)), WithinAbs(0.639784789812871, 1e-6));
            CHECK_THAT(std::arg(values(4, 3)), WithinAbs(-0.565601279784796, 1e-6));
            CHECK_THAT(std::arg(values(4, 4)), WithinAbs(-0.565601279784796, 1e-6));
            CHECK_THAT(std::arg(values(4, 5)), WithinAbs(0.639784789812871, 1e-6));
            CHECK_THAT(std::arg(values(4, 6)), WithinAbs(-0.875285092401345, 1e-6));
            CHECK_THAT(std::arg(values(4, 7)), WithinAbs(-1.73470220406938, 1e-6));
            CHECK_THAT(std::arg(values(5, 0)), WithinAbs(-2.65587357462535, 1e-6));
            CHECK_THAT(std::arg(values(5, 1)), WithinAbs(-1.30302497013105, 1e-6));
            CHECK_THAT(std::arg(values(5, 2)), WithinAbs(-2.56513148566834, 1e-6));
            CHECK_THAT(std::arg(values(5, 3)), WithinAbs(1.50288452264914, 1e-6));
            CHECK_THAT(std::arg(values(5, 4)), WithinAbs(1.50288452264914, 1e-6));
            CHECK_THAT(std::arg(values(5, 5)), WithinAbs(-2.56513148566834, 1e-6));
            CHECK_THAT(std::arg(values(5, 6)), WithinAbs(-1.30302497013065, 1e-6));
            CHECK_THAT(std::arg(values(5, 7)), WithinAbs(-2.65587357462535, 1e-6));
            CHECK_THAT(std::arg(values(6, 0)), WithinAbs(2.90989473171143, 1e-6));
            CHECK_THAT(std::arg(values(6, 1)), WithinAbs(1.33584297314295, 1e-6));
            CHECK_THAT(std::arg(values(6, 2)), WithinAbs(1.07597558528228, 1e-6));
            CHECK_THAT(std::arg(values(6, 3)), WithinAbs(2.80787643375169, 1e-6));
            CHECK_THAT(std::arg(values(6, 4)), WithinAbs(2.80787643375169, 1e-6));
            CHECK_THAT(std::arg(values(6, 5)), WithinAbs(1.07597558528228, 1e-6));
            CHECK_THAT(std::arg(values(6, 6)), WithinAbs(1.33584297314295, 1e-6));
            CHECK_THAT(std::arg(values(6, 7)), WithinAbs(2.90989473171143, 1e-6));
            CHECK_THAT(std::arg(values(7, 0)), WithinAbs(1.41636191652562, 1e-6));
            CHECK_THAT(std::arg(values(7, 1)), WithinAbs(-1.29995402322705, 1e-6));
            CHECK_THAT(std::arg(values(7, 2)), WithinAbs(2.34715161578265, 1e-6));
            CHECK_THAT(std::arg(values(7, 3)), WithinAbs(-0.0792612673224425, 1e-6));
            CHECK_THAT(std::arg(values(7, 4)), WithinAbs(-0.0792612673224425, 1e-6));
            CHECK_THAT(std::arg(values(7, 5)), WithinAbs(2.34715161578265, 1e-6));
            CHECK_THAT(std::arg(values(7, 6)), WithinAbs(-1.29995402322705, 1e-6));
            CHECK_THAT(std::arg(values(7, 7)), WithinAbs(1.41636191652563, 1e-6));
            CHECK_THAT(std::arg(values(8, 0)), WithinAbs(1.41636191652562, 1e-6));
            CHECK_THAT(std::arg(values(8, 1)), WithinAbs(-1.29995402322705, 1e-6));
            CHECK_THAT(std::arg(values(8, 2)), WithinAbs(2.34715161578265, 1e-6));
            CHECK_THAT(std::arg(values(8, 3)), WithinAbs(-0.0792612673224425, 1e-6));
            CHECK_THAT(std::arg(values(8, 4)), WithinAbs(-0.0792612673224425, 1e-6));
            CHECK_THAT(std::arg(values(8, 5)), WithinAbs(2.34715161578265, 1e-6));
            CHECK_THAT(std::arg(values(8, 6)), WithinAbs(-1.29995402322705, 1e-6));
            CHECK_THAT(std::arg(values(8, 7)), WithinAbs(1.41636191652563, 1e-6));
            CHECK_THAT(std::arg(values(9, 0)), WithinAbs(2.90989473171143, 1e-6));
            CHECK_THAT(std::arg(values(9, 1)), WithinAbs(1.33584297314295, 1e-6));
            CHECK_THAT(std::arg(values(9, 2)), WithinAbs(1.07597558528228, 1e-6));
            CHECK_THAT(std::arg(values(9, 3)), WithinAbs(2.80787643375169, 1e-6));
            CHECK_THAT(std::arg(values(9, 4)), WithinAbs(2.80787643375169, 1e-6));
            CHECK_THAT(std::arg(values(9, 5)), WithinAbs(1.07597558528228, 1e-6));
            CHECK_THAT(std::arg(values(9, 6)), WithinAbs(1.33584297314295, 1e-6));
            CHECK_THAT(std::arg(values(9, 7)), WithinAbs(2.90989473171143, 1e-6));
            CHECK_THAT(std::arg(values(10, 0)), WithinAbs(-2.65587357462535, 1e-6));
            CHECK_THAT(std::arg(values(10, 1)), WithinAbs(-1.30302497013105, 1e-6));
            CHECK_THAT(std::arg(values(10, 2)), WithinAbs(-2.56513148566834, 1e-6));
            CHECK_THAT(std::arg(values(10, 3)), WithinAbs(1.50288452264914, 1e-6));
            CHECK_THAT(std::arg(values(10, 4)), WithinAbs(1.50288452264914, 1e-6));
            CHECK_THAT(std::arg(values(10, 5)), WithinAbs(-2.56513148566834, 1e-6));
            CHECK_THAT(std::arg(values(10, 6)), WithinAbs(-1.30302497013065, 1e-6));
            CHECK_THAT(std::arg(values(10, 7)), WithinAbs(-2.65587357462535, 1e-6));
            CHECK_THAT(std::arg(values(11, 0)), WithinAbs(-1.73470220406938, 1e-6));
            CHECK_THAT(std::arg(values(11, 1)), WithinAbs(-0.875285092401346, 1e-6));
            CHECK_THAT(std::arg(values(11, 2)), WithinAbs(0.639784789812871, 1e-6));
            CHECK_THAT(std::arg(values(11, 3)), WithinAbs(-0.565601279784796, 1e-6));
            CHECK_THAT(std::arg(values(11, 4)), WithinAbs(-0.565601279784796, 1e-6));
            CHECK_THAT(std::arg(values(11, 5)), WithinAbs(0.639784789812871, 1e-6));
            CHECK_THAT(std::arg(values(11, 6)), WithinAbs(-0.875285092401345, 1e-6));
            CHECK_THAT(std::arg(values(11, 7)), WithinAbs(-1.73470220406938, 1e-6));
            CHECK_THAT(std::arg(values(12, 0)), WithinAbs(2.39951755384229, 1e-6));
            CHECK_THAT(std::arg(values(12, 1)), WithinAbs(0.570607536877226, 1e-6));
            CHECK_THAT(std::arg(values(12, 2)), WithinAbs(-2.95705504518585, 1e-6));
            CHECK_THAT(std::arg(values(12, 3)), WithinAbs(2.34375554995456, 1e-6));
            CHECK_THAT(std::arg(values(12, 4)), WithinAbs(2.34375554995456, 1e-6));
            CHECK_THAT(std::arg(values(12, 5)), WithinAbs(-2.95705504518585, 1e-6));
            CHECK_THAT(std::arg(values(12, 6)), WithinAbs(0.570607536876314, 1e-6));
            CHECK_THAT(std::arg(values(12, 7)), WithinAbs(2.39951755384229, 1e-6));
            CHECK_THAT(std::arg(values(13, 0)), WithinAbs(-1.53877106842562, 1e-6));
            CHECK_THAT(std::arg(values(13, 1)), WithinAbs(1.66489040853494, 1e-6));
            CHECK_THAT(std::arg(values(13, 2)), WithinAbs(-1.2999218759808, 1e-6));
            CHECK_THAT(std::arg(values(13, 3)), WithinAbs(-2.37743568424626, 1e-6));
            CHECK_THAT(std::arg(values(13, 4)), WithinAbs(-2.37743568424626, 1e-6));
            CHECK_THAT(std::arg(values(13, 5)), WithinAbs(-1.2999218759808, 1e-6));
            CHECK_THAT(std::arg(values(13, 6)), WithinAbs(1.66489040853494, 1e-6));
            CHECK_THAT(std::arg(values(13, 7)), WithinAbs(-1.53877106842561, 1e-6));
            CHECK_THAT(std::arg(values(14, 0)), WithinAbs(0.898828212347599, 1e-6));
            CHECK_THAT(std::arg(values(14, 1)), WithinAbs(0.57912013854105, 1e-6));
            CHECK_THAT(std::arg(values(14, 2)), WithinAbs(-1.82938225706847, 1e-6));
            CHECK_THAT(std::arg(values(14, 3)), WithinAbs(-2.9733018895961, 1e-6));
            CHECK_THAT(std::arg(values(14, 4)), WithinAbs(-2.9733018895961, 1e-6));
            CHECK_THAT(std::arg(values(14, 5)), WithinAbs(-1.82938225706848, 1e-6));
            CHECK_THAT(std::arg(values(14, 6)), WithinAbs(0.579120138543665, 1e-6));
            CHECK_THAT(std::arg(values(14, 7)), WithinAbs(0.8988282123476, 1e-6));
            CHECK_THAT(std::arg(values(15, 0)), WithinAbs(-1.59046134379987, 1e-6));
            CHECK_THAT(std::arg(values(15, 1)), WithinAbs(-2.18508268543537, 1e-6));
            CHECK_THAT(std::arg(values(15, 2)), WithinAbs(-0.724963766234791, 1e-6));
            CHECK_THAT(std::arg(values(15, 3)), WithinAbs(-1.68114156649414, 1e-6));
            CHECK_THAT(std::arg(values(15, 4)), WithinAbs(-1.68114156649414, 1e-6));
            CHECK_THAT(std::arg(values(15, 5)), WithinAbs(-0.724963766234791, 1e-6));
            CHECK_THAT(std::arg(values(15, 6)), WithinAbs(-2.18508268543733, 1e-6));
            CHECK_THAT(std::arg(values(15, 7)), WithinAbs(-1.59046134379987, 1e-6));
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
            CHECK_THAT(positions(0, 0).x, WithinAbs(100, 1e-9));
            CHECK_THAT(positions(0, 0).y, WithinAbs(100, 1e-9));
            CHECK_THAT(positions(0, 0).z, WithinAbs(-50, 1e-9));
            CHECK_THAT(positions(positions.shape().rows - 1, 0).x, WithinAbs(-100, 1e-9));
            CHECK_THAT(positions(positions.shape().rows - 1, 0).y, WithinAbs(100, 1e-9));
            CHECK_THAT(positions(positions.shape().rows - 1, 0).z, WithinAbs(-50, 1e-9));
            CHECK_THAT(positions(0, positions.shape().cols - 1).x, WithinAbs(100, 1e-9));
            CHECK_THAT(positions(0, positions.shape().cols - 1).y, WithinAbs(100, 1e-9));
            CHECK_THAT(positions(0, positions.shape().cols - 1).z, WithinAbs(50, 1e-9));
            CHECK_THAT(positions(positions.shape().rows - 1, positions.shape().cols - 1).x, WithinAbs(-100, 1e-9));
            CHECK_THAT(positions(positions.shape().rows - 1, positions.shape().cols - 1).y, WithinAbs(100, 1e-9));
            CHECK_THAT(positions(positions.shape().rows - 1, positions.shape().cols - 1).z, WithinAbs(50, 1e-9));

            // Verify page 0 (note: this page should be equal to the eval_geometry test)
            {
                auto& values = data[0];

                // Next, find the maximum of the physical values and normalize for better comparison
                double const abs_max = normalize(values);
                CHECK_THAT(abs_max, WithinAbs(0.00448208702149741, 1e-9));

                // Verify physical values along the surface
                CHECK_THAT(std::abs(values(0, 0)), WithinAbs(0.0811805333041281, 1e-9));
                CHECK_THAT(std::abs(values(0, 1)), WithinAbs(0.0209935151691007, 1e-9));
                CHECK_THAT(std::abs(values(0, 2)), WithinAbs(0.181776966584661, 1e-9));
                CHECK_THAT(std::abs(values(0, 3)), WithinAbs(0.959227993440513, 1e-9));
                CHECK_THAT(std::abs(values(0, 4)), WithinAbs(0.959227993440513, 1e-9));
                CHECK_THAT(std::abs(values(0, 5)), WithinAbs(0.181776966584661, 1e-9));
                CHECK_THAT(std::abs(values(0, 6)), WithinAbs(0.0209935151689359, 1e-9));
                CHECK_THAT(std::abs(values(0, 7)), WithinAbs(0.0811805333041281, 1e-9));
                CHECK_THAT(std::abs(values(1, 0)), WithinAbs(0.0455986942249276, 1e-9));
                CHECK_THAT(std::abs(values(1, 1)), WithinAbs(0.0411457551690019, 1e-9));
                CHECK_THAT(std::abs(values(1, 2)), WithinAbs(0.236208994298118, 1e-9));
                CHECK_THAT(std::abs(values(1, 3)), WithinAbs(0.980316457457755, 1e-9));
                CHECK_THAT(std::abs(values(1, 4)), WithinAbs(0.980316457457755, 1e-9));
                CHECK_THAT(std::abs(values(1, 5)), WithinAbs(0.236208994298118, 1e-9));
                CHECK_THAT(std::abs(values(1, 6)), WithinAbs(0.0411457551690372, 1e-9));
                CHECK_THAT(std::abs(values(1, 7)), WithinAbs(0.0455986942249277, 1e-9));
                CHECK_THAT(std::abs(values(2, 0)), WithinAbs(0.00257584589170965, 1e-9));
                CHECK_THAT(std::abs(values(2, 1)), WithinAbs(0.0941592075283062, 1e-9));
                CHECK_THAT(std::abs(values(2, 2)), WithinAbs(0.275478907113585, 1e-9));
                CHECK_THAT(std::abs(values(2, 3)), WithinAbs(0.99429542171149, 1e-9));
                CHECK_THAT(std::abs(values(2, 4)), WithinAbs(0.99429542171149, 1e-9));
                CHECK_THAT(std::abs(values(2, 5)), WithinAbs(0.275478907113585, 1e-9));
                CHECK_THAT(std::abs(values(2, 6)), WithinAbs(0.0941592075283062, 1e-9));
                CHECK_THAT(std::abs(values(2, 7)), WithinAbs(0.00257584589170968, 1e-9));
                CHECK_THAT(std::abs(values(3, 0)), WithinAbs(0.0357596041928141, 1e-9));
                CHECK_THAT(std::abs(values(3, 1)), WithinAbs(0.128220787975769, 1e-9));
                CHECK_THAT(std::abs(values(3, 2)), WithinAbs(0.294897071700718, 1e-9));
                CHECK_THAT(std::abs(values(3, 3)), WithinAbs(1, 1e-9));
                CHECK_THAT(std::abs(values(3, 4)), WithinAbs(1, 1e-9));
                CHECK_THAT(std::abs(values(3, 5)), WithinAbs(0.294897071700718, 1e-9));
                CHECK_THAT(std::abs(values(3, 6)), WithinAbs(0.128220787975793, 1e-9));
                CHECK_THAT(std::abs(values(3, 7)), WithinAbs(0.0357596041928141, 1e-9));
                CHECK_THAT(std::abs(values(4, 0)), WithinAbs(0.0589060465262149, 1e-9));
                CHECK_THAT(std::abs(values(4, 1)), WithinAbs(0.139456471473219, 1e-9));
                CHECK_THAT(std::abs(values(4, 2)), WithinAbs(0.294128532445929, 1e-9));
                CHECK_THAT(std::abs(values(4, 3)), WithinAbs(0.997606657015968, 1e-9));
                CHECK_THAT(std::abs(values(4, 4)), WithinAbs(0.997606657015968, 1e-9));
                CHECK_THAT(std::abs(values(4, 5)), WithinAbs(0.294128532445929, 1e-9));
                CHECK_THAT(std::abs(values(4, 6)), WithinAbs(0.139456471473219, 1e-9));
                CHECK_THAT(std::abs(values(4, 7)), WithinAbs(0.0589060465262149, 1e-9));
                CHECK_THAT(std::abs(values(5, 0)), WithinAbs(0.0665616877563288, 1e-9));
                CHECK_THAT(std::abs(values(5, 1)), WithinAbs(0.132742825255253, 1e-9));
                CHECK_THAT(std::abs(values(5, 2)), WithinAbs(0.27873146020983, 1e-9));
                CHECK_THAT(std::abs(values(5, 3)), WithinAbs(0.989316385385742, 1e-9));
                CHECK_THAT(std::abs(values(5, 4)), WithinAbs(0.989316385385742, 1e-9));
                CHECK_THAT(std::abs(values(5, 5)), WithinAbs(0.27873146020983, 1e-9));
                CHECK_THAT(std::abs(values(5, 6)), WithinAbs(0.13274282525528, 1e-9));
                CHECK_THAT(std::abs(values(5, 7)), WithinAbs(0.0665616877563289, 1e-9));
                CHECK_THAT(std::abs(values(6, 0)), WithinAbs(0.0650061963206579, 1e-9));
                CHECK_THAT(std::abs(values(6, 1)), WithinAbs(0.118932954431791, 1e-9));
                CHECK_THAT(std::abs(values(6, 2)), WithinAbs(0.258936075347669, 1e-9));
                CHECK_THAT(std::abs(values(6, 3)), WithinAbs(0.979344504415583, 1e-9));
                CHECK_THAT(std::abs(values(6, 4)), WithinAbs(0.979344504415583, 1e-9));
                CHECK_THAT(std::abs(values(6, 5)), WithinAbs(0.258936075347669, 1e-9));
                CHECK_THAT(std::abs(values(6, 6)), WithinAbs(0.118932954431791, 1e-9));
                CHECK_THAT(std::abs(values(6, 7)), WithinAbs(0.0650061963206579, 1e-9));
                CHECK_THAT(std::abs(values(7, 0)), WithinAbs(0.0619228784388384, 1e-9));
                CHECK_THAT(std::abs(values(7, 1)), WithinAbs(0.108899738482795, 1e-9));
                CHECK_THAT(std::abs(values(7, 2)), WithinAbs(0.245612300311409, 1e-9));
                CHECK_THAT(std::abs(values(7, 3)), WithinAbs(0.972648506328136, 1e-9));
                CHECK_THAT(std::abs(values(7, 4)), WithinAbs(0.972648506328136, 1e-9));
                CHECK_THAT(std::abs(values(7, 5)), WithinAbs(0.245612300311409, 1e-9));
                CHECK_THAT(std::abs(values(7, 6)), WithinAbs(0.108899738482732, 1e-9));
                CHECK_THAT(std::abs(values(7, 7)), WithinAbs(0.0619228784388384, 1e-9));
                CHECK_THAT(std::abs(values(8, 0)), WithinAbs(0.0619228784388384, 1e-9));
                CHECK_THAT(std::abs(values(8, 1)), WithinAbs(0.108899738482794, 1e-9));
                CHECK_THAT(std::abs(values(8, 2)), WithinAbs(0.245612300311409, 1e-9));
                CHECK_THAT(std::abs(values(8, 3)), WithinAbs(0.972648506328136, 1e-9));
                CHECK_THAT(std::abs(values(8, 4)), WithinAbs(0.972648506328136, 1e-9));
                CHECK_THAT(std::abs(values(8, 5)), WithinAbs(0.245612300311409, 1e-9));
                CHECK_THAT(std::abs(values(8, 6)), WithinAbs(0.108899738482732, 1e-9));
                CHECK_THAT(std::abs(values(8, 7)), WithinAbs(0.0619228784388384, 1e-9));
                CHECK_THAT(std::abs(values(9, 0)), WithinAbs(0.0650061963206579, 1e-9));
                CHECK_THAT(std::abs(values(9, 1)), WithinAbs(0.118932954431791, 1e-9));
                CHECK_THAT(std::abs(values(9, 2)), WithinAbs(0.258936075347669, 1e-9));
                CHECK_THAT(std::abs(values(9, 3)), WithinAbs(0.979344504415583, 1e-9));
                CHECK_THAT(std::abs(values(9, 4)), WithinAbs(0.979344504415583, 1e-9));
                CHECK_THAT(std::abs(values(9, 5)), WithinAbs(0.258936075347669, 1e-9));
                CHECK_THAT(std::abs(values(9, 6)), WithinAbs(0.118932954431791, 1e-9));
                CHECK_THAT(std::abs(values(9, 7)), WithinAbs(0.0650061963206579, 1e-9));
                CHECK_THAT(std::abs(values(10, 0)), WithinAbs(0.0665616877563289, 1e-9));
                CHECK_THAT(std::abs(values(10, 1)), WithinAbs(0.132742825255253, 1e-9));
                CHECK_THAT(std::abs(values(10, 2)), WithinAbs(0.27873146020983, 1e-9));
                CHECK_THAT(std::abs(values(10, 3)), WithinAbs(0.989316385385742, 1e-9));
                CHECK_THAT(std::abs(values(10, 4)), WithinAbs(0.989316385385742, 1e-9));
                CHECK_THAT(std::abs(values(10, 5)), WithinAbs(0.27873146020983, 1e-9));
                CHECK_THAT(std::abs(values(10, 6)), WithinAbs(0.13274282525528, 1e-9));
                CHECK_THAT(std::abs(values(10, 7)), WithinAbs(0.0665616877563289, 1e-9));
                CHECK_THAT(std::abs(values(11, 0)), WithinAbs(0.0589060465262149, 1e-9));
                CHECK_THAT(std::abs(values(11, 1)), WithinAbs(0.139456471473219, 1e-9));
                CHECK_THAT(std::abs(values(11, 2)), WithinAbs(0.294128532445929, 1e-9));
                CHECK_THAT(std::abs(values(11, 3)), WithinAbs(0.997606657015968, 1e-9));
                CHECK_THAT(std::abs(values(11, 4)), WithinAbs(0.997606657015968, 1e-9));
                CHECK_THAT(std::abs(values(11, 5)), WithinAbs(0.294128532445929, 1e-9));
                CHECK_THAT(std::abs(values(11, 6)), WithinAbs(0.139456471473219, 1e-9));
                CHECK_THAT(std::abs(values(11, 7)), WithinAbs(0.058906046526215, 1e-9));
                CHECK_THAT(std::abs(values(12, 0)), WithinAbs(0.0357596041928872, 1e-9));
                CHECK_THAT(std::abs(values(12, 1)), WithinAbs(0.128220787975745, 1e-9));
                CHECK_THAT(std::abs(values(12, 2)), WithinAbs(0.294897071701048, 1e-9));
                CHECK_THAT(std::abs(values(12, 3)), WithinAbs(0.999999999999537, 1e-9));
                CHECK_THAT(std::abs(values(12, 4)), WithinAbs(0.999999999999537, 1e-9));
                CHECK_THAT(std::abs(values(12, 5)), WithinAbs(0.294897071701048, 1e-9));
                CHECK_THAT(std::abs(values(12, 6)), WithinAbs(0.12822078797574, 1e-9));
                CHECK_THAT(std::abs(values(12, 7)), WithinAbs(0.0357596041928873, 1e-9));
                CHECK_THAT(std::abs(values(13, 0)), WithinAbs(0.00257584589170966, 1e-9));
                CHECK_THAT(std::abs(values(13, 1)), WithinAbs(0.0941592075283062, 1e-9));
                CHECK_THAT(std::abs(values(13, 2)), WithinAbs(0.275478907113585, 1e-9));
                CHECK_THAT(std::abs(values(13, 3)), WithinAbs(0.99429542171149, 1e-9));
                CHECK_THAT(std::abs(values(13, 4)), WithinAbs(0.99429542171149, 1e-9));
                CHECK_THAT(std::abs(values(13, 5)), WithinAbs(0.275478907113585, 1e-9));
                CHECK_THAT(std::abs(values(13, 6)), WithinAbs(0.0941592075283062, 1e-9));
                CHECK_THAT(std::abs(values(13, 7)), WithinAbs(0.00257584589170969, 1e-9));
                CHECK_THAT(std::abs(values(14, 0)), WithinAbs(0.0455986942249276, 1e-9));
                CHECK_THAT(std::abs(values(14, 1)), WithinAbs(0.0411457551690019, 1e-9));
                CHECK_THAT(std::abs(values(14, 2)), WithinAbs(0.236208994298118, 1e-9));
                CHECK_THAT(std::abs(values(14, 3)), WithinAbs(0.980316457457755, 1e-9));
                CHECK_THAT(std::abs(values(14, 4)), WithinAbs(0.980316457457755, 1e-9));
                CHECK_THAT(std::abs(values(14, 5)), WithinAbs(0.236208994298118, 1e-9));
                CHECK_THAT(std::abs(values(14, 6)), WithinAbs(0.0411457551690372, 1e-9));
                CHECK_THAT(std::abs(values(14, 7)), WithinAbs(0.0455986942249276, 1e-9));
                CHECK_THAT(std::abs(values(15, 0)), WithinAbs(0.0811805333041281, 1e-9));
                CHECK_THAT(std::abs(values(15, 1)), WithinAbs(0.0209935151691007, 1e-9));
                CHECK_THAT(std::abs(values(15, 2)), WithinAbs(0.181776966584661, 1e-9));
                CHECK_THAT(std::abs(values(15, 3)), WithinAbs(0.959227993440513, 1e-9));
                CHECK_THAT(std::abs(values(15, 4)), WithinAbs(0.959227993440513, 1e-9));
                CHECK_THAT(std::abs(values(15, 5)), WithinAbs(0.181776966584661, 1e-9));
                CHECK_THAT(std::abs(values(15, 6)), WithinAbs(0.0209935151689359, 1e-9));
                CHECK_THAT(std::abs(values(15, 7)), WithinAbs(0.0811805333041281, 1e-9));
                CHECK_THAT(std::arg(values(0, 0)), WithinAbs(-1.59046134379987, 1e-6));
                CHECK_THAT(std::arg(values(0, 1)), WithinAbs(-2.18508268543536, 1e-6));
                CHECK_THAT(std::arg(values(0, 2)), WithinAbs(-0.724963766234791, 1e-6));
                CHECK_THAT(std::arg(values(0, 3)), WithinAbs(-1.68114156649414, 1e-6));
                CHECK_THAT(std::arg(values(0, 4)), WithinAbs(-1.68114156649414, 1e-6));
                CHECK_THAT(std::arg(values(0, 5)), WithinAbs(-0.724963766234791, 1e-6));
                CHECK_THAT(std::arg(values(0, 6)), WithinAbs(-2.18508268543734, 1e-6));
                CHECK_THAT(std::arg(values(0, 7)), WithinAbs(-1.59046134379987, 1e-6));
                CHECK_THAT(std::arg(values(1, 0)), WithinAbs(0.8988282123476, 1e-6));
                CHECK_THAT(std::arg(values(1, 1)), WithinAbs(0.57912013854105, 1e-6));
                CHECK_THAT(std::arg(values(1, 2)), WithinAbs(-1.82938225706847, 1e-6));
                CHECK_THAT(std::arg(values(1, 3)), WithinAbs(-2.9733018895961, 1e-6));
                CHECK_THAT(std::arg(values(1, 4)), WithinAbs(-2.9733018895961, 1e-6));
                CHECK_THAT(std::arg(values(1, 5)), WithinAbs(-1.82938225706847, 1e-6));
                CHECK_THAT(std::arg(values(1, 6)), WithinAbs(0.579120138543666, 1e-6));
                CHECK_THAT(std::arg(values(1, 7)), WithinAbs(0.898828212347601, 1e-6));
                CHECK_THAT(std::arg(values(2, 0)), WithinAbs(-1.53877106842563, 1e-6));
                CHECK_THAT(std::arg(values(2, 1)), WithinAbs(1.66489040853495, 1e-6));
                CHECK_THAT(std::arg(values(2, 2)), WithinAbs(-1.2999218759808, 1e-6));
                CHECK_THAT(std::arg(values(2, 3)), WithinAbs(-2.37743568424626, 1e-6));
                CHECK_THAT(std::arg(values(2, 4)), WithinAbs(-2.37743568424626, 1e-6));
                CHECK_THAT(std::arg(values(2, 5)), WithinAbs(-1.2999218759808, 1e-6));
                CHECK_THAT(std::arg(values(2, 6)), WithinAbs(1.66489040853494, 1e-6));
                CHECK_THAT(std::arg(values(2, 7)), WithinAbs(-1.53877106842561, 1e-6));
                CHECK_THAT(std::arg(values(3, 0)), WithinAbs(2.39951755383822, 1e-6));
                CHECK_THAT(std::arg(values(3, 1)), WithinAbs(0.570607536876258, 1e-6));
                CHECK_THAT(std::arg(values(3, 2)), WithinAbs(-2.9570550451856, 1e-6));
                CHECK_THAT(std::arg(values(3, 3)), WithinAbs(2.34375554995525, 1e-6));
                CHECK_THAT(std::arg(values(3, 4)), WithinAbs(2.34375554995525, 1e-6));
                CHECK_THAT(std::arg(values(3, 5)), WithinAbs(-2.9570550451856, 1e-6));
                CHECK_THAT(std::arg(values(3, 6)), WithinAbs(0.570607536876678, 1e-6));
                CHECK_THAT(std::arg(values(3, 7)), WithinAbs(2.39951755383822, 1e-6));
                CHECK_THAT(std::arg(values(4, 0)), WithinAbs(-1.73470220406938, 1e-6));
                CHECK_THAT(std::arg(values(4, 1)), WithinAbs(-0.875285092401346, 1e-6));
                CHECK_THAT(std::arg(values(4, 2)), WithinAbs(0.639784789812871, 1e-6));
                CHECK_THAT(std::arg(values(4, 3)), WithinAbs(-0.565601279784796, 1e-6));
                CHECK_THAT(std::arg(values(4, 4)), WithinAbs(-0.565601279784796, 1e-6));
                CHECK_THAT(std::arg(values(4, 5)), WithinAbs(0.639784789812871, 1e-6));
                CHECK_THAT(std::arg(values(4, 6)), WithinAbs(-0.875285092401345, 1e-6));
                CHECK_THAT(std::arg(values(4, 7)), WithinAbs(-1.73470220406938, 1e-6));
                CHECK_THAT(std::arg(values(5, 0)), WithinAbs(-2.65587357462535, 1e-6));
                CHECK_THAT(std::arg(values(5, 1)), WithinAbs(-1.30302497013105, 1e-6));
                CHECK_THAT(std::arg(values(5, 2)), WithinAbs(-2.56513148566834, 1e-6));
                CHECK_THAT(std::arg(values(5, 3)), WithinAbs(1.50288452264914, 1e-6));
                CHECK_THAT(std::arg(values(5, 4)), WithinAbs(1.50288452264914, 1e-6));
                CHECK_THAT(std::arg(values(5, 5)), WithinAbs(-2.56513148566834, 1e-6));
                CHECK_THAT(std::arg(values(5, 6)), WithinAbs(-1.30302497013065, 1e-6));
                CHECK_THAT(std::arg(values(5, 7)), WithinAbs(-2.65587357462535, 1e-6));
                CHECK_THAT(std::arg(values(6, 0)), WithinAbs(2.90989473171143, 1e-6));
                CHECK_THAT(std::arg(values(6, 1)), WithinAbs(1.33584297314295, 1e-6));
                CHECK_THAT(std::arg(values(6, 2)), WithinAbs(1.07597558528228, 1e-6));
                CHECK_THAT(std::arg(values(6, 3)), WithinAbs(2.80787643375169, 1e-6));
                CHECK_THAT(std::arg(values(6, 4)), WithinAbs(2.80787643375169, 1e-6));
                CHECK_THAT(std::arg(values(6, 5)), WithinAbs(1.07597558528228, 1e-6));
                CHECK_THAT(std::arg(values(6, 6)), WithinAbs(1.33584297314295, 1e-6));
                CHECK_THAT(std::arg(values(6, 7)), WithinAbs(2.90989473171143, 1e-6));
                CHECK_THAT(std::arg(values(7, 0)), WithinAbs(1.41636191652562, 1e-6));
                CHECK_THAT(std::arg(values(7, 1)), WithinAbs(-1.29995402322705, 1e-6));
                CHECK_THAT(std::arg(values(7, 2)), WithinAbs(2.34715161578265, 1e-6));
                CHECK_THAT(std::arg(values(7, 3)), WithinAbs(-0.0792612673224425, 1e-6));
                CHECK_THAT(std::arg(values(7, 4)), WithinAbs(-0.0792612673224425, 1e-6));
                CHECK_THAT(std::arg(values(7, 5)), WithinAbs(2.34715161578265, 1e-6));
                CHECK_THAT(std::arg(values(7, 6)), WithinAbs(-1.29995402322705, 1e-6));
                CHECK_THAT(std::arg(values(7, 7)), WithinAbs(1.41636191652563, 1e-6));
                CHECK_THAT(std::arg(values(8, 0)), WithinAbs(1.41636191652562, 1e-6));
                CHECK_THAT(std::arg(values(8, 1)), WithinAbs(-1.29995402322705, 1e-6));
                CHECK_THAT(std::arg(values(8, 2)), WithinAbs(2.34715161578265, 1e-6));
                CHECK_THAT(std::arg(values(8, 3)), WithinAbs(-0.0792612673224425, 1e-6));
                CHECK_THAT(std::arg(values(8, 4)), WithinAbs(-0.0792612673224425, 1e-6));
                CHECK_THAT(std::arg(values(8, 5)), WithinAbs(2.34715161578265, 1e-6));
                CHECK_THAT(std::arg(values(8, 6)), WithinAbs(-1.29995402322705, 1e-6));
                CHECK_THAT(std::arg(values(8, 7)), WithinAbs(1.41636191652563, 1e-6));
                CHECK_THAT(std::arg(values(9, 0)), WithinAbs(2.90989473171143, 1e-6));
                CHECK_THAT(std::arg(values(9, 1)), WithinAbs(1.33584297314295, 1e-6));
                CHECK_THAT(std::arg(values(9, 2)), WithinAbs(1.07597558528228, 1e-6));
                CHECK_THAT(std::arg(values(9, 3)), WithinAbs(2.80787643375169, 1e-6));
                CHECK_THAT(std::arg(values(9, 4)), WithinAbs(2.80787643375169, 1e-6));
                CHECK_THAT(std::arg(values(9, 5)), WithinAbs(1.07597558528228, 1e-6));
                CHECK_THAT(std::arg(values(9, 6)), WithinAbs(1.33584297314295, 1e-6));
                CHECK_THAT(std::arg(values(9, 7)), WithinAbs(2.90989473171143, 1e-6));
                CHECK_THAT(std::arg(values(10, 0)), WithinAbs(-2.65587357462535, 1e-6));
                CHECK_THAT(std::arg(values(10, 1)), WithinAbs(-1.30302497013105, 1e-6));
                CHECK_THAT(std::arg(values(10, 2)), WithinAbs(-2.56513148566834, 1e-6));
                CHECK_THAT(std::arg(values(10, 3)), WithinAbs(1.50288452264914, 1e-6));
                CHECK_THAT(std::arg(values(10, 4)), WithinAbs(1.50288452264914, 1e-6));
                CHECK_THAT(std::arg(values(10, 5)), WithinAbs(-2.56513148566834, 1e-6));
                CHECK_THAT(std::arg(values(10, 6)), WithinAbs(-1.30302497013065, 1e-6));
                CHECK_THAT(std::arg(values(10, 7)), WithinAbs(-2.65587357462535, 1e-6));
                CHECK_THAT(std::arg(values(11, 0)), WithinAbs(-1.73470220406938, 1e-6));
                CHECK_THAT(std::arg(values(11, 1)), WithinAbs(-0.875285092401346, 1e-6));
                CHECK_THAT(std::arg(values(11, 2)), WithinAbs(0.639784789812871, 1e-6));
                CHECK_THAT(std::arg(values(11, 3)), WithinAbs(-0.565601279784796, 1e-6));
                CHECK_THAT(std::arg(values(11, 4)), WithinAbs(-0.565601279784796, 1e-6));
                CHECK_THAT(std::arg(values(11, 5)), WithinAbs(0.639784789812871, 1e-6));
                CHECK_THAT(std::arg(values(11, 6)), WithinAbs(-0.875285092401345, 1e-6));
                CHECK_THAT(std::arg(values(11, 7)), WithinAbs(-1.73470220406938, 1e-6));
                CHECK_THAT(std::arg(values(12, 0)), WithinAbs(2.39951755384229, 1e-6));
                CHECK_THAT(std::arg(values(12, 1)), WithinAbs(0.570607536877226, 1e-6));
                CHECK_THAT(std::arg(values(12, 2)), WithinAbs(-2.95705504518585, 1e-6));
                CHECK_THAT(std::arg(values(12, 3)), WithinAbs(2.34375554995456, 1e-6));
                CHECK_THAT(std::arg(values(12, 4)), WithinAbs(2.34375554995456, 1e-6));
                CHECK_THAT(std::arg(values(12, 5)), WithinAbs(-2.95705504518585, 1e-6));
                CHECK_THAT(std::arg(values(12, 6)), WithinAbs(0.570607536876314, 1e-6));
                CHECK_THAT(std::arg(values(12, 7)), WithinAbs(2.39951755384229, 1e-6));
                CHECK_THAT(std::arg(values(13, 0)), WithinAbs(-1.53877106842562, 1e-6));
                CHECK_THAT(std::arg(values(13, 1)), WithinAbs(1.66489040853494, 1e-6));
                CHECK_THAT(std::arg(values(13, 2)), WithinAbs(-1.2999218759808, 1e-6));
                CHECK_THAT(std::arg(values(13, 3)), WithinAbs(-2.37743568424626, 1e-6));
                CHECK_THAT(std::arg(values(13, 4)), WithinAbs(-2.37743568424626, 1e-6));
                CHECK_THAT(std::arg(values(13, 5)), WithinAbs(-1.2999218759808, 1e-6));
                CHECK_THAT(std::arg(values(13, 6)), WithinAbs(1.66489040853494, 1e-6));
                CHECK_THAT(std::arg(values(13, 7)), WithinAbs(-1.53877106842561, 1e-6));
                CHECK_THAT(std::arg(values(14, 0)), WithinAbs(0.898828212347599, 1e-6));
                CHECK_THAT(std::arg(values(14, 1)), WithinAbs(0.57912013854105, 1e-6));
                CHECK_THAT(std::arg(values(14, 2)), WithinAbs(-1.82938225706847, 1e-6));
                CHECK_THAT(std::arg(values(14, 3)), WithinAbs(-2.9733018895961, 1e-6));
                CHECK_THAT(std::arg(values(14, 4)), WithinAbs(-2.9733018895961, 1e-6));
                CHECK_THAT(std::arg(values(14, 5)), WithinAbs(-1.82938225706848, 1e-6));
                CHECK_THAT(std::arg(values(14, 6)), WithinAbs(0.579120138543665, 1e-6));
                CHECK_THAT(std::arg(values(14, 7)), WithinAbs(0.8988282123476, 1e-6));
                CHECK_THAT(std::arg(values(15, 0)), WithinAbs(-1.59046134379987, 1e-6));
                CHECK_THAT(std::arg(values(15, 1)), WithinAbs(-2.18508268543537, 1e-6));
                CHECK_THAT(std::arg(values(15, 2)), WithinAbs(-0.724963766234791, 1e-6));
                CHECK_THAT(std::arg(values(15, 3)), WithinAbs(-1.68114156649414, 1e-6));
                CHECK_THAT(std::arg(values(15, 4)), WithinAbs(-1.68114156649414, 1e-6));
                CHECK_THAT(std::arg(values(15, 5)), WithinAbs(-0.724963766234791, 1e-6));
                CHECK_THAT(std::arg(values(15, 6)), WithinAbs(-2.18508268543733, 1e-6));
                CHECK_THAT(std::arg(values(15, 7)), WithinAbs(-1.59046134379987, 1e-6));
            }

            // Verify page 1
            {
                auto& values = data[1];

                // Next, find the maximum of the physical values and normalize for better comparison
                double const abs_max = normalize(values);
                CHECK_THAT(abs_max, WithinAbs(0.02045743339963739, 1e-9));

                // Verify physical values along the surface
                CHECK_THAT(std::abs(values(0, 0)), WithinAbs(0.0813920796290424, 1e-9));
                CHECK_THAT(std::abs(values(0, 1)), WithinAbs(0.159917982283952, 1e-9));
                CHECK_THAT(std::abs(values(0, 2)), WithinAbs(0.205458647782092, 1e-9));
                CHECK_THAT(std::abs(values(0, 3)), WithinAbs(0.8090469509971, 1e-9));
                CHECK_THAT(std::abs(values(0, 4)), WithinAbs(0.8090469509971, 1e-9));
                CHECK_THAT(std::abs(values(0, 5)), WithinAbs(0.205458647782091, 1e-9));
                CHECK_THAT(std::abs(values(0, 6)), WithinAbs(0.15991798228388, 1e-9));
                CHECK_THAT(std::abs(values(0, 7)), WithinAbs(0.0813920796290423, 1e-9));
                CHECK_THAT(std::abs(values(1, 0)), WithinAbs(0.043570731989055, 1e-9));
                CHECK_THAT(std::abs(values(1, 1)), WithinAbs(0.180815780118988, 1e-9));
                CHECK_THAT(std::abs(values(1, 2)), WithinAbs(0.15471939047631, 1e-9));
                CHECK_THAT(std::abs(values(1, 3)), WithinAbs(0.848680945743085, 1e-9));
                CHECK_THAT(std::abs(values(1, 4)), WithinAbs(0.848680945743085, 1e-9));
                CHECK_THAT(std::abs(values(1, 5)), WithinAbs(0.15471939047631, 1e-9));
                CHECK_THAT(std::abs(values(1, 6)), WithinAbs(0.180815780118965, 1e-9));
                CHECK_THAT(std::abs(values(1, 7)), WithinAbs(0.0435707319890549, 1e-9));
                CHECK_THAT(std::abs(values(2, 0)), WithinAbs(0.00253488461355195, 1e-9));
                CHECK_THAT(std::abs(values(2, 1)), WithinAbs(0.188632002748327, 1e-9));
                CHECK_THAT(std::abs(values(2, 2)), WithinAbs(0.0980501762348033, 1e-9));
                CHECK_THAT(std::abs(values(2, 3)), WithinAbs(0.886905104645068, 1e-9));
                CHECK_THAT(std::abs(values(2, 4)), WithinAbs(0.886905104645068, 1e-9));
                CHECK_THAT(std::abs(values(2, 5)), WithinAbs(0.0980501762348032, 1e-9));
                CHECK_THAT(std::abs(values(2, 6)), WithinAbs(0.188632002748327, 1e-9));
                CHECK_THAT(std::abs(values(2, 7)), WithinAbs(0.00253488461355193, 1e-9));
                CHECK_THAT(std::abs(values(3, 0)), WithinAbs(0.0365040952134442, 1e-9));
                CHECK_THAT(std::abs(values(3, 1)), WithinAbs(0.182481715515508, 1e-9));
                CHECK_THAT(std::abs(values(3, 2)), WithinAbs(0.0392879112594042, 1e-9));
                CHECK_THAT(std::abs(values(3, 3)), WithinAbs(0.922020581974186, 1e-9));
                CHECK_THAT(std::abs(values(3, 4)), WithinAbs(0.922020581974186, 1e-9));
                CHECK_THAT(std::abs(values(3, 5)), WithinAbs(0.0392879112594042, 1e-9));
                CHECK_THAT(std::abs(values(3, 6)), WithinAbs(0.182481715515508, 1e-9));
                CHECK_THAT(std::abs(values(3, 7)), WithinAbs(0.0365040952134442, 1e-9));
                CHECK_THAT(std::abs(values(4, 0)), WithinAbs(0.0666287244737948, 1e-9));
                CHECK_THAT(std::abs(values(4, 1)), WithinAbs(0.16489049508561, 1e-9));
                CHECK_THAT(std::abs(values(4, 2)), WithinAbs(0.019185876248624, 1e-9));
                CHECK_THAT(std::abs(values(4, 3)), WithinAbs(0.952208610160764, 1e-9));
                CHECK_THAT(std::abs(values(4, 4)), WithinAbs(0.952208610160764, 1e-9));
                CHECK_THAT(std::abs(values(4, 5)), WithinAbs(0.019185876248624, 1e-9));
                CHECK_THAT(std::abs(values(4, 6)), WithinAbs(0.16489049508561, 1e-9));
                CHECK_THAT(std::abs(values(4, 7)), WithinAbs(0.0666287244737948, 1e-9));
                CHECK_THAT(std::abs(values(5, 0)), WithinAbs(0.0858695121111939, 1e-9));
                CHECK_THAT(std::abs(values(5, 1)), WithinAbs(0.141758512009754, 1e-9));
                CHECK_THAT(std::abs(values(5, 2)), WithinAbs(0.0662395726249229, 1e-9));
                CHECK_THAT(std::abs(values(5, 3)), WithinAbs(0.975866754178091, 1e-9));
                CHECK_THAT(std::abs(values(5, 4)), WithinAbs(0.975866754178091, 1e-9));
                CHECK_THAT(std::abs(values(5, 5)), WithinAbs(0.0662395726249229, 1e-9));
                CHECK_THAT(std::abs(values(5, 6)), WithinAbs(0.141758512009737, 1e-9));
                CHECK_THAT(std::abs(values(5, 7)), WithinAbs(0.0858695121111939, 1e-9));
                CHECK_THAT(std::abs(values(6, 0)), WithinAbs(0.0955417665208314, 1e-9));
                CHECK_THAT(std::abs(values(6, 1)), WithinAbs(0.12065295827092, 1e-9));
                CHECK_THAT(std::abs(values(6, 2)), WithinAbs(0.100767426207421, 1e-9));
                CHECK_THAT(std::abs(values(6, 3)), WithinAbs(0.991938924464441, 1e-9));
                CHECK_THAT(std::abs(values(6, 4)), WithinAbs(0.991938924464441, 1e-9));
                CHECK_THAT(std::abs(values(6, 5)), WithinAbs(0.100767426207421, 1e-9));
                CHECK_THAT(std::abs(values(6, 6)), WithinAbs(0.12065295827092, 1e-9));
                CHECK_THAT(std::abs(values(6, 7)), WithinAbs(0.0955417665208314, 1e-9));
                CHECK_THAT(std::abs(values(7, 0)), WithinAbs(0.0990390295480543, 1e-9));
                CHECK_THAT(std::abs(values(7, 1)), WithinAbs(0.108229371169274, 1e-9));
                CHECK_THAT(std::abs(values(7, 2)), WithinAbs(0.118767855264159, 1e-9));
                CHECK_THAT(std::abs(values(7, 3)), WithinAbs(1, 1e-9));
                CHECK_THAT(std::abs(values(7, 4)), WithinAbs(1, 1e-9));
                CHECK_THAT(std::abs(values(7, 5)), WithinAbs(0.118767855264159, 1e-9));
                CHECK_THAT(std::abs(values(7, 6)), WithinAbs(0.108229371169274, 1e-9));
                CHECK_THAT(std::abs(values(7, 7)), WithinAbs(0.0990390295480543, 1e-9));
                CHECK_THAT(std::abs(values(8, 0)), WithinAbs(0.0990390295480544, 1e-9));
                CHECK_THAT(std::abs(values(8, 1)), WithinAbs(0.108229371169274, 1e-9));
                CHECK_THAT(std::abs(values(8, 2)), WithinAbs(0.118767855264159, 1e-9));
                CHECK_THAT(std::abs(values(8, 3)), WithinAbs(1, 1e-9));
                CHECK_THAT(std::abs(values(8, 4)), WithinAbs(1, 1e-9));
                CHECK_THAT(std::abs(values(8, 5)), WithinAbs(0.118767855264159, 1e-9));
                CHECK_THAT(std::abs(values(8, 6)), WithinAbs(0.108229371169274, 1e-9));
                CHECK_THAT(std::abs(values(8, 7)), WithinAbs(0.0990390295480543, 1e-9));
                CHECK_THAT(std::abs(values(9, 0)), WithinAbs(0.0955417665208314, 1e-9));
                CHECK_THAT(std::abs(values(9, 1)), WithinAbs(0.12065295827092, 1e-9));
                CHECK_THAT(std::abs(values(9, 2)), WithinAbs(0.100767426207421, 1e-9));
                CHECK_THAT(std::abs(values(9, 3)), WithinAbs(0.991938924464441, 1e-9));
                CHECK_THAT(std::abs(values(9, 4)), WithinAbs(0.991938924464441, 1e-9));
                CHECK_THAT(std::abs(values(9, 5)), WithinAbs(0.100767426207421, 1e-9));
                CHECK_THAT(std::abs(values(9, 6)), WithinAbs(0.12065295827092, 1e-9));
                CHECK_THAT(std::abs(values(9, 7)), WithinAbs(0.0955417665208314, 1e-9));
                CHECK_THAT(std::abs(values(10, 0)), WithinAbs(0.0858695121111939, 1e-9));
                CHECK_THAT(std::abs(values(10, 1)), WithinAbs(0.141758512009754, 1e-9));
                CHECK_THAT(std::abs(values(10, 2)), WithinAbs(0.0662395726249229, 1e-9));
                CHECK_THAT(std::abs(values(10, 3)), WithinAbs(0.975866754178091, 1e-9));
                CHECK_THAT(std::abs(values(10, 4)), WithinAbs(0.975866754178091, 1e-9));
                CHECK_THAT(std::abs(values(10, 5)), WithinAbs(0.0662395726249229, 1e-9));
                CHECK_THAT(std::abs(values(10, 6)), WithinAbs(0.141758512009737, 1e-9));
                CHECK_THAT(std::abs(values(10, 7)), WithinAbs(0.0858695121111939, 1e-9));
                CHECK_THAT(std::abs(values(11, 0)), WithinAbs(0.0666287244737948, 1e-9));
                CHECK_THAT(std::abs(values(11, 1)), WithinAbs(0.16489049508561, 1e-9));
                CHECK_THAT(std::abs(values(11, 2)), WithinAbs(0.019185876248624, 1e-9));
                CHECK_THAT(std::abs(values(11, 3)), WithinAbs(0.952208610160764, 1e-9));
                CHECK_THAT(std::abs(values(11, 4)), WithinAbs(0.952208610160764, 1e-9));
                CHECK_THAT(std::abs(values(11, 5)), WithinAbs(0.019185876248624, 1e-9));
                CHECK_THAT(std::abs(values(11, 6)), WithinAbs(0.16489049508561, 1e-9));
                CHECK_THAT(std::abs(values(11, 7)), WithinAbs(0.0666287244737948, 1e-9));
                CHECK_THAT(std::abs(values(12, 0)), WithinAbs(0.0365040952133544, 1e-9));
                CHECK_THAT(std::abs(values(12, 1)), WithinAbs(0.182481715515543, 1e-9));
                CHECK_THAT(std::abs(values(12, 2)), WithinAbs(0.0392879112593979, 1e-9));
                CHECK_THAT(std::abs(values(12, 3)), WithinAbs(0.922020581974036, 1e-9));
                CHECK_THAT(std::abs(values(12, 4)), WithinAbs(0.922020581974036, 1e-9));
                CHECK_THAT(std::abs(values(12, 5)), WithinAbs(0.0392879112593979, 1e-9));
                CHECK_THAT(std::abs(values(12, 6)), WithinAbs(0.1824817155155, 1e-9));
                CHECK_THAT(std::abs(values(12, 7)), WithinAbs(0.0365040952133545, 1e-9));
                CHECK_THAT(std::abs(values(13, 0)), WithinAbs(0.00253488461355196, 1e-9));
                CHECK_THAT(std::abs(values(13, 1)), WithinAbs(0.188632002748327, 1e-9));
                CHECK_THAT(std::abs(values(13, 2)), WithinAbs(0.0980501762348032, 1e-9));
                CHECK_THAT(std::abs(values(13, 3)), WithinAbs(0.886905104645069, 1e-9));
                CHECK_THAT(std::abs(values(13, 4)), WithinAbs(0.886905104645068, 1e-9));
                CHECK_THAT(std::abs(values(13, 5)), WithinAbs(0.0980501762348032, 1e-9));
                CHECK_THAT(std::abs(values(13, 6)), WithinAbs(0.188632002748327, 1e-9));
                CHECK_THAT(std::abs(values(13, 7)), WithinAbs(0.00253488461355191, 1e-9));
                CHECK_THAT(std::abs(values(14, 0)), WithinAbs(0.043570731989055, 1e-9));
                CHECK_THAT(std::abs(values(14, 1)), WithinAbs(0.180815780118988, 1e-9));
                CHECK_THAT(std::abs(values(14, 2)), WithinAbs(0.15471939047631, 1e-9));
                CHECK_THAT(std::abs(values(14, 3)), WithinAbs(0.848680945743085, 1e-9));
                CHECK_THAT(std::abs(values(14, 4)), WithinAbs(0.848680945743085, 1e-9));
                CHECK_THAT(std::abs(values(14, 5)), WithinAbs(0.15471939047631, 1e-9));
                CHECK_THAT(std::abs(values(14, 6)), WithinAbs(0.180815780118965, 1e-9));
                CHECK_THAT(std::abs(values(14, 7)), WithinAbs(0.0435707319890549, 1e-9));
                CHECK_THAT(std::abs(values(15, 0)), WithinAbs(0.0813920796290424, 1e-9));
                CHECK_THAT(std::abs(values(15, 1)), WithinAbs(0.159917982283952, 1e-9));
                CHECK_THAT(std::abs(values(15, 2)), WithinAbs(0.205458647782092, 1e-9));
                CHECK_THAT(std::abs(values(15, 3)), WithinAbs(0.8090469509971, 1e-9));
                CHECK_THAT(std::abs(values(15, 4)), WithinAbs(0.8090469509971, 1e-9));
                CHECK_THAT(std::abs(values(15, 5)), WithinAbs(0.205458647782091, 1e-9));
                CHECK_THAT(std::abs(values(15, 6)), WithinAbs(0.15991798228388, 1e-9));
                CHECK_THAT(std::abs(values(15, 7)), WithinAbs(0.0813920796290423, 1e-9));
                CHECK_THAT(std::arg(values(0, 0)), WithinAbs(1.5660297421716, 1e-6));
                CHECK_THAT(std::arg(values(0, 1)), WithinAbs(-1.02342533187269, 1e-6));
                CHECK_THAT(std::arg(values(0, 2)), WithinAbs(1.1393340071669, 1e-6));
                CHECK_THAT(std::arg(values(0, 3)), WithinAbs(-1.64573752094675, 1e-6));
                CHECK_THAT(std::arg(values(0, 4)), WithinAbs(-1.64573752094675, 1e-6));
                CHECK_THAT(std::arg(values(0, 5)), WithinAbs(1.1393340071669, 1e-6));
                CHECK_THAT(std::arg(values(0, 6)), WithinAbs(-1.02342533187256, 1e-6));
                CHECK_THAT(std::arg(values(0, 7)), WithinAbs(1.5660297421716, 1e-6));
                CHECK_THAT(std::arg(values(1, 0)), WithinAbs(1.13662676122023, 1e-6));
                CHECK_THAT(std::arg(values(1, 1)), WithinAbs(3.04174809369909, 1e-6));
                CHECK_THAT(std::arg(values(1, 2)), WithinAbs(0.408142059494028, 1e-6));
                CHECK_THAT(std::arg(values(1, 3)), WithinAbs(1.68124791287845, 1e-6));
                CHECK_THAT(std::arg(values(1, 4)), WithinAbs(1.68124791287845, 1e-6));
                CHECK_THAT(std::arg(values(1, 5)), WithinAbs(0.408142059494028, 1e-6));
                CHECK_THAT(std::arg(values(1, 6)), WithinAbs(3.04174809369858, 1e-6));
                CHECK_THAT(std::arg(values(1, 7)), WithinAbs(1.13662676122023, 1e-6));
                CHECK_THAT(std::arg(values(2, 0)), WithinAbs(-0.281782173001943, 1e-6));
                CHECK_THAT(std::arg(values(2, 1)), WithinAbs(-0.44585260614164, 1e-6));
                CHECK_THAT(std::arg(values(2, 2)), WithinAbs(-1.31038369616287, 1e-6));
                CHECK_THAT(std::arg(values(2, 3)), WithinAbs(-2.11076064133287, 1e-6));
                CHECK_THAT(std::arg(values(2, 4)), WithinAbs(-2.11076064133287, 1e-6));
                CHECK_THAT(std::arg(values(2, 5)), WithinAbs(-1.31038369616287, 1e-6));
                CHECK_THAT(std::arg(values(2, 6)), WithinAbs(-0.44585260614164, 1e-6));
                CHECK_THAT(std::arg(values(2, 7)), WithinAbs(-0.28178217300192, 1e-6));
                CHECK_THAT(std::arg(values(3, 0)), WithinAbs(-1.03214774294244, 1e-6));
                CHECK_THAT(std::arg(values(3, 1)), WithinAbs(0.914750636729227, 1e-6));
                CHECK_THAT(std::arg(values(3, 2)), WithinAbs(1.88056378952507, 1e-6));
                CHECK_THAT(std::arg(values(3, 3)), WithinAbs(-1.05826267843879, 1e-6));
                CHECK_THAT(std::arg(values(3, 4)), WithinAbs(-1.05826267843879, 1e-6));
                CHECK_THAT(std::arg(values(3, 5)), WithinAbs(1.88056378952508, 1e-6));
                CHECK_THAT(std::arg(values(3, 6)), WithinAbs(0.914750636729228, 1e-6));
                CHECK_THAT(std::arg(values(3, 7)), WithinAbs(-1.03214774294244, 1e-6));
                CHECK_THAT(std::arg(values(4, 0)), WithinAbs(0.406633098776391, 1e-6));
                CHECK_THAT(std::arg(values(4, 1)), WithinAbs(-0.0500930861027495, 1e-6));
                CHECK_THAT(std::arg(values(4, 2)), WithinAbs(-1.52278451940661, 1e-6));
                CHECK_THAT(std::arg(values(4, 3)), WithinAbs(-2.99849032627672, 1e-6));
                CHECK_THAT(std::arg(values(4, 4)), WithinAbs(-2.99849032627672, 1e-6));
                CHECK_THAT(std::arg(values(4, 5)), WithinAbs(-1.52278451940661, 1e-6));
                CHECK_THAT(std::arg(values(4, 6)), WithinAbs(-0.0500930861027497, 1e-6));
                CHECK_THAT(std::arg(values(4, 7)), WithinAbs(0.406633098776391, 1e-6));
                CHECK_THAT(std::arg(values(5, 0)), WithinAbs(-0.206461548927317, 1e-6));
                CHECK_THAT(std::arg(values(5, 1)), WithinAbs(1.75954197966185, 1e-6));
                CHECK_THAT(std::arg(values(5, 2)), WithinAbs(0.799886570466575, 1e-6));
                CHECK_THAT(std::arg(values(5, 3)), WithinAbs(2.56860641118644, 1e-6));
                CHECK_THAT(std::arg(values(5, 4)), WithinAbs(2.56860641118644, 1e-6));
                CHECK_THAT(std::arg(values(5, 5)), WithinAbs(0.799886570466576, 1e-6));
                CHECK_THAT(std::arg(values(5, 6)), WithinAbs(1.75954197966149, 1e-6));
                CHECK_THAT(std::arg(values(5, 7)), WithinAbs(-0.206461548927317, 1e-6));
                CHECK_THAT(std::arg(values(6, 0)), WithinAbs(-2.77947442039561, 1e-6));
                CHECK_THAT(std::arg(values(6, 1)), WithinAbs(-0.668997750346929, 1e-6));
                CHECK_THAT(std::arg(values(6, 2)), WithinAbs(-3.02092983736254, 1e-6));
                CHECK_THAT(std::arg(values(6, 3)), WithinAbs(1.34361210956709, 1e-6));
                CHECK_THAT(std::arg(values(6, 4)), WithinAbs(1.34361210956709, 1e-6));
                CHECK_THAT(std::arg(values(6, 5)), WithinAbs(-3.02092983736254, 1e-6));
                CHECK_THAT(std::arg(values(6, 6)), WithinAbs(-0.668997750346928, 1e-6));
                CHECK_THAT(std::arg(values(6, 7)), WithinAbs(-2.77947442039561, 1e-6));
                CHECK_THAT(std::arg(values(7, 0)), WithinAbs(0.413072121693463, 1e-6));
                CHECK_THAT(std::arg(values(7, 1)), WithinAbs(-0.330919849876266, 1e-6));
                CHECK_THAT(std::arg(values(7, 2)), WithinAbs(-0.0696652751859242, 1e-6));
                CHECK_THAT(std::arg(values(7, 3)), WithinAbs(-0.581499217679268, 1e-6));
                CHECK_THAT(std::arg(values(7, 4)), WithinAbs(-0.581499217679268, 1e-6));
                CHECK_THAT(std::arg(values(7, 5)), WithinAbs(-0.0696652751859251, 1e-6));
                CHECK_THAT(std::arg(values(7, 6)), WithinAbs(-0.330919849876267, 1e-6));
                CHECK_THAT(std::arg(values(7, 7)), WithinAbs(0.413072121693464, 1e-6));
                CHECK_THAT(std::arg(values(8, 0)), WithinAbs(0.413072121693463, 1e-6));
                CHECK_THAT(std::arg(values(8, 1)), WithinAbs(-0.330919849876267, 1e-6));
                CHECK_THAT(std::arg(values(8, 2)), WithinAbs(-0.0696652751859242, 1e-6));
                CHECK_THAT(std::arg(values(8, 3)), WithinAbs(-0.581499217679268, 1e-6));
                CHECK_THAT(std::arg(values(8, 4)), WithinAbs(-0.581499217679268, 1e-6));
                CHECK_THAT(std::arg(values(8, 5)), WithinAbs(-0.0696652751859251, 1e-6));
                CHECK_THAT(std::arg(values(8, 6)), WithinAbs(-0.330919849876267, 1e-6));
                CHECK_THAT(std::arg(values(8, 7)), WithinAbs(0.413072121693464, 1e-6));
                CHECK_THAT(std::arg(values(9, 0)), WithinAbs(-2.77947442039561, 1e-6));
                CHECK_THAT(std::arg(values(9, 1)), WithinAbs(-0.668997750346928, 1e-6));
                CHECK_THAT(std::arg(values(9, 2)), WithinAbs(-3.02092983736254, 1e-6));
                CHECK_THAT(std::arg(values(9, 3)), WithinAbs(1.34361210956709, 1e-6));
                CHECK_THAT(std::arg(values(9, 4)), WithinAbs(1.34361210956709, 1e-6));
                CHECK_THAT(std::arg(values(9, 5)), WithinAbs(-3.02092983736254, 1e-6));
                CHECK_THAT(std::arg(values(9, 6)), WithinAbs(-0.668997750346928, 1e-6));
                CHECK_THAT(std::arg(values(9, 7)), WithinAbs(-2.77947442039561, 1e-6));
                CHECK_THAT(std::arg(values(10, 0)), WithinAbs(-0.206461548927317, 1e-6));
                CHECK_THAT(std::arg(values(10, 1)), WithinAbs(1.75954197966185, 1e-6));
                CHECK_THAT(std::arg(values(10, 2)), WithinAbs(0.799886570466575, 1e-6));
                CHECK_THAT(std::arg(values(10, 3)), WithinAbs(2.56860641118644, 1e-6));
                CHECK_THAT(std::arg(values(10, 4)), WithinAbs(2.56860641118644, 1e-6));
                CHECK_THAT(std::arg(values(10, 5)), WithinAbs(0.799886570466576, 1e-6));
                CHECK_THAT(std::arg(values(10, 6)), WithinAbs(1.75954197966149, 1e-6));
                CHECK_THAT(std::arg(values(10, 7)), WithinAbs(-0.206461548927317, 1e-6));
                CHECK_THAT(std::arg(values(11, 0)), WithinAbs(0.40663309877639, 1e-6));
                CHECK_THAT(std::arg(values(11, 1)), WithinAbs(-0.0500930861027495, 1e-6));
                CHECK_THAT(std::arg(values(11, 2)), WithinAbs(-1.52278451940661, 1e-6));
                CHECK_THAT(std::arg(values(11, 3)), WithinAbs(-2.99849032627672, 1e-6));
                CHECK_THAT(std::arg(values(11, 4)), WithinAbs(-2.99849032627672, 1e-6));
                CHECK_THAT(std::arg(values(11, 5)), WithinAbs(-1.52278451940661, 1e-6));
                CHECK_THAT(std::arg(values(11, 6)), WithinAbs(-0.0500930861027495, 1e-6));
                CHECK_THAT(std::arg(values(11, 7)), WithinAbs(0.40663309877639, 1e-6));
                CHECK_THAT(std::arg(values(12, 0)), WithinAbs(-1.03214774294059, 1e-6));
                CHECK_THAT(std::arg(values(12, 1)), WithinAbs(0.91475063672914, 1e-6));
                CHECK_THAT(std::arg(values(12, 2)), WithinAbs(1.88056378952585, 1e-6));
                CHECK_THAT(std::arg(values(12, 3)), WithinAbs(-1.05826267843925, 1e-6));
                CHECK_THAT(std::arg(values(12, 4)), WithinAbs(-1.05826267843925, 1e-6));
                CHECK_THAT(std::arg(values(12, 5)), WithinAbs(1.88056378952585, 1e-6));
                CHECK_THAT(std::arg(values(12, 6)), WithinAbs(0.914750636729289, 1e-6));
                CHECK_THAT(std::arg(values(12, 7)), WithinAbs(-1.03214774294059, 1e-6));
                CHECK_THAT(std::arg(values(13, 0)), WithinAbs(-0.281782173001936, 1e-6));
                CHECK_THAT(std::arg(values(13, 1)), WithinAbs(-0.44585260614164, 1e-6));
                CHECK_THAT(std::arg(values(13, 2)), WithinAbs(-1.31038369616287, 1e-6));
                CHECK_THAT(std::arg(values(13, 3)), WithinAbs(-2.11076064133287, 1e-6));
                CHECK_THAT(std::arg(values(13, 4)), WithinAbs(-2.11076064133287, 1e-6));
                CHECK_THAT(std::arg(values(13, 5)), WithinAbs(-1.31038369616287, 1e-6));
                CHECK_THAT(std::arg(values(13, 6)), WithinAbs(-0.44585260614164, 1e-6));
                CHECK_THAT(std::arg(values(13, 7)), WithinAbs(-0.281782173001923, 1e-6));
                CHECK_THAT(std::arg(values(14, 0)), WithinAbs(1.13662676122023, 1e-6));
                CHECK_THAT(std::arg(values(14, 1)), WithinAbs(3.04174809369909, 1e-6));
                CHECK_THAT(std::arg(values(14, 2)), WithinAbs(0.408142059494028, 1e-6));
                CHECK_THAT(std::arg(values(14, 3)), WithinAbs(1.68124791287845, 1e-6));
                CHECK_THAT(std::arg(values(14, 4)), WithinAbs(1.68124791287845, 1e-6));
                CHECK_THAT(std::arg(values(14, 5)), WithinAbs(0.408142059494027, 1e-6));
                CHECK_THAT(std::arg(values(14, 6)), WithinAbs(3.04174809369858, 1e-6));
                CHECK_THAT(std::arg(values(14, 7)), WithinAbs(1.13662676122023, 1e-6));
                CHECK_THAT(std::arg(values(15, 0)), WithinAbs(1.5660297421716, 1e-6));
                CHECK_THAT(std::arg(values(15, 1)), WithinAbs(-1.02342533187269, 1e-6));
                CHECK_THAT(std::arg(values(15, 2)), WithinAbs(1.13933400716691, 1e-6));
                CHECK_THAT(std::arg(values(15, 3)), WithinAbs(-1.64573752094675, 1e-6));
                CHECK_THAT(std::arg(values(15, 4)), WithinAbs(-1.64573752094675, 1e-6));
                CHECK_THAT(std::arg(values(15, 5)), WithinAbs(1.1393340071669, 1e-6));
                CHECK_THAT(std::arg(values(15, 6)), WithinAbs(-1.02342533187256, 1e-6));
                CHECK_THAT(std::arg(values(15, 7)), WithinAbs(1.5660297421716, 1e-6));
            }

            // Verify page 2
            {
                auto& values = data[2];

                // Next, find the maximum of the physical values and normalize for better comparison
                double const abs_max = normalize(values);
                CHECK_THAT(abs_max, WithinAbs(0.03906500808528994, 1e-9));

                // Verify physical values along the surface
                CHECK_THAT(std::abs(values(0, 0)), WithinAbs(0.136351738833879, 1e-9));
                CHECK_THAT(std::abs(values(0, 1)), WithinAbs(0.0156554679377567, 1e-9));
                CHECK_THAT(std::abs(values(0, 2)), WithinAbs(0.397490031436339, 1e-9));
                CHECK_THAT(std::abs(values(0, 3)), WithinAbs(0.762783961753825, 1e-9));
                CHECK_THAT(std::abs(values(0, 4)), WithinAbs(0.762783961753825, 1e-9));
                CHECK_THAT(std::abs(values(0, 5)), WithinAbs(0.397490031436339, 1e-9));
                CHECK_THAT(std::abs(values(0, 6)), WithinAbs(0.0156554679377761, 1e-9));
                CHECK_THAT(std::abs(values(0, 7)), WithinAbs(0.13635173883388, 1e-9));
                CHECK_THAT(std::abs(values(1, 0)), WithinAbs(0.14857503965707, 1e-9));
                CHECK_THAT(std::abs(values(1, 1)), WithinAbs(0.0312673485306491, 1e-9));
                CHECK_THAT(std::abs(values(1, 2)), WithinAbs(0.376578411378417, 1e-9));
                CHECK_THAT(std::abs(values(1, 3)), WithinAbs(0.806714274219831, 1e-9));
                CHECK_THAT(std::abs(values(1, 4)), WithinAbs(0.806714274219831, 1e-9));
                CHECK_THAT(std::abs(values(1, 5)), WithinAbs(0.376578411378417, 1e-9));
                CHECK_THAT(std::abs(values(1, 6)), WithinAbs(0.0312673485306451, 1e-9));
                CHECK_THAT(std::abs(values(1, 7)), WithinAbs(0.14857503965707, 1e-9));
                CHECK_THAT(std::abs(values(2, 0)), WithinAbs(0.15070288882525, 1e-9));
                CHECK_THAT(std::abs(values(2, 1)), WithinAbs(0.0765947669904878, 1e-9));
                CHECK_THAT(std::abs(values(2, 2)), WithinAbs(0.348634440864513, 1e-9));
                CHECK_THAT(std::abs(values(2, 3)), WithinAbs(0.850848809701595, 1e-9));
                CHECK_THAT(std::abs(values(2, 4)), WithinAbs(0.850848809701595, 1e-9));
                CHECK_THAT(std::abs(values(2, 5)), WithinAbs(0.348634440864513, 1e-9));
                CHECK_THAT(std::abs(values(2, 6)), WithinAbs(0.0765947669904879, 1e-9));
                CHECK_THAT(std::abs(values(2, 7)), WithinAbs(0.15070288882525, 1e-9));
                CHECK_THAT(std::abs(values(3, 0)), WithinAbs(0.14295381470043, 1e-9));
                CHECK_THAT(std::abs(values(3, 1)), WithinAbs(0.117354855779035, 1e-9));
                CHECK_THAT(std::abs(values(3, 2)), WithinAbs(0.314840404215442, 1e-9));
                CHECK_THAT(std::abs(values(3, 3)), WithinAbs(0.893433711187075, 1e-9));
                CHECK_THAT(std::abs(values(3, 4)), WithinAbs(0.893433711187075, 1e-9));
                CHECK_THAT(std::abs(values(3, 5)), WithinAbs(0.314840404215442, 1e-9));
                CHECK_THAT(std::abs(values(3, 6)), WithinAbs(0.117354855779031, 1e-9));
                CHECK_THAT(std::abs(values(3, 7)), WithinAbs(0.14295381470043, 1e-9));
                CHECK_THAT(std::abs(values(4, 0)), WithinAbs(0.127883027223684, 1e-9));
                CHECK_THAT(std::abs(values(4, 1)), WithinAbs(0.150260701398678, 1e-9));
                CHECK_THAT(std::abs(values(4, 2)), WithinAbs(0.278065746582953, 1e-9));
                CHECK_THAT(std::abs(values(4, 3)), WithinAbs(0.932188646845362, 1e-9));
                CHECK_THAT(std::abs(values(4, 4)), WithinAbs(0.932188646845362, 1e-9));
                CHECK_THAT(std::abs(values(4, 5)), WithinAbs(0.278065746582953, 1e-9));
                CHECK_THAT(std::abs(values(4, 6)), WithinAbs(0.150260701398678, 1e-9));
                CHECK_THAT(std::abs(values(4, 7)), WithinAbs(0.127883027223684, 1e-9));
                CHECK_THAT(std::abs(values(5, 0)), WithinAbs(0.110001257985308, 1e-9));
                CHECK_THAT(std::abs(values(5, 1)), WithinAbs(0.173533460612235, 1e-9));
                CHECK_THAT(std::abs(values(5, 2)), WithinAbs(0.242902564630109, 1e-9));
                CHECK_THAT(std::abs(values(5, 3)), WithinAbs(0.964497645940459, 1e-9));
                CHECK_THAT(std::abs(values(5, 4)), WithinAbs(0.964497645940459, 1e-9));
                CHECK_THAT(std::abs(values(5, 5)), WithinAbs(0.242902564630109, 1e-9));
                CHECK_THAT(std::abs(values(5, 6)), WithinAbs(0.173533460612229, 1e-9));
                CHECK_THAT(std::abs(values(5, 7)), WithinAbs(0.110001257985308, 1e-9));
                CHECK_THAT(std::abs(values(6, 0)), WithinAbs(0.094473506141054, 1e-9));
                CHECK_THAT(std::abs(values(6, 1)), WithinAbs(0.187426546889668, 1e-9));
                CHECK_THAT(std::abs(values(6, 2)), WithinAbs(0.214959331864622, 1e-9));
                CHECK_THAT(std::abs(values(6, 3)), WithinAbs(0.987787141496915, 1e-9));
                CHECK_THAT(std::abs(values(6, 4)), WithinAbs(0.987787141496915, 1e-9));
                CHECK_THAT(std::abs(values(6, 5)), WithinAbs(0.214959331864622, 1e-9));
                CHECK_THAT(std::abs(values(6, 6)), WithinAbs(0.187426546889668, 1e-9));
                CHECK_THAT(std::abs(values(6, 7)), WithinAbs(0.094473506141054, 1e-9));
                CHECK_THAT(std::abs(values(7, 0)), WithinAbs(0.0855555739512042, 1e-9));
                CHECK_THAT(std::abs(values(7, 1)), WithinAbs(0.193613808063871, 1e-9));
                CHECK_THAT(std::abs(values(7, 2)), WithinAbs(0.199423301961654, 1e-9));
                CHECK_THAT(std::abs(values(7, 3)), WithinAbs(1, 1e-9));
                CHECK_THAT(std::abs(values(7, 4)), WithinAbs(1, 1e-9));
                CHECK_THAT(std::abs(values(7, 5)), WithinAbs(0.199423301961654, 1e-9));
                CHECK_THAT(std::abs(values(7, 6)), WithinAbs(0.193613808063838, 1e-9));
                CHECK_THAT(std::abs(values(7, 7)), WithinAbs(0.0855555739512041, 1e-9));
                CHECK_THAT(std::abs(values(8, 0)), WithinAbs(0.0855555739512042, 1e-9));
                CHECK_THAT(std::abs(values(8, 1)), WithinAbs(0.193613808063871, 1e-9));
                CHECK_THAT(std::abs(values(8, 2)), WithinAbs(0.199423301961654, 1e-9));
                CHECK_THAT(std::abs(values(8, 3)), WithinAbs(1, 1e-9));
                CHECK_THAT(std::abs(values(8, 4)), WithinAbs(1, 1e-9));
                CHECK_THAT(std::abs(values(8, 5)), WithinAbs(0.199423301961654, 1e-9));
                CHECK_THAT(std::abs(values(8, 6)), WithinAbs(0.193613808063838, 1e-9));
                CHECK_THAT(std::abs(values(8, 7)), WithinAbs(0.0855555739512041, 1e-9));
                CHECK_THAT(std::abs(values(9, 0)), WithinAbs(0.094473506141054, 1e-9));
                CHECK_THAT(std::abs(values(9, 1)), WithinAbs(0.187426546889668, 1e-9));
                CHECK_THAT(std::abs(values(9, 2)), WithinAbs(0.214959331864622, 1e-9));
                CHECK_THAT(std::abs(values(9, 3)), WithinAbs(0.987787141496915, 1e-9));
                CHECK_THAT(std::abs(values(9, 4)), WithinAbs(0.987787141496915, 1e-9));
                CHECK_THAT(std::abs(values(9, 5)), WithinAbs(0.214959331864622, 1e-9));
                CHECK_THAT(std::abs(values(9, 6)), WithinAbs(0.187426546889668, 1e-9));
                CHECK_THAT(std::abs(values(9, 7)), WithinAbs(0.094473506141054, 1e-9));
                CHECK_THAT(std::abs(values(10, 0)), WithinAbs(0.110001257985308, 1e-9));
                CHECK_THAT(std::abs(values(10, 1)), WithinAbs(0.173533460612235, 1e-9));
                CHECK_THAT(std::abs(values(10, 2)), WithinAbs(0.242902564630109, 1e-9));
                CHECK_THAT(std::abs(values(10, 3)), WithinAbs(0.964497645940459, 1e-9));
                CHECK_THAT(std::abs(values(10, 4)), WithinAbs(0.964497645940459, 1e-9));
                CHECK_THAT(std::abs(values(10, 5)), WithinAbs(0.242902564630109, 1e-9));
                CHECK_THAT(std::abs(values(10, 6)), WithinAbs(0.173533460612229, 1e-9));
                CHECK_THAT(std::abs(values(10, 7)), WithinAbs(0.110001257985308, 1e-9));
                CHECK_THAT(std::abs(values(11, 0)), WithinAbs(0.127883027223684, 1e-9));
                CHECK_THAT(std::abs(values(11, 1)), WithinAbs(0.150260701398678, 1e-9));
                CHECK_THAT(std::abs(values(11, 2)), WithinAbs(0.278065746582953, 1e-9));
                CHECK_THAT(std::abs(values(11, 3)), WithinAbs(0.932188646845362, 1e-9));
                CHECK_THAT(std::abs(values(11, 4)), WithinAbs(0.932188646845362, 1e-9));
                CHECK_THAT(std::abs(values(11, 5)), WithinAbs(0.278065746582953, 1e-9));
                CHECK_THAT(std::abs(values(11, 6)), WithinAbs(0.150260701398678, 1e-9));
                CHECK_THAT(std::abs(values(11, 7)), WithinAbs(0.127883027223684, 1e-9));
                CHECK_THAT(std::abs(values(12, 0)), WithinAbs(0.142953814700409, 1e-9));
                CHECK_THAT(std::abs(values(12, 1)), WithinAbs(0.117354855779051, 1e-9));
                CHECK_THAT(std::abs(values(12, 2)), WithinAbs(0.314840404215458, 1e-9));
                CHECK_THAT(std::abs(values(12, 3)), WithinAbs(0.893433711186985, 1e-9));
                CHECK_THAT(std::abs(values(12, 4)), WithinAbs(0.893433711186985, 1e-9));
                CHECK_THAT(std::abs(values(12, 5)), WithinAbs(0.314840404215458, 1e-9));
                CHECK_THAT(std::abs(values(12, 6)), WithinAbs(0.117354855779004, 1e-9));
                CHECK_THAT(std::abs(values(12, 7)), WithinAbs(0.142953814700408, 1e-9));
                CHECK_THAT(std::abs(values(13, 0)), WithinAbs(0.15070288882525, 1e-9));
                CHECK_THAT(std::abs(values(13, 1)), WithinAbs(0.0765947669904878, 1e-9));
                CHECK_THAT(std::abs(values(13, 2)), WithinAbs(0.348634440864513, 1e-9));
                CHECK_THAT(std::abs(values(13, 3)), WithinAbs(0.850848809701595, 1e-9));
                CHECK_THAT(std::abs(values(13, 4)), WithinAbs(0.850848809701595, 1e-9));
                CHECK_THAT(std::abs(values(13, 5)), WithinAbs(0.348634440864513, 1e-9));
                CHECK_THAT(std::abs(values(13, 6)), WithinAbs(0.0765947669904879, 1e-9));
                CHECK_THAT(std::abs(values(13, 7)), WithinAbs(0.15070288882525, 1e-9));
                CHECK_THAT(std::abs(values(14, 0)), WithinAbs(0.14857503965707, 1e-9));
                CHECK_THAT(std::abs(values(14, 1)), WithinAbs(0.0312673485306491, 1e-9));
                CHECK_THAT(std::abs(values(14, 2)), WithinAbs(0.376578411378417, 1e-9));
                CHECK_THAT(std::abs(values(14, 3)), WithinAbs(0.806714274219831, 1e-9));
                CHECK_THAT(std::abs(values(14, 4)), WithinAbs(0.806714274219831, 1e-9));
                CHECK_THAT(std::abs(values(14, 5)), WithinAbs(0.376578411378417, 1e-9));
                CHECK_THAT(std::abs(values(14, 6)), WithinAbs(0.0312673485306451, 1e-9));
                CHECK_THAT(std::abs(values(14, 7)), WithinAbs(0.14857503965707, 1e-9));
                CHECK_THAT(std::abs(values(15, 0)), WithinAbs(0.136351738833879, 1e-9));
                CHECK_THAT(std::abs(values(15, 1)), WithinAbs(0.0156554679377567, 1e-9));
                CHECK_THAT(std::abs(values(15, 2)), WithinAbs(0.397490031436339, 1e-9));
                CHECK_THAT(std::abs(values(15, 3)), WithinAbs(0.762783961753825, 1e-9));
                CHECK_THAT(std::abs(values(15, 4)), WithinAbs(0.762783961753825, 1e-9));
                CHECK_THAT(std::abs(values(15, 5)), WithinAbs(0.397490031436339, 1e-9));
                CHECK_THAT(std::abs(values(15, 6)), WithinAbs(0.0156554679377761, 1e-9));
                CHECK_THAT(std::abs(values(15, 7)), WithinAbs(0.13635173883388, 1e-9));
                CHECK_THAT(std::arg(values(0, 0)), WithinAbs(1.55239300789175, 1e-6));
                CHECK_THAT(std::arg(values(0, 1)), WithinAbs(2.97398471370171, 1e-6));
                CHECK_THAT(std::arg(values(0, 2)), WithinAbs(-2.68954196355744, 1e-6));
                CHECK_THAT(std::arg(values(0, 3)), WithinAbs(-1.62730398817709, 1e-6));
                CHECK_THAT(std::arg(values(0, 4)), WithinAbs(-1.62730398817709, 1e-6));
                CHECK_THAT(std::arg(values(0, 5)), WithinAbs(-2.68954196355744, 1e-6));
                CHECK_THAT(std::arg(values(0, 6)), WithinAbs(2.97398471370042, 1e-6));
                CHECK_THAT(std::arg(values(0, 7)), WithinAbs(1.55239300789175, 1e-6));
                CHECK_THAT(std::arg(values(1, 0)), WithinAbs(-0.350198689109576, 1e-6));
                CHECK_THAT(std::arg(values(1, 1)), WithinAbs(-0.561001726224911, 1e-6));
                CHECK_THAT(std::arg(values(1, 2)), WithinAbs(3.03807656147935, 1e-6));
                CHECK_THAT(std::arg(values(1, 3)), WithinAbs(0.867862641187321, 1e-6));
                CHECK_THAT(std::arg(values(1, 4)), WithinAbs(0.867862641187321, 1e-6));
                CHECK_THAT(std::arg(values(1, 5)), WithinAbs(3.03807656147935, 1e-6));
                CHECK_THAT(std::arg(values(1, 6)), WithinAbs(-0.561001726226291, 1e-6));
                CHECK_THAT(std::arg(values(1, 7)), WithinAbs(-0.350198689109575, 1e-6));
                CHECK_THAT(std::arg(values(2, 0)), WithinAbs(-1.83005923463768, 1e-6));
                CHECK_THAT(std::arg(values(2, 1)), WithinAbs(0.0220812220548482, 1e-6));
                CHECK_THAT(std::arg(values(2, 2)), WithinAbs(-2.98127085760698, 1e-6));
                CHECK_THAT(std::arg(values(2, 3)), WithinAbs(-1.97623246327732, 1e-6));
                CHECK_THAT(std::arg(values(2, 4)), WithinAbs(-1.97623246327732, 1e-6));
                CHECK_THAT(std::arg(values(2, 5)), WithinAbs(-2.98127085760698, 1e-6));
                CHECK_THAT(std::arg(values(2, 6)), WithinAbs(0.0220812220548478, 1e-6));
                CHECK_THAT(std::arg(values(2, 7)), WithinAbs(-1.83005923463768, 1e-6));
                CHECK_THAT(std::arg(values(3, 0)), WithinAbs(-1.13562679905633, 1e-6));
                CHECK_THAT(std::arg(values(3, 1)), WithinAbs(2.62588304315354, 1e-6));
                CHECK_THAT(std::arg(values(3, 2)), WithinAbs(-0.667604247344914, 1e-6));
                CHECK_THAT(std::arg(values(3, 3)), WithinAbs(-2.75775985524043, 1e-6));
                CHECK_THAT(std::arg(values(3, 4)), WithinAbs(-2.75775985524043, 1e-6));
                CHECK_THAT(std::arg(values(3, 5)), WithinAbs(-0.667604247344914, 1e-6));
                CHECK_THAT(std::arg(values(3, 6)), WithinAbs(2.62588304315334, 1e-6));
                CHECK_THAT(std::arg(values(3, 7)), WithinAbs(-1.13562679905633, 1e-6));
                CHECK_THAT(std::arg(values(4, 0)), WithinAbs(3.07351007973182, 1e-6));
                CHECK_THAT(std::arg(values(4, 1)), WithinAbs(1.90631943349166, 1e-6));
                CHECK_THAT(std::arg(values(4, 2)), WithinAbs(-2.0089493832073, 1e-6));
                CHECK_THAT(std::arg(values(4, 3)), WithinAbs(-1.07145356107594, 1e-6));
                CHECK_THAT(std::arg(values(4, 4)), WithinAbs(-1.07145356107594, 1e-6));
                CHECK_THAT(std::arg(values(4, 5)), WithinAbs(-2.0089493832073, 1e-6));
                CHECK_THAT(std::arg(values(4, 6)), WithinAbs(1.90631943349166, 1e-6));
                CHECK_THAT(std::arg(values(4, 7)), WithinAbs(3.07351007973182, 1e-6));
                CHECK_THAT(std::arg(values(5, 0)), WithinAbs(-0.529553924296922, 1e-6));
                CHECK_THAT(std::arg(values(5, 1)), WithinAbs(-1.44822308307812, 1e-6));
                CHECK_THAT(std::arg(values(5, 2)), WithinAbs(-0.467093517661823, 1e-6));
                CHECK_THAT(std::arg(values(5, 3)), WithinAbs(3.10375378688958, 1e-6));
                CHECK_THAT(std::arg(values(5, 4)), WithinAbs(3.10375378688958, 1e-6));
                CHECK_THAT(std::arg(values(5, 5)), WithinAbs(-0.467093517661823, 1e-6));
                CHECK_THAT(std::arg(values(5, 6)), WithinAbs(-1.44822308307826, 1e-6));
                CHECK_THAT(std::arg(values(5, 7)), WithinAbs(-0.529553924296922, 1e-6));
                CHECK_THAT(std::arg(values(6, 0)), WithinAbs(2.25392287848855, 1e-6));
                CHECK_THAT(std::arg(values(6, 1)), WithinAbs(-0.129160285081671, 1e-6));
                CHECK_THAT(std::arg(values(6, 2)), WithinAbs(-1.78515010878263, 1e-6));
                CHECK_THAT(std::arg(values(6, 3)), WithinAbs(-2.52747702795532, 1e-6));
                CHECK_THAT(std::arg(values(6, 4)), WithinAbs(-2.52747702795532, 1e-6));
                CHECK_THAT(std::arg(values(6, 5)), WithinAbs(-1.78515010878263, 1e-6));
                CHECK_THAT(std::arg(values(6, 6)), WithinAbs(-0.129160285081671, 1e-6));
                CHECK_THAT(std::arg(values(6, 7)), WithinAbs(2.25392287848855, 1e-6));
                CHECK_THAT(std::arg(values(7, 0)), WithinAbs(1.50788027264301, 1e-6));
                CHECK_THAT(std::arg(values(7, 1)), WithinAbs(1.69386996651941, 1e-6));
                CHECK_THAT(std::arg(values(7, 2)), WithinAbs(1.99409812030865, 1e-6));
                CHECK_THAT(std::arg(values(7, 3)), WithinAbs(2.3118192521272, 1e-6));
                CHECK_THAT(std::arg(values(7, 4)), WithinAbs(2.3118192521272, 1e-6));
                CHECK_THAT(std::arg(values(7, 5)), WithinAbs(1.99409812030865, 1e-6));
                CHECK_THAT(std::arg(values(7, 6)), WithinAbs(1.69386996651957, 1e-6));
                CHECK_THAT(std::arg(values(7, 7)), WithinAbs(1.50788027264301, 1e-6));
                CHECK_THAT(std::arg(values(8, 0)), WithinAbs(1.50788027264301, 1e-6));
                CHECK_THAT(std::arg(values(8, 1)), WithinAbs(1.69386996651941, 1e-6));
                CHECK_THAT(std::arg(values(8, 2)), WithinAbs(1.99409812030865, 1e-6));
                CHECK_THAT(std::arg(values(8, 3)), WithinAbs(2.3118192521272, 1e-6));
                CHECK_THAT(std::arg(values(8, 4)), WithinAbs(2.3118192521272, 1e-6));
                CHECK_THAT(std::arg(values(8, 5)), WithinAbs(1.99409812030865, 1e-6));
                CHECK_THAT(std::arg(values(8, 6)), WithinAbs(1.69386996651957, 1e-6));
                CHECK_THAT(std::arg(values(8, 7)), WithinAbs(1.50788027264301, 1e-6));
                CHECK_THAT(std::arg(values(9, 0)), WithinAbs(2.25392287848855, 1e-6));
                CHECK_THAT(std::arg(values(9, 1)), WithinAbs(-0.129160285081671, 1e-6));
                CHECK_THAT(std::arg(values(9, 2)), WithinAbs(-1.78515010878263, 1e-6));
                CHECK_THAT(std::arg(values(9, 3)), WithinAbs(-2.52747702795532, 1e-6));
                CHECK_THAT(std::arg(values(9, 4)), WithinAbs(-2.52747702795532, 1e-6));
                CHECK_THAT(std::arg(values(9, 5)), WithinAbs(-1.78515010878263, 1e-6));
                CHECK_THAT(std::arg(values(9, 6)), WithinAbs(-0.129160285081671, 1e-6));
                CHECK_THAT(std::arg(values(9, 7)), WithinAbs(2.25392287848855, 1e-6));
                CHECK_THAT(std::arg(values(10, 0)), WithinAbs(-0.529553924296922, 1e-6));
                CHECK_THAT(std::arg(values(10, 1)), WithinAbs(-1.44822308307812, 1e-6));
                CHECK_THAT(std::arg(values(10, 2)), WithinAbs(-0.467093517661823, 1e-6));
                CHECK_THAT(std::arg(values(10, 3)), WithinAbs(3.10375378688958, 1e-6));
                CHECK_THAT(std::arg(values(10, 4)), WithinAbs(3.10375378688958, 1e-6));
                CHECK_THAT(std::arg(values(10, 5)), WithinAbs(-0.467093517661823, 1e-6));
                CHECK_THAT(std::arg(values(10, 6)), WithinAbs(-1.44822308307826, 1e-6));
                CHECK_THAT(std::arg(values(10, 7)), WithinAbs(-0.529553924296922, 1e-6));
                CHECK_THAT(std::arg(values(11, 0)), WithinAbs(3.07351007973182, 1e-6));
                CHECK_THAT(std::arg(values(11, 1)), WithinAbs(1.90631943349166, 1e-6));
                CHECK_THAT(std::arg(values(11, 2)), WithinAbs(-2.0089493832073, 1e-6));
                CHECK_THAT(std::arg(values(11, 3)), WithinAbs(-1.07145356107594, 1e-6));
                CHECK_THAT(std::arg(values(11, 4)), WithinAbs(-1.07145356107594, 1e-6));
                CHECK_THAT(std::arg(values(11, 5)), WithinAbs(-2.0089493832073, 1e-6));
                CHECK_THAT(std::arg(values(11, 6)), WithinAbs(1.90631943349166, 1e-6));
                CHECK_THAT(std::arg(values(11, 7)), WithinAbs(3.07351007973182, 1e-6));
                CHECK_THAT(std::arg(values(12, 0)), WithinAbs(-1.13562679905683, 1e-6));
                CHECK_THAT(std::arg(values(12, 1)), WithinAbs(2.62588304315341, 1e-6));
                CHECK_THAT(std::arg(values(12, 2)), WithinAbs(-0.667604247345041, 1e-6));
                CHECK_THAT(std::arg(values(12, 3)), WithinAbs(-2.75775985524077, 1e-6));
                CHECK_THAT(std::arg(values(12, 4)), WithinAbs(-2.75775985524077, 1e-6));
                CHECK_THAT(std::arg(values(12, 5)), WithinAbs(-0.667604247345041, 1e-6));
                CHECK_THAT(std::arg(values(12, 6)), WithinAbs(2.62588304315341, 1e-6));
                CHECK_THAT(std::arg(values(12, 7)), WithinAbs(-1.13562679905683, 1e-6));
                CHECK_THAT(std::arg(values(13, 0)), WithinAbs(-1.83005923463768, 1e-6));
                CHECK_THAT(std::arg(values(13, 1)), WithinAbs(0.0220812220548485, 1e-6));
                CHECK_THAT(std::arg(values(13, 2)), WithinAbs(-2.98127085760698, 1e-6));
                CHECK_THAT(std::arg(values(13, 3)), WithinAbs(-1.97623246327732, 1e-6));
                CHECK_THAT(std::arg(values(13, 4)), WithinAbs(-1.97623246327732, 1e-6));
                CHECK_THAT(std::arg(values(13, 5)), WithinAbs(-2.98127085760698, 1e-6));
                CHECK_THAT(std::arg(values(13, 6)), WithinAbs(0.0220812220548478, 1e-6));
                CHECK_THAT(std::arg(values(13, 7)), WithinAbs(-1.83005923463768, 1e-6));
                CHECK_THAT(std::arg(values(14, 0)), WithinAbs(-0.350198689109576, 1e-6));
                CHECK_THAT(std::arg(values(14, 1)), WithinAbs(-0.56100172622491, 1e-6));
                CHECK_THAT(std::arg(values(14, 2)), WithinAbs(3.03807656147935, 1e-6));
                CHECK_THAT(std::arg(values(14, 3)), WithinAbs(0.867862641187321, 1e-6));
                CHECK_THAT(std::arg(values(14, 4)), WithinAbs(0.867862641187321, 1e-6));
                CHECK_THAT(std::arg(values(14, 5)), WithinAbs(3.03807656147935, 1e-6));
                CHECK_THAT(std::arg(values(14, 6)), WithinAbs(-0.561001726226293, 1e-6));
                CHECK_THAT(std::arg(values(14, 7)), WithinAbs(-0.350198689109575, 1e-6));
                CHECK_THAT(std::arg(values(15, 0)), WithinAbs(1.55239300789175, 1e-6));
                CHECK_THAT(std::arg(values(15, 1)), WithinAbs(2.97398471370171, 1e-6));
                CHECK_THAT(std::arg(values(15, 2)), WithinAbs(-2.68954196355744, 1e-6));
                CHECK_THAT(std::arg(values(15, 3)), WithinAbs(-1.62730398817709, 1e-6));
                CHECK_THAT(std::arg(values(15, 4)), WithinAbs(-1.62730398817709, 1e-6));
                CHECK_THAT(std::arg(values(15, 5)), WithinAbs(-2.68954196355744, 1e-6));
                CHECK_THAT(std::arg(values(15, 6)), WithinAbs(2.97398471370042, 1e-6));
                CHECK_THAT(std::arg(values(15, 7)), WithinAbs(1.55239300789175, 1e-6));
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
            CHECK_THAT(positions(0, 0).x, WithinAbs(65.32814824381881635, 1e-9));
            CHECK_THAT(positions(0, 0).y, WithinAbs(65.32814824381883057, 1e-9));
            CHECK_THAT(positions(0, 0).z, WithinAbs(-38.26834323650897574, 1e-9));
            CHECK_THAT(positions(positions.shape().rows - 1, 0).x, WithinAbs(-65.32814824381881635, 1e-9));
            CHECK_THAT(positions(positions.shape().rows - 1, 0).y, WithinAbs(65.32814824381883057, 1e-9));
            CHECK_THAT(positions(positions.shape().rows - 1, 0).z, WithinAbs(-38.26834323650897574, 1e-9));
            CHECK_THAT(positions(0, positions.shape().cols - 1).x, WithinAbs(65.32814824381881635, 1e-9));
            CHECK_THAT(positions(0, positions.shape().cols - 1).y, WithinAbs(65.32814824381883057, 1e-9));
            CHECK_THAT(positions(0, positions.shape().cols - 1).z, WithinAbs(38.26834323650897574, 1e-9));
            CHECK_THAT(positions(positions.shape().rows - 1, positions.shape().cols - 1).x, WithinAbs(-65.32814824381881635, 1e-9));
            CHECK_THAT(positions(positions.shape().rows - 1, positions.shape().cols - 1).y, WithinAbs(65.32814824381883057, 1e-9));
            CHECK_THAT(positions(positions.shape().rows - 1, positions.shape().cols - 1).z, WithinAbs(38.26834323650897574, 1e-9));

            // Next, find the maximum of the physical values and normalize for better comparison
            double const abs_max = normalize(values);
            CHECK_THAT(abs_max, WithinAbs(0.0056509419038846, 1e-9));

            // Verify physical values along the surface
            CHECK_THAT(std::abs(values(0, 0)), WithinAbs(0.0161323981305048, 1e-9));
            CHECK_THAT(std::abs(values(0, 1)), WithinAbs(0.0966088241309794, 1e-9));
            CHECK_THAT(std::abs(values(0, 2)), WithinAbs(0.270909628866502, 1e-9));
            CHECK_THAT(std::abs(values(0, 3)), WithinAbs(0.999999999999391, 1e-9));
            CHECK_THAT(std::abs(values(0, 4)), WithinAbs(0.999999999999391, 1e-9));
            CHECK_THAT(std::abs(values(0, 5)), WithinAbs(0.270909628866501, 1e-9));
            CHECK_THAT(std::abs(values(0, 6)), WithinAbs(0.0966088241310958, 1e-9));
            CHECK_THAT(std::abs(values(0, 7)), WithinAbs(0.0161323981305049, 1e-9));
            CHECK_THAT(std::abs(values(1, 0)), WithinAbs(0.0161323981305376, 1e-9));
            CHECK_THAT(std::abs(values(1, 1)), WithinAbs(0.0966088241308526, 1e-9));
            CHECK_THAT(std::abs(values(1, 2)), WithinAbs(0.270909628866573, 1e-9));
            CHECK_THAT(std::abs(values(1, 3)), WithinAbs(0.999999999999534, 1e-9));
            CHECK_THAT(std::abs(values(1, 4)), WithinAbs(0.999999999999555, 1e-9));
            CHECK_THAT(std::abs(values(1, 5)), WithinAbs(0.270909628866573, 1e-9));
            CHECK_THAT(std::abs(values(1, 6)), WithinAbs(0.0966088241308649, 1e-9));
            CHECK_THAT(std::abs(values(1, 7)), WithinAbs(0.0161323981305376, 1e-9));
            CHECK_THAT(std::abs(values(2, 0)), WithinAbs(0.0161323981306128, 1e-9));
            CHECK_THAT(std::abs(values(2, 1)), WithinAbs(0.0966088241309214, 1e-9));
            CHECK_THAT(std::abs(values(2, 2)), WithinAbs(0.270909628866805, 1e-9));
            CHECK_THAT(std::abs(values(2, 3)), WithinAbs(0.999999999999486, 1e-9));
            CHECK_THAT(std::abs(values(2, 4)), WithinAbs(0.999999999999487, 1e-9));
            CHECK_THAT(std::abs(values(2, 5)), WithinAbs(0.270909628866805, 1e-9));
            CHECK_THAT(std::abs(values(2, 6)), WithinAbs(0.0966088241309214, 1e-9));
            CHECK_THAT(std::abs(values(2, 7)), WithinAbs(0.0161323981306127, 1e-9));
            CHECK_THAT(std::abs(values(3, 0)), WithinAbs(0.0161323981306627, 1e-9));
            CHECK_THAT(std::abs(values(3, 1)), WithinAbs(0.0966088241310159, 1e-9));
            CHECK_THAT(std::abs(values(3, 2)), WithinAbs(0.27090962886659, 1e-9));
            CHECK_THAT(std::abs(values(3, 3)), WithinAbs(0.999999999999756, 1e-9));
            CHECK_THAT(std::abs(values(3, 4)), WithinAbs(0.999999999999756, 1e-9));
            CHECK_THAT(std::abs(values(3, 5)), WithinAbs(0.27090962886659, 1e-9));
            CHECK_THAT(std::abs(values(3, 6)), WithinAbs(0.096608824131016, 1e-9));
            CHECK_THAT(std::abs(values(3, 7)), WithinAbs(0.0161323981306626, 1e-9));
            CHECK_THAT(std::abs(values(4, 0)), WithinAbs(0.0161323981305155, 1e-9));
            CHECK_THAT(std::abs(values(4, 1)), WithinAbs(0.0966088241308565, 1e-9));
            CHECK_THAT(std::abs(values(4, 2)), WithinAbs(0.270909628866603, 1e-9));
            CHECK_THAT(std::abs(values(4, 3)), WithinAbs(0.999999999999401, 1e-9));
            CHECK_THAT(std::abs(values(4, 4)), WithinAbs(0.999999999999401, 1e-9));
            CHECK_THAT(std::abs(values(4, 5)), WithinAbs(0.270909628866603, 1e-9));
            CHECK_THAT(std::abs(values(4, 6)), WithinAbs(0.0966088241308739, 1e-9));
            CHECK_THAT(std::abs(values(4, 7)), WithinAbs(0.0161323981305156, 1e-9));
            CHECK_THAT(std::abs(values(5, 0)), WithinAbs(0.0161323981307005, 1e-9));
            CHECK_THAT(std::abs(values(5, 1)), WithinAbs(0.0966088241308201, 1e-9));
            CHECK_THAT(std::abs(values(5, 2)), WithinAbs(0.270909628866586, 1e-9));
            CHECK_THAT(std::abs(values(5, 3)), WithinAbs(0.99999999999957, 1e-9));
            CHECK_THAT(std::abs(values(5, 4)), WithinAbs(0.99999999999957, 1e-9));
            CHECK_THAT(std::abs(values(5, 5)), WithinAbs(0.270909628866585, 1e-9));
            CHECK_THAT(std::abs(values(5, 6)), WithinAbs(0.0966088241308295, 1e-9));
            CHECK_THAT(std::abs(values(5, 7)), WithinAbs(0.0161323981307005, 1e-9));
            CHECK_THAT(std::abs(values(6, 0)), WithinAbs(0.0161323981305657, 1e-9));
            CHECK_THAT(std::abs(values(6, 1)), WithinAbs(0.0966088241307718, 1e-9));
            CHECK_THAT(std::abs(values(6, 2)), WithinAbs(0.270909628866542, 1e-9));
            CHECK_THAT(std::abs(values(6, 3)), WithinAbs(0.999999999999859, 1e-9));
            CHECK_THAT(std::abs(values(6, 4)), WithinAbs(1, 1e-9));
            CHECK_THAT(std::abs(values(6, 5)), WithinAbs(0.270909628866541, 1e-9));
            CHECK_THAT(std::abs(values(6, 6)), WithinAbs(0.0966088241308371, 1e-9));
            CHECK_THAT(std::abs(values(6, 7)), WithinAbs(0.0161323981305657, 1e-9));
            CHECK_THAT(std::abs(values(7, 0)), WithinAbs(0.0161323981305536, 1e-9));
            CHECK_THAT(std::abs(values(7, 1)), WithinAbs(0.0966088241310956, 1e-9));
            CHECK_THAT(std::abs(values(7, 2)), WithinAbs(0.270909628866735, 1e-9));
            CHECK_THAT(std::abs(values(7, 3)), WithinAbs(0.999999999999692, 1e-9));
            CHECK_THAT(std::abs(values(7, 4)), WithinAbs(0.999999999999692, 1e-9));
            CHECK_THAT(std::abs(values(7, 5)), WithinAbs(0.270909628866735, 1e-9));
            CHECK_THAT(std::abs(values(7, 6)), WithinAbs(0.0966088241310184, 1e-9));
            CHECK_THAT(std::abs(values(7, 7)), WithinAbs(0.0161323981305537, 1e-9));
            CHECK_THAT(std::abs(values(8, 0)), WithinAbs(0.0161323981305536, 1e-9));
            CHECK_THAT(std::abs(values(8, 1)), WithinAbs(0.0966088241310956, 1e-9));
            CHECK_THAT(std::abs(values(8, 2)), WithinAbs(0.270909628866735, 1e-9));
            CHECK_THAT(std::abs(values(8, 3)), WithinAbs(0.999999999999692, 1e-9));
            CHECK_THAT(std::abs(values(8, 4)), WithinAbs(0.999999999999692, 1e-9));
            CHECK_THAT(std::abs(values(8, 5)), WithinAbs(0.270909628866735, 1e-9));
            CHECK_THAT(std::abs(values(8, 6)), WithinAbs(0.0966088241310184, 1e-9));
            CHECK_THAT(std::abs(values(8, 7)), WithinAbs(0.0161323981305537, 1e-9));
            CHECK_THAT(std::abs(values(9, 0)), WithinAbs(0.0161323981305657, 1e-9));
            CHECK_THAT(std::abs(values(9, 1)), WithinAbs(0.0966088241307718, 1e-9));
            CHECK_THAT(std::abs(values(9, 2)), WithinAbs(0.270909628866542, 1e-9));
            CHECK_THAT(std::abs(values(9, 3)), WithinAbs(0.999999999999859, 1e-9));
            CHECK_THAT(std::abs(values(9, 4)), WithinAbs(1, 1e-9));
            CHECK_THAT(std::abs(values(9, 5)), WithinAbs(0.270909628866542, 1e-9));
            CHECK_THAT(std::abs(values(9, 6)), WithinAbs(0.0966088241308371, 1e-9));
            CHECK_THAT(std::abs(values(9, 7)), WithinAbs(0.0161323981305657, 1e-9));
            CHECK_THAT(std::abs(values(10, 0)), WithinAbs(0.0161323981307004, 1e-9));
            CHECK_THAT(std::abs(values(10, 1)), WithinAbs(0.0966088241308201, 1e-9));
            CHECK_THAT(std::abs(values(10, 2)), WithinAbs(0.270909628866586, 1e-9));
            CHECK_THAT(std::abs(values(10, 3)), WithinAbs(0.99999999999957, 1e-9));
            CHECK_THAT(std::abs(values(10, 4)), WithinAbs(0.99999999999957, 1e-9));
            CHECK_THAT(std::abs(values(10, 5)), WithinAbs(0.270909628866585, 1e-9));
            CHECK_THAT(std::abs(values(10, 6)), WithinAbs(0.0966088241308295, 1e-9));
            CHECK_THAT(std::abs(values(10, 7)), WithinAbs(0.0161323981307004, 1e-9));
            CHECK_THAT(std::abs(values(11, 0)), WithinAbs(0.0161323981306158, 1e-9));
            CHECK_THAT(std::abs(values(11, 1)), WithinAbs(0.0966088241310356, 1e-9));
            CHECK_THAT(std::abs(values(11, 2)), WithinAbs(0.270909628866517, 1e-9));
            CHECK_THAT(std::abs(values(11, 3)), WithinAbs(0.999999999999496, 1e-9));
            CHECK_THAT(std::abs(values(11, 4)), WithinAbs(0.999999999999496, 1e-9));
            CHECK_THAT(std::abs(values(11, 5)), WithinAbs(0.270909628866517, 1e-9));
            CHECK_THAT(std::abs(values(11, 6)), WithinAbs(0.0966088241309495, 1e-9));
            CHECK_THAT(std::abs(values(11, 7)), WithinAbs(0.0161323981306157, 1e-9));
            CHECK_THAT(std::abs(values(12, 0)), WithinAbs(0.0161323981306627, 1e-9));
            CHECK_THAT(std::abs(values(12, 1)), WithinAbs(0.096608824131016, 1e-9));
            CHECK_THAT(std::abs(values(12, 2)), WithinAbs(0.27090962886659, 1e-9));
            CHECK_THAT(std::abs(values(12, 3)), WithinAbs(0.999999999999756, 1e-9));
            CHECK_THAT(std::abs(values(12, 4)), WithinAbs(0.999999999999756, 1e-9));
            CHECK_THAT(std::abs(values(12, 5)), WithinAbs(0.27090962886659, 1e-9));
            CHECK_THAT(std::abs(values(12, 6)), WithinAbs(0.096608824131016, 1e-9));
            CHECK_THAT(std::abs(values(12, 7)), WithinAbs(0.0161323981306626, 1e-9));
            CHECK_THAT(std::abs(values(13, 0)), WithinAbs(0.0161323981306128, 1e-9));
            CHECK_THAT(std::abs(values(13, 1)), WithinAbs(0.0966088241309214, 1e-9));
            CHECK_THAT(std::abs(values(13, 2)), WithinAbs(0.270909628866805, 1e-9));
            CHECK_THAT(std::abs(values(13, 3)), WithinAbs(0.999999999999486, 1e-9));
            CHECK_THAT(std::abs(values(13, 4)), WithinAbs(0.999999999999487, 1e-9));
            CHECK_THAT(std::abs(values(13, 5)), WithinAbs(0.270909628866805, 1e-9));
            CHECK_THAT(std::abs(values(13, 6)), WithinAbs(0.0966088241309214, 1e-9));
            CHECK_THAT(std::abs(values(13, 7)), WithinAbs(0.0161323981306128, 1e-9));
            CHECK_THAT(std::abs(values(14, 0)), WithinAbs(0.0161323981305375, 1e-9));
            CHECK_THAT(std::abs(values(14, 1)), WithinAbs(0.0966088241308526, 1e-9));
            CHECK_THAT(std::abs(values(14, 2)), WithinAbs(0.270909628866573, 1e-9));
            CHECK_THAT(std::abs(values(14, 3)), WithinAbs(0.999999999999534, 1e-9));
            CHECK_THAT(std::abs(values(14, 4)), WithinAbs(0.999999999999555, 1e-9));
            CHECK_THAT(std::abs(values(14, 5)), WithinAbs(0.270909628866572, 1e-9));
            CHECK_THAT(std::abs(values(14, 6)), WithinAbs(0.0966088241308648, 1e-9));
            CHECK_THAT(std::abs(values(14, 7)), WithinAbs(0.0161323981305376, 1e-9));
            CHECK_THAT(std::abs(values(15, 0)), WithinAbs(0.0161323981305049, 1e-9));
            CHECK_THAT(std::abs(values(15, 1)), WithinAbs(0.0966088241309794, 1e-9));
            CHECK_THAT(std::abs(values(15, 2)), WithinAbs(0.270909628866502, 1e-9));
            CHECK_THAT(std::abs(values(15, 3)), WithinAbs(0.999999999999391, 1e-9));
            CHECK_THAT(std::abs(values(15, 4)), WithinAbs(0.999999999999391, 1e-9));
            CHECK_THAT(std::abs(values(15, 5)), WithinAbs(0.270909628866501, 1e-9));
            CHECK_THAT(std::abs(values(15, 6)), WithinAbs(0.0966088241310958, 1e-9));
            CHECK_THAT(std::abs(values(15, 7)), WithinAbs(0.0161323981305048, 1e-9));
            CHECK_THAT(std::arg(values(0, 0)), WithinAbs(1.41879122381658, 1e-6));
            CHECK_THAT(std::arg(values(0, 1)), WithinAbs(-1.64229502822274, 1e-6));
            CHECK_THAT(std::arg(values(0, 2)), WithinAbs(1.51299993708328, 1e-6));
            CHECK_THAT(std::arg(values(0, 3)), WithinAbs(-1.58094085237358, 1e-6));
            CHECK_THAT(std::arg(values(0, 4)), WithinAbs(-1.58094085237358, 1e-6));
            CHECK_THAT(std::arg(values(0, 5)), WithinAbs(1.51299993708328, 1e-6));
            CHECK_THAT(std::arg(values(0, 6)), WithinAbs(-1.64229502822321, 1e-6));
            CHECK_THAT(std::arg(values(0, 7)), WithinAbs(1.41879122381658, 1e-6));
            CHECK_THAT(std::arg(values(1, 0)), WithinAbs(1.41879122381387, 1e-6));
            CHECK_THAT(std::arg(values(1, 1)), WithinAbs(-1.64229502822221, 1e-6));
            CHECK_THAT(std::arg(values(1, 2)), WithinAbs(1.51299993708435, 1e-6));
            CHECK_THAT(std::arg(values(1, 3)), WithinAbs(-1.58094085237376, 1e-6));
            CHECK_THAT(std::arg(values(1, 4)), WithinAbs(-1.58094085237368, 1e-6));
            CHECK_THAT(std::arg(values(1, 5)), WithinAbs(1.51299993708435, 1e-6));
            CHECK_THAT(std::arg(values(1, 6)), WithinAbs(-1.64229502822161, 1e-6));
            CHECK_THAT(std::arg(values(1, 7)), WithinAbs(1.41879122381387, 1e-6));
            CHECK_THAT(std::arg(values(2, 0)), WithinAbs(1.41879122381051, 1e-6));
            CHECK_THAT(std::arg(values(2, 1)), WithinAbs(-1.64229502822366, 1e-6));
            CHECK_THAT(std::arg(values(2, 2)), WithinAbs(1.51299993708404, 1e-6));
            CHECK_THAT(std::arg(values(2, 3)), WithinAbs(-1.58094085237337, 1e-6));
            CHECK_THAT(std::arg(values(2, 4)), WithinAbs(-1.58094085237337, 1e-6));
            CHECK_THAT(std::arg(values(2, 5)), WithinAbs(1.51299993708404, 1e-6));
            CHECK_THAT(std::arg(values(2, 6)), WithinAbs(-1.64229502822366, 1e-6));
            CHECK_THAT(std::arg(values(2, 7)), WithinAbs(1.4187912238105, 1e-6));
            CHECK_THAT(std::arg(values(3, 0)), WithinAbs(1.41879122381355, 1e-6));
            CHECK_THAT(std::arg(values(3, 1)), WithinAbs(-1.64229502822457, 1e-6));
            CHECK_THAT(std::arg(values(3, 2)), WithinAbs(1.51299993708421, 1e-6));
            CHECK_THAT(std::arg(values(3, 3)), WithinAbs(-1.58094085237409, 1e-6));
            CHECK_THAT(std::arg(values(3, 4)), WithinAbs(-1.58094085237409, 1e-6));
            CHECK_THAT(std::arg(values(3, 5)), WithinAbs(1.51299993708421, 1e-6));
            CHECK_THAT(std::arg(values(3, 6)), WithinAbs(-1.64229502822457, 1e-6));
            CHECK_THAT(std::arg(values(3, 7)), WithinAbs(1.41879122381355, 1e-6));
            CHECK_THAT(std::arg(values(4, 0)), WithinAbs(1.41879122380683, 1e-6));
            CHECK_THAT(std::arg(values(4, 1)), WithinAbs(-1.64229502822128, 1e-6));
            CHECK_THAT(std::arg(values(4, 2)), WithinAbs(1.51299993708441, 1e-6));
            CHECK_THAT(std::arg(values(4, 3)), WithinAbs(-1.58094085237337, 1e-6));
            CHECK_THAT(std::arg(values(4, 4)), WithinAbs(-1.58094085237337, 1e-6));
            CHECK_THAT(std::arg(values(4, 5)), WithinAbs(1.51299993708441, 1e-6));
            CHECK_THAT(std::arg(values(4, 6)), WithinAbs(-1.64229502822293, 1e-6));
            CHECK_THAT(std::arg(values(4, 7)), WithinAbs(1.41879122380683, 1e-6));
            CHECK_THAT(std::arg(values(5, 0)), WithinAbs(1.41879122381167, 1e-6));
            CHECK_THAT(std::arg(values(5, 1)), WithinAbs(-1.6422950282215, 1e-6));
            CHECK_THAT(std::arg(values(5, 2)), WithinAbs(1.51299993708512, 1e-6));
            CHECK_THAT(std::arg(values(5, 3)), WithinAbs(-1.58094085237398, 1e-6));
            CHECK_THAT(std::arg(values(5, 4)), WithinAbs(-1.58094085237398, 1e-6));
            CHECK_THAT(std::arg(values(5, 5)), WithinAbs(1.51299993708512, 1e-6));
            CHECK_THAT(std::arg(values(5, 6)), WithinAbs(-1.64229502822211, 1e-6));
            CHECK_THAT(std::arg(values(5, 7)), WithinAbs(1.41879122381167, 1e-6));
            CHECK_THAT(std::arg(values(6, 0)), WithinAbs(1.41879122381508, 1e-6));
            CHECK_THAT(std::arg(values(6, 1)), WithinAbs(-1.64229502822325, 1e-6));
            CHECK_THAT(std::arg(values(6, 2)), WithinAbs(1.51299993708512, 1e-6));
            CHECK_THAT(std::arg(values(6, 3)), WithinAbs(-1.58094085237371, 1e-6));
            CHECK_THAT(std::arg(values(6, 4)), WithinAbs(-1.58094085237351, 1e-6));
            CHECK_THAT(std::arg(values(6, 5)), WithinAbs(1.51299993708512, 1e-6));
            CHECK_THAT(std::arg(values(6, 6)), WithinAbs(-1.64229502822365, 1e-6));
            CHECK_THAT(std::arg(values(6, 7)), WithinAbs(1.41879122381508, 1e-6));
            CHECK_THAT(std::arg(values(7, 0)), WithinAbs(1.41879122380825, 1e-6));
            CHECK_THAT(std::arg(values(7, 1)), WithinAbs(-1.6422950282223, 1e-6));
            CHECK_THAT(std::arg(values(7, 2)), WithinAbs(1.51299993708358, 1e-6));
            CHECK_THAT(std::arg(values(7, 3)), WithinAbs(-1.5809408523736, 1e-6));
            CHECK_THAT(std::arg(values(7, 4)), WithinAbs(-1.5809408523736, 1e-6));
            CHECK_THAT(std::arg(values(7, 5)), WithinAbs(1.51299993708358, 1e-6));
            CHECK_THAT(std::arg(values(7, 6)), WithinAbs(-1.64229502822322, 1e-6));
            CHECK_THAT(std::arg(values(7, 7)), WithinAbs(1.41879122380825, 1e-6));
            CHECK_THAT(std::arg(values(8, 0)), WithinAbs(1.41879122380825, 1e-6));
            CHECK_THAT(std::arg(values(8, 1)), WithinAbs(-1.6422950282223, 1e-6));
            CHECK_THAT(std::arg(values(8, 2)), WithinAbs(1.51299993708358, 1e-6));
            CHECK_THAT(std::arg(values(8, 3)), WithinAbs(-1.5809408523736, 1e-6));
            CHECK_THAT(std::arg(values(8, 4)), WithinAbs(-1.5809408523736, 1e-6));
            CHECK_THAT(std::arg(values(8, 5)), WithinAbs(1.51299993708358, 1e-6));
            CHECK_THAT(std::arg(values(8, 6)), WithinAbs(-1.64229502822322, 1e-6));
            CHECK_THAT(std::arg(values(8, 7)), WithinAbs(1.41879122380825, 1e-6));
            CHECK_THAT(std::arg(values(9, 0)), WithinAbs(1.41879122381508, 1e-6));
            CHECK_THAT(std::arg(values(9, 1)), WithinAbs(-1.64229502822325, 1e-6));
            CHECK_THAT(std::arg(values(9, 2)), WithinAbs(1.51299993708512, 1e-6));
            CHECK_THAT(std::arg(values(9, 3)), WithinAbs(-1.58094085237371, 1e-6));
            CHECK_THAT(std::arg(values(9, 4)), WithinAbs(-1.58094085237351, 1e-6));
            CHECK_THAT(std::arg(values(9, 5)), WithinAbs(1.51299993708512, 1e-6));
            CHECK_THAT(std::arg(values(9, 6)), WithinAbs(-1.64229502822365, 1e-6));
            CHECK_THAT(std::arg(values(9, 7)), WithinAbs(1.41879122381508, 1e-6));
            CHECK_THAT(std::arg(values(10, 0)), WithinAbs(1.41879122381167, 1e-6));
            CHECK_THAT(std::arg(values(10, 1)), WithinAbs(-1.6422950282215, 1e-6));
            CHECK_THAT(std::arg(values(10, 2)), WithinAbs(1.51299993708512, 1e-6));
            CHECK_THAT(std::arg(values(10, 3)), WithinAbs(-1.58094085237398, 1e-6));
            CHECK_THAT(std::arg(values(10, 4)), WithinAbs(-1.58094085237398, 1e-6));
            CHECK_THAT(std::arg(values(10, 5)), WithinAbs(1.51299993708512, 1e-6));
            CHECK_THAT(std::arg(values(10, 6)), WithinAbs(-1.64229502822211, 1e-6));
            CHECK_THAT(std::arg(values(10, 7)), WithinAbs(1.41879122381167, 1e-6));
            CHECK_THAT(std::arg(values(11, 0)), WithinAbs(1.41879122381584, 1e-6));
            CHECK_THAT(std::arg(values(11, 1)), WithinAbs(-1.64229502822229, 1e-6));
            CHECK_THAT(std::arg(values(11, 2)), WithinAbs(1.51299993708506, 1e-6));
            CHECK_THAT(std::arg(values(11, 3)), WithinAbs(-1.58094085237298, 1e-6));
            CHECK_THAT(std::arg(values(11, 4)), WithinAbs(-1.58094085237298, 1e-6));
            CHECK_THAT(std::arg(values(11, 5)), WithinAbs(1.51299993708506, 1e-6));
            CHECK_THAT(std::arg(values(11, 6)), WithinAbs(-1.64229502822068, 1e-6));
            CHECK_THAT(std::arg(values(11, 7)), WithinAbs(1.41879122381584, 1e-6));
            CHECK_THAT(std::arg(values(12, 0)), WithinAbs(1.41879122381355, 1e-6));
            CHECK_THAT(std::arg(values(12, 1)), WithinAbs(-1.64229502822457, 1e-6));
            CHECK_THAT(std::arg(values(12, 2)), WithinAbs(1.51299993708421, 1e-6));
            CHECK_THAT(std::arg(values(12, 3)), WithinAbs(-1.58094085237409, 1e-6));
            CHECK_THAT(std::arg(values(12, 4)), WithinAbs(-1.58094085237409, 1e-6));
            CHECK_THAT(std::arg(values(12, 5)), WithinAbs(1.51299993708421, 1e-6));
            CHECK_THAT(std::arg(values(12, 6)), WithinAbs(-1.64229502822457, 1e-6));
            CHECK_THAT(std::arg(values(12, 7)), WithinAbs(1.41879122381355, 1e-6));
            CHECK_THAT(std::arg(values(13, 0)), WithinAbs(1.4187912238105, 1e-6));
            CHECK_THAT(std::arg(values(13, 1)), WithinAbs(-1.64229502822366, 1e-6));
            CHECK_THAT(std::arg(values(13, 2)), WithinAbs(1.51299993708404, 1e-6));
            CHECK_THAT(std::arg(values(13, 3)), WithinAbs(-1.58094085237337, 1e-6));
            CHECK_THAT(std::arg(values(13, 4)), WithinAbs(-1.58094085237337, 1e-6));
            CHECK_THAT(std::arg(values(13, 5)), WithinAbs(1.51299993708404, 1e-6));
            CHECK_THAT(std::arg(values(13, 6)), WithinAbs(-1.64229502822366, 1e-6));
            CHECK_THAT(std::arg(values(13, 7)), WithinAbs(1.4187912238105, 1e-6));
            CHECK_THAT(std::arg(values(14, 0)), WithinAbs(1.41879122381387, 1e-6));
            CHECK_THAT(std::arg(values(14, 1)), WithinAbs(-1.6422950282222, 1e-6));
            CHECK_THAT(std::arg(values(14, 2)), WithinAbs(1.51299993708435, 1e-6));
            CHECK_THAT(std::arg(values(14, 3)), WithinAbs(-1.58094085237376, 1e-6));
            CHECK_THAT(std::arg(values(14, 4)), WithinAbs(-1.58094085237368, 1e-6));
            CHECK_THAT(std::arg(values(14, 5)), WithinAbs(1.51299993708435, 1e-6));
            CHECK_THAT(std::arg(values(14, 6)), WithinAbs(-1.64229502822161, 1e-6));
            CHECK_THAT(std::arg(values(14, 7)), WithinAbs(1.41879122381387, 1e-6));
            CHECK_THAT(std::arg(values(15, 0)), WithinAbs(1.41879122381658, 1e-6));
            CHECK_THAT(std::arg(values(15, 1)), WithinAbs(-1.64229502822274, 1e-6));
            CHECK_THAT(std::arg(values(15, 2)), WithinAbs(1.51299993708328, 1e-6));
            CHECK_THAT(std::arg(values(15, 3)), WithinAbs(-1.58094085237358, 1e-6));
            CHECK_THAT(std::arg(values(15, 4)), WithinAbs(-1.58094085237358, 1e-6));
            CHECK_THAT(std::arg(values(15, 5)), WithinAbs(1.51299993708328, 1e-6));
            CHECK_THAT(std::arg(values(15, 6)), WithinAbs(-1.64229502822321, 1e-6));
            CHECK_THAT(std::arg(values(15, 7)), WithinAbs(1.41879122381658, 1e-6));
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
            CHECK_THAT(positions(0, 0).x, WithinAbs(65.32814824381881635, 1e-9));
            CHECK_THAT(positions(0, 0).y, WithinAbs(65.32814824381883057, 1e-9));
            CHECK_THAT(positions(0, 0).z, WithinAbs(-38.26834323650897574, 1e-9));
            CHECK_THAT(positions(positions.shape().rows - 1, 0).x, WithinAbs(-65.32814824381881635, 1e-9));
            CHECK_THAT(positions(positions.shape().rows - 1, 0).y, WithinAbs(65.32814824381883057, 1e-9));
            CHECK_THAT(positions(positions.shape().rows - 1, 0).z, WithinAbs(-38.26834323650897574, 1e-9));
            CHECK_THAT(positions(0, positions.shape().cols - 1).x, WithinAbs(65.32814824381881635, 1e-9));
            CHECK_THAT(positions(0, positions.shape().cols - 1).y, WithinAbs(65.32814824381883057, 1e-9));
            CHECK_THAT(positions(0, positions.shape().cols - 1).z, WithinAbs(38.26834323650897574, 1e-9));
            CHECK_THAT(positions(positions.shape().rows - 1, positions.shape().cols - 1).x, WithinAbs(-65.32814824381881635, 1e-9));
            CHECK_THAT(positions(positions.shape().rows - 1, positions.shape().cols - 1).y, WithinAbs(65.32814824381883057, 1e-9));
            CHECK_THAT(positions(positions.shape().rows - 1, positions.shape().cols - 1).z, WithinAbs(38.26834323650897574, 1e-9));

            // Verify page 0 (note: this page should be equal to the eval_geometry test)
            {
                auto& values = data[0];

                // Next, find the maximum of the physical values and normalize for better comparison
                double const abs_max = normalize(values);
                CHECK_THAT(abs_max, WithinAbs(0.0056509419038846, 1e-9));

                // Verify physical values along the surface
                CHECK_THAT(std::abs(values(0, 0)), WithinAbs(0.0161323981305048, 1e-9));
                CHECK_THAT(std::abs(values(0, 1)), WithinAbs(0.0966088241309794, 1e-9));
                CHECK_THAT(std::abs(values(0, 2)), WithinAbs(0.270909628866502, 1e-9));
                CHECK_THAT(std::abs(values(0, 3)), WithinAbs(0.999999999999391, 1e-9));
                CHECK_THAT(std::abs(values(0, 4)), WithinAbs(0.999999999999391, 1e-9));
                CHECK_THAT(std::abs(values(0, 5)), WithinAbs(0.270909628866501, 1e-9));
                CHECK_THAT(std::abs(values(0, 6)), WithinAbs(0.0966088241310958, 1e-9));
                CHECK_THAT(std::abs(values(0, 7)), WithinAbs(0.0161323981305049, 1e-9));
                CHECK_THAT(std::abs(values(1, 0)), WithinAbs(0.0161323981305376, 1e-9));
                CHECK_THAT(std::abs(values(1, 1)), WithinAbs(0.0966088241308526, 1e-9));
                CHECK_THAT(std::abs(values(1, 2)), WithinAbs(0.270909628866573, 1e-9));
                CHECK_THAT(std::abs(values(1, 3)), WithinAbs(0.999999999999534, 1e-9));
                CHECK_THAT(std::abs(values(1, 4)), WithinAbs(0.999999999999555, 1e-9));
                CHECK_THAT(std::abs(values(1, 5)), WithinAbs(0.270909628866573, 1e-9));
                CHECK_THAT(std::abs(values(1, 6)), WithinAbs(0.0966088241308649, 1e-9));
                CHECK_THAT(std::abs(values(1, 7)), WithinAbs(0.0161323981305376, 1e-9));
                CHECK_THAT(std::abs(values(2, 0)), WithinAbs(0.0161323981306128, 1e-9));
                CHECK_THAT(std::abs(values(2, 1)), WithinAbs(0.0966088241309214, 1e-9));
                CHECK_THAT(std::abs(values(2, 2)), WithinAbs(0.270909628866805, 1e-9));
                CHECK_THAT(std::abs(values(2, 3)), WithinAbs(0.999999999999486, 1e-9));
                CHECK_THAT(std::abs(values(2, 4)), WithinAbs(0.999999999999487, 1e-9));
                CHECK_THAT(std::abs(values(2, 5)), WithinAbs(0.270909628866805, 1e-9));
                CHECK_THAT(std::abs(values(2, 6)), WithinAbs(0.0966088241309214, 1e-9));
                CHECK_THAT(std::abs(values(2, 7)), WithinAbs(0.0161323981306127, 1e-9));
                CHECK_THAT(std::abs(values(3, 0)), WithinAbs(0.0161323981306627, 1e-9));
                CHECK_THAT(std::abs(values(3, 1)), WithinAbs(0.0966088241310159, 1e-9));
                CHECK_THAT(std::abs(values(3, 2)), WithinAbs(0.27090962886659, 1e-9));
                CHECK_THAT(std::abs(values(3, 3)), WithinAbs(0.999999999999756, 1e-9));
                CHECK_THAT(std::abs(values(3, 4)), WithinAbs(0.999999999999756, 1e-9));
                CHECK_THAT(std::abs(values(3, 5)), WithinAbs(0.27090962886659, 1e-9));
                CHECK_THAT(std::abs(values(3, 6)), WithinAbs(0.096608824131016, 1e-9));
                CHECK_THAT(std::abs(values(3, 7)), WithinAbs(0.0161323981306626, 1e-9));
                CHECK_THAT(std::abs(values(4, 0)), WithinAbs(0.0161323981305155, 1e-9));
                CHECK_THAT(std::abs(values(4, 1)), WithinAbs(0.0966088241308565, 1e-9));
                CHECK_THAT(std::abs(values(4, 2)), WithinAbs(0.270909628866603, 1e-9));
                CHECK_THAT(std::abs(values(4, 3)), WithinAbs(0.999999999999401, 1e-9));
                CHECK_THAT(std::abs(values(4, 4)), WithinAbs(0.999999999999401, 1e-9));
                CHECK_THAT(std::abs(values(4, 5)), WithinAbs(0.270909628866603, 1e-9));
                CHECK_THAT(std::abs(values(4, 6)), WithinAbs(0.0966088241308739, 1e-9));
                CHECK_THAT(std::abs(values(4, 7)), WithinAbs(0.0161323981305156, 1e-9));
                CHECK_THAT(std::abs(values(5, 0)), WithinAbs(0.0161323981307005, 1e-9));
                CHECK_THAT(std::abs(values(5, 1)), WithinAbs(0.0966088241308201, 1e-9));
                CHECK_THAT(std::abs(values(5, 2)), WithinAbs(0.270909628866586, 1e-9));
                CHECK_THAT(std::abs(values(5, 3)), WithinAbs(0.99999999999957, 1e-9));
                CHECK_THAT(std::abs(values(5, 4)), WithinAbs(0.99999999999957, 1e-9));
                CHECK_THAT(std::abs(values(5, 5)), WithinAbs(0.270909628866585, 1e-9));
                CHECK_THAT(std::abs(values(5, 6)), WithinAbs(0.0966088241308295, 1e-9));
                CHECK_THAT(std::abs(values(5, 7)), WithinAbs(0.0161323981307005, 1e-9));
                CHECK_THAT(std::abs(values(6, 0)), WithinAbs(0.0161323981305657, 1e-9));
                CHECK_THAT(std::abs(values(6, 1)), WithinAbs(0.0966088241307718, 1e-9));
                CHECK_THAT(std::abs(values(6, 2)), WithinAbs(0.270909628866542, 1e-9));
                CHECK_THAT(std::abs(values(6, 3)), WithinAbs(0.999999999999859, 1e-9));
                CHECK_THAT(std::abs(values(6, 4)), WithinAbs(1, 1e-9));
                CHECK_THAT(std::abs(values(6, 5)), WithinAbs(0.270909628866541, 1e-9));
                CHECK_THAT(std::abs(values(6, 6)), WithinAbs(0.0966088241308371, 1e-9));
                CHECK_THAT(std::abs(values(6, 7)), WithinAbs(0.0161323981305657, 1e-9));
                CHECK_THAT(std::abs(values(7, 0)), WithinAbs(0.0161323981305536, 1e-9));
                CHECK_THAT(std::abs(values(7, 1)), WithinAbs(0.0966088241310956, 1e-9));
                CHECK_THAT(std::abs(values(7, 2)), WithinAbs(0.270909628866735, 1e-9));
                CHECK_THAT(std::abs(values(7, 3)), WithinAbs(0.999999999999692, 1e-9));
                CHECK_THAT(std::abs(values(7, 4)), WithinAbs(0.999999999999692, 1e-9));
                CHECK_THAT(std::abs(values(7, 5)), WithinAbs(0.270909628866735, 1e-9));
                CHECK_THAT(std::abs(values(7, 6)), WithinAbs(0.0966088241310184, 1e-9));
                CHECK_THAT(std::abs(values(7, 7)), WithinAbs(0.0161323981305537, 1e-9));
                CHECK_THAT(std::abs(values(8, 0)), WithinAbs(0.0161323981305536, 1e-9));
                CHECK_THAT(std::abs(values(8, 1)), WithinAbs(0.0966088241310956, 1e-9));
                CHECK_THAT(std::abs(values(8, 2)), WithinAbs(0.270909628866735, 1e-9));
                CHECK_THAT(std::abs(values(8, 3)), WithinAbs(0.999999999999692, 1e-9));
                CHECK_THAT(std::abs(values(8, 4)), WithinAbs(0.999999999999692, 1e-9));
                CHECK_THAT(std::abs(values(8, 5)), WithinAbs(0.270909628866735, 1e-9));
                CHECK_THAT(std::abs(values(8, 6)), WithinAbs(0.0966088241310184, 1e-9));
                CHECK_THAT(std::abs(values(8, 7)), WithinAbs(0.0161323981305537, 1e-9));
                CHECK_THAT(std::abs(values(9, 0)), WithinAbs(0.0161323981305657, 1e-9));
                CHECK_THAT(std::abs(values(9, 1)), WithinAbs(0.0966088241307718, 1e-9));
                CHECK_THAT(std::abs(values(9, 2)), WithinAbs(0.270909628866542, 1e-9));
                CHECK_THAT(std::abs(values(9, 3)), WithinAbs(0.999999999999859, 1e-9));
                CHECK_THAT(std::abs(values(9, 4)), WithinAbs(1, 1e-9));
                CHECK_THAT(std::abs(values(9, 5)), WithinAbs(0.270909628866542, 1e-9));
                CHECK_THAT(std::abs(values(9, 6)), WithinAbs(0.0966088241308371, 1e-9));
                CHECK_THAT(std::abs(values(9, 7)), WithinAbs(0.0161323981305657, 1e-9));
                CHECK_THAT(std::abs(values(10, 0)), WithinAbs(0.0161323981307004, 1e-9));
                CHECK_THAT(std::abs(values(10, 1)), WithinAbs(0.0966088241308201, 1e-9));
                CHECK_THAT(std::abs(values(10, 2)), WithinAbs(0.270909628866586, 1e-9));
                CHECK_THAT(std::abs(values(10, 3)), WithinAbs(0.99999999999957, 1e-9));
                CHECK_THAT(std::abs(values(10, 4)), WithinAbs(0.99999999999957, 1e-9));
                CHECK_THAT(std::abs(values(10, 5)), WithinAbs(0.270909628866585, 1e-9));
                CHECK_THAT(std::abs(values(10, 6)), WithinAbs(0.0966088241308295, 1e-9));
                CHECK_THAT(std::abs(values(10, 7)), WithinAbs(0.0161323981307004, 1e-9));
                CHECK_THAT(std::abs(values(11, 0)), WithinAbs(0.0161323981306158, 1e-9));
                CHECK_THAT(std::abs(values(11, 1)), WithinAbs(0.0966088241310356, 1e-9));
                CHECK_THAT(std::abs(values(11, 2)), WithinAbs(0.270909628866517, 1e-9));
                CHECK_THAT(std::abs(values(11, 3)), WithinAbs(0.999999999999496, 1e-9));
                CHECK_THAT(std::abs(values(11, 4)), WithinAbs(0.999999999999496, 1e-9));
                CHECK_THAT(std::abs(values(11, 5)), WithinAbs(0.270909628866517, 1e-9));
                CHECK_THAT(std::abs(values(11, 6)), WithinAbs(0.0966088241309495, 1e-9));
                CHECK_THAT(std::abs(values(11, 7)), WithinAbs(0.0161323981306157, 1e-9));
                CHECK_THAT(std::abs(values(12, 0)), WithinAbs(0.0161323981306627, 1e-9));
                CHECK_THAT(std::abs(values(12, 1)), WithinAbs(0.096608824131016, 1e-9));
                CHECK_THAT(std::abs(values(12, 2)), WithinAbs(0.27090962886659, 1e-9));
                CHECK_THAT(std::abs(values(12, 3)), WithinAbs(0.999999999999756, 1e-9));
                CHECK_THAT(std::abs(values(12, 4)), WithinAbs(0.999999999999756, 1e-9));
                CHECK_THAT(std::abs(values(12, 5)), WithinAbs(0.27090962886659, 1e-9));
                CHECK_THAT(std::abs(values(12, 6)), WithinAbs(0.096608824131016, 1e-9));
                CHECK_THAT(std::abs(values(12, 7)), WithinAbs(0.0161323981306626, 1e-9));
                CHECK_THAT(std::abs(values(13, 0)), WithinAbs(0.0161323981306128, 1e-9));
                CHECK_THAT(std::abs(values(13, 1)), WithinAbs(0.0966088241309214, 1e-9));
                CHECK_THAT(std::abs(values(13, 2)), WithinAbs(0.270909628866805, 1e-9));
                CHECK_THAT(std::abs(values(13, 3)), WithinAbs(0.999999999999486, 1e-9));
                CHECK_THAT(std::abs(values(13, 4)), WithinAbs(0.999999999999487, 1e-9));
                CHECK_THAT(std::abs(values(13, 5)), WithinAbs(0.270909628866805, 1e-9));
                CHECK_THAT(std::abs(values(13, 6)), WithinAbs(0.0966088241309214, 1e-9));
                CHECK_THAT(std::abs(values(13, 7)), WithinAbs(0.0161323981306128, 1e-9));
                CHECK_THAT(std::abs(values(14, 0)), WithinAbs(0.0161323981305375, 1e-9));
                CHECK_THAT(std::abs(values(14, 1)), WithinAbs(0.0966088241308526, 1e-9));
                CHECK_THAT(std::abs(values(14, 2)), WithinAbs(0.270909628866573, 1e-9));
                CHECK_THAT(std::abs(values(14, 3)), WithinAbs(0.999999999999534, 1e-9));
                CHECK_THAT(std::abs(values(14, 4)), WithinAbs(0.999999999999555, 1e-9));
                CHECK_THAT(std::abs(values(14, 5)), WithinAbs(0.270909628866572, 1e-9));
                CHECK_THAT(std::abs(values(14, 6)), WithinAbs(0.0966088241308648, 1e-9));
                CHECK_THAT(std::abs(values(14, 7)), WithinAbs(0.0161323981305376, 1e-9));
                CHECK_THAT(std::abs(values(15, 0)), WithinAbs(0.0161323981305049, 1e-9));
                CHECK_THAT(std::abs(values(15, 1)), WithinAbs(0.0966088241309794, 1e-9));
                CHECK_THAT(std::abs(values(15, 2)), WithinAbs(0.270909628866502, 1e-9));
                CHECK_THAT(std::abs(values(15, 3)), WithinAbs(0.999999999999391, 1e-9));
                CHECK_THAT(std::abs(values(15, 4)), WithinAbs(0.999999999999391, 1e-9));
                CHECK_THAT(std::abs(values(15, 5)), WithinAbs(0.270909628866501, 1e-9));
                CHECK_THAT(std::abs(values(15, 6)), WithinAbs(0.0966088241310958, 1e-9));
                CHECK_THAT(std::abs(values(15, 7)), WithinAbs(0.0161323981305048, 1e-9));
                CHECK_THAT(std::arg(values(0, 0)), WithinAbs(1.41879122381658, 1e-6));
                CHECK_THAT(std::arg(values(0, 1)), WithinAbs(-1.64229502822274, 1e-6));
                CHECK_THAT(std::arg(values(0, 2)), WithinAbs(1.51299993708328, 1e-6));
                CHECK_THAT(std::arg(values(0, 3)), WithinAbs(-1.58094085237358, 1e-6));
                CHECK_THAT(std::arg(values(0, 4)), WithinAbs(-1.58094085237358, 1e-6));
                CHECK_THAT(std::arg(values(0, 5)), WithinAbs(1.51299993708328, 1e-6));
                CHECK_THAT(std::arg(values(0, 6)), WithinAbs(-1.64229502822321, 1e-6));
                CHECK_THAT(std::arg(values(0, 7)), WithinAbs(1.41879122381658, 1e-6));
                CHECK_THAT(std::arg(values(1, 0)), WithinAbs(1.41879122381387, 1e-6));
                CHECK_THAT(std::arg(values(1, 1)), WithinAbs(-1.64229502822221, 1e-6));
                CHECK_THAT(std::arg(values(1, 2)), WithinAbs(1.51299993708435, 1e-6));
                CHECK_THAT(std::arg(values(1, 3)), WithinAbs(-1.58094085237376, 1e-6));
                CHECK_THAT(std::arg(values(1, 4)), WithinAbs(-1.58094085237368, 1e-6));
                CHECK_THAT(std::arg(values(1, 5)), WithinAbs(1.51299993708435, 1e-6));
                CHECK_THAT(std::arg(values(1, 6)), WithinAbs(-1.64229502822161, 1e-6));
                CHECK_THAT(std::arg(values(1, 7)), WithinAbs(1.41879122381387, 1e-6));
                CHECK_THAT(std::arg(values(2, 0)), WithinAbs(1.41879122381051, 1e-6));
                CHECK_THAT(std::arg(values(2, 1)), WithinAbs(-1.64229502822366, 1e-6));
                CHECK_THAT(std::arg(values(2, 2)), WithinAbs(1.51299993708404, 1e-6));
                CHECK_THAT(std::arg(values(2, 3)), WithinAbs(-1.58094085237337, 1e-6));
                CHECK_THAT(std::arg(values(2, 4)), WithinAbs(-1.58094085237337, 1e-6));
                CHECK_THAT(std::arg(values(2, 5)), WithinAbs(1.51299993708404, 1e-6));
                CHECK_THAT(std::arg(values(2, 6)), WithinAbs(-1.64229502822366, 1e-6));
                CHECK_THAT(std::arg(values(2, 7)), WithinAbs(1.4187912238105, 1e-6));
                CHECK_THAT(std::arg(values(3, 0)), WithinAbs(1.41879122381355, 1e-6));
                CHECK_THAT(std::arg(values(3, 1)), WithinAbs(-1.64229502822457, 1e-6));
                CHECK_THAT(std::arg(values(3, 2)), WithinAbs(1.51299993708421, 1e-6));
                CHECK_THAT(std::arg(values(3, 3)), WithinAbs(-1.58094085237409, 1e-6));
                CHECK_THAT(std::arg(values(3, 4)), WithinAbs(-1.58094085237409, 1e-6));
                CHECK_THAT(std::arg(values(3, 5)), WithinAbs(1.51299993708421, 1e-6));
                CHECK_THAT(std::arg(values(3, 6)), WithinAbs(-1.64229502822457, 1e-6));
                CHECK_THAT(std::arg(values(3, 7)), WithinAbs(1.41879122381355, 1e-6));
                CHECK_THAT(std::arg(values(4, 0)), WithinAbs(1.41879122380683, 1e-6));
                CHECK_THAT(std::arg(values(4, 1)), WithinAbs(-1.64229502822128, 1e-6));
                CHECK_THAT(std::arg(values(4, 2)), WithinAbs(1.51299993708441, 1e-6));
                CHECK_THAT(std::arg(values(4, 3)), WithinAbs(-1.58094085237337, 1e-6));
                CHECK_THAT(std::arg(values(4, 4)), WithinAbs(-1.58094085237337, 1e-6));
                CHECK_THAT(std::arg(values(4, 5)), WithinAbs(1.51299993708441, 1e-6));
                CHECK_THAT(std::arg(values(4, 6)), WithinAbs(-1.64229502822293, 1e-6));
                CHECK_THAT(std::arg(values(4, 7)), WithinAbs(1.41879122380683, 1e-6));
                CHECK_THAT(std::arg(values(5, 0)), WithinAbs(1.41879122381167, 1e-6));
                CHECK_THAT(std::arg(values(5, 1)), WithinAbs(-1.6422950282215, 1e-6));
                CHECK_THAT(std::arg(values(5, 2)), WithinAbs(1.51299993708512, 1e-6));
                CHECK_THAT(std::arg(values(5, 3)), WithinAbs(-1.58094085237398, 1e-6));
                CHECK_THAT(std::arg(values(5, 4)), WithinAbs(-1.58094085237398, 1e-6));
                CHECK_THAT(std::arg(values(5, 5)), WithinAbs(1.51299993708512, 1e-6));
                CHECK_THAT(std::arg(values(5, 6)), WithinAbs(-1.64229502822211, 1e-6));
                CHECK_THAT(std::arg(values(5, 7)), WithinAbs(1.41879122381167, 1e-6));
                CHECK_THAT(std::arg(values(6, 0)), WithinAbs(1.41879122381508, 1e-6));
                CHECK_THAT(std::arg(values(6, 1)), WithinAbs(-1.64229502822325, 1e-6));
                CHECK_THAT(std::arg(values(6, 2)), WithinAbs(1.51299993708512, 1e-6));
                CHECK_THAT(std::arg(values(6, 3)), WithinAbs(-1.58094085237371, 1e-6));
                CHECK_THAT(std::arg(values(6, 4)), WithinAbs(-1.58094085237351, 1e-6));
                CHECK_THAT(std::arg(values(6, 5)), WithinAbs(1.51299993708512, 1e-6));
                CHECK_THAT(std::arg(values(6, 6)), WithinAbs(-1.64229502822365, 1e-6));
                CHECK_THAT(std::arg(values(6, 7)), WithinAbs(1.41879122381508, 1e-6));
                CHECK_THAT(std::arg(values(7, 0)), WithinAbs(1.41879122380825, 1e-6));
                CHECK_THAT(std::arg(values(7, 1)), WithinAbs(-1.6422950282223, 1e-6));
                CHECK_THAT(std::arg(values(7, 2)), WithinAbs(1.51299993708358, 1e-6));
                CHECK_THAT(std::arg(values(7, 3)), WithinAbs(-1.5809408523736, 1e-6));
                CHECK_THAT(std::arg(values(7, 4)), WithinAbs(-1.5809408523736, 1e-6));
                CHECK_THAT(std::arg(values(7, 5)), WithinAbs(1.51299993708358, 1e-6));
                CHECK_THAT(std::arg(values(7, 6)), WithinAbs(-1.64229502822322, 1e-6));
                CHECK_THAT(std::arg(values(7, 7)), WithinAbs(1.41879122380825, 1e-6));
                CHECK_THAT(std::arg(values(8, 0)), WithinAbs(1.41879122380825, 1e-6));
                CHECK_THAT(std::arg(values(8, 1)), WithinAbs(-1.6422950282223, 1e-6));
                CHECK_THAT(std::arg(values(8, 2)), WithinAbs(1.51299993708358, 1e-6));
                CHECK_THAT(std::arg(values(8, 3)), WithinAbs(-1.5809408523736, 1e-6));
                CHECK_THAT(std::arg(values(8, 4)), WithinAbs(-1.5809408523736, 1e-6));
                CHECK_THAT(std::arg(values(8, 5)), WithinAbs(1.51299993708358, 1e-6));
                CHECK_THAT(std::arg(values(8, 6)), WithinAbs(-1.64229502822322, 1e-6));
                CHECK_THAT(std::arg(values(8, 7)), WithinAbs(1.41879122380825, 1e-6));
                CHECK_THAT(std::arg(values(9, 0)), WithinAbs(1.41879122381508, 1e-6));
                CHECK_THAT(std::arg(values(9, 1)), WithinAbs(-1.64229502822325, 1e-6));
                CHECK_THAT(std::arg(values(9, 2)), WithinAbs(1.51299993708512, 1e-6));
                CHECK_THAT(std::arg(values(9, 3)), WithinAbs(-1.58094085237371, 1e-6));
                CHECK_THAT(std::arg(values(9, 4)), WithinAbs(-1.58094085237351, 1e-6));
                CHECK_THAT(std::arg(values(9, 5)), WithinAbs(1.51299993708512, 1e-6));
                CHECK_THAT(std::arg(values(9, 6)), WithinAbs(-1.64229502822365, 1e-6));
                CHECK_THAT(std::arg(values(9, 7)), WithinAbs(1.41879122381508, 1e-6));
                CHECK_THAT(std::arg(values(10, 0)), WithinAbs(1.41879122381167, 1e-6));
                CHECK_THAT(std::arg(values(10, 1)), WithinAbs(-1.6422950282215, 1e-6));
                CHECK_THAT(std::arg(values(10, 2)), WithinAbs(1.51299993708512, 1e-6));
                CHECK_THAT(std::arg(values(10, 3)), WithinAbs(-1.58094085237398, 1e-6));
                CHECK_THAT(std::arg(values(10, 4)), WithinAbs(-1.58094085237398, 1e-6));
                CHECK_THAT(std::arg(values(10, 5)), WithinAbs(1.51299993708512, 1e-6));
                CHECK_THAT(std::arg(values(10, 6)), WithinAbs(-1.64229502822211, 1e-6));
                CHECK_THAT(std::arg(values(10, 7)), WithinAbs(1.41879122381167, 1e-6));
                CHECK_THAT(std::arg(values(11, 0)), WithinAbs(1.41879122381584, 1e-6));
                CHECK_THAT(std::arg(values(11, 1)), WithinAbs(-1.64229502822229, 1e-6));
                CHECK_THAT(std::arg(values(11, 2)), WithinAbs(1.51299993708506, 1e-6));
                CHECK_THAT(std::arg(values(11, 3)), WithinAbs(-1.58094085237298, 1e-6));
                CHECK_THAT(std::arg(values(11, 4)), WithinAbs(-1.58094085237298, 1e-6));
                CHECK_THAT(std::arg(values(11, 5)), WithinAbs(1.51299993708506, 1e-6));
                CHECK_THAT(std::arg(values(11, 6)), WithinAbs(-1.64229502822068, 1e-6));
                CHECK_THAT(std::arg(values(11, 7)), WithinAbs(1.41879122381584, 1e-6));
                CHECK_THAT(std::arg(values(12, 0)), WithinAbs(1.41879122381355, 1e-6));
                CHECK_THAT(std::arg(values(12, 1)), WithinAbs(-1.64229502822457, 1e-6));
                CHECK_THAT(std::arg(values(12, 2)), WithinAbs(1.51299993708421, 1e-6));
                CHECK_THAT(std::arg(values(12, 3)), WithinAbs(-1.58094085237409, 1e-6));
                CHECK_THAT(std::arg(values(12, 4)), WithinAbs(-1.58094085237409, 1e-6));
                CHECK_THAT(std::arg(values(12, 5)), WithinAbs(1.51299993708421, 1e-6));
                CHECK_THAT(std::arg(values(12, 6)), WithinAbs(-1.64229502822457, 1e-6));
                CHECK_THAT(std::arg(values(12, 7)), WithinAbs(1.41879122381355, 1e-6));
                CHECK_THAT(std::arg(values(13, 0)), WithinAbs(1.4187912238105, 1e-6));
                CHECK_THAT(std::arg(values(13, 1)), WithinAbs(-1.64229502822366, 1e-6));
                CHECK_THAT(std::arg(values(13, 2)), WithinAbs(1.51299993708404, 1e-6));
                CHECK_THAT(std::arg(values(13, 3)), WithinAbs(-1.58094085237337, 1e-6));
                CHECK_THAT(std::arg(values(13, 4)), WithinAbs(-1.58094085237337, 1e-6));
                CHECK_THAT(std::arg(values(13, 5)), WithinAbs(1.51299993708404, 1e-6));
                CHECK_THAT(std::arg(values(13, 6)), WithinAbs(-1.64229502822366, 1e-6));
                CHECK_THAT(std::arg(values(13, 7)), WithinAbs(1.4187912238105, 1e-6));
                CHECK_THAT(std::arg(values(14, 0)), WithinAbs(1.41879122381387, 1e-6));
                CHECK_THAT(std::arg(values(14, 1)), WithinAbs(-1.6422950282222, 1e-6));
                CHECK_THAT(std::arg(values(14, 2)), WithinAbs(1.51299993708435, 1e-6));
                CHECK_THAT(std::arg(values(14, 3)), WithinAbs(-1.58094085237376, 1e-6));
                CHECK_THAT(std::arg(values(14, 4)), WithinAbs(-1.58094085237368, 1e-6));
                CHECK_THAT(std::arg(values(14, 5)), WithinAbs(1.51299993708435, 1e-6));
                CHECK_THAT(std::arg(values(14, 6)), WithinAbs(-1.64229502822161, 1e-6));
                CHECK_THAT(std::arg(values(14, 7)), WithinAbs(1.41879122381387, 1e-6));
                CHECK_THAT(std::arg(values(15, 0)), WithinAbs(1.41879122381658, 1e-6));
                CHECK_THAT(std::arg(values(15, 1)), WithinAbs(-1.64229502822274, 1e-6));
                CHECK_THAT(std::arg(values(15, 2)), WithinAbs(1.51299993708328, 1e-6));
                CHECK_THAT(std::arg(values(15, 3)), WithinAbs(-1.58094085237358, 1e-6));
                CHECK_THAT(std::arg(values(15, 4)), WithinAbs(-1.58094085237358, 1e-6));
                CHECK_THAT(std::arg(values(15, 5)), WithinAbs(1.51299993708328, 1e-6));
                CHECK_THAT(std::arg(values(15, 6)), WithinAbs(-1.64229502822321, 1e-6));
                CHECK_THAT(std::arg(values(15, 7)), WithinAbs(1.41879122381658, 1e-6));
            }

            // Verify page 1
            {
                auto& values = data[1];

                // Next, find the maximum of the physical values and normalize for better comparison
                double const abs_max = normalize(values);
                CHECK_THAT(abs_max, WithinAbs(0.02272696136288165, 1e-9));

                // Verify physical values along the surface
                CHECK_THAT(std::abs(values(0, 0)), WithinAbs(0.0179844363415044, 1e-9));
                CHECK_THAT(std::abs(values(0, 1)), WithinAbs(0.219074952276568, 1e-9));
                CHECK_THAT(std::abs(values(0, 2)), WithinAbs(0.130694634215046, 1e-9));
                CHECK_THAT(std::abs(values(0, 3)), WithinAbs(0.99999999999972, 1e-9));
                CHECK_THAT(std::abs(values(0, 4)), WithinAbs(0.99999999999972, 1e-9));
                CHECK_THAT(std::abs(values(0, 5)), WithinAbs(0.130694634215046, 1e-9));
                CHECK_THAT(std::abs(values(0, 6)), WithinAbs(0.219074952276612, 1e-9));
                CHECK_THAT(std::abs(values(0, 7)), WithinAbs(0.0179844363415045, 1e-9));
                CHECK_THAT(std::abs(values(1, 0)), WithinAbs(0.017984436341519, 1e-9));
                CHECK_THAT(std::abs(values(1, 1)), WithinAbs(0.219074952276687, 1e-9));
                CHECK_THAT(std::abs(values(1, 2)), WithinAbs(0.130694634214944, 1e-9));
                CHECK_THAT(std::abs(values(1, 3)), WithinAbs(0.999999999999826, 1e-9));
                CHECK_THAT(std::abs(values(1, 4)), WithinAbs(0.999999999999837, 1e-9));
                CHECK_THAT(std::abs(values(1, 5)), WithinAbs(0.130694634214944, 1e-9));
                CHECK_THAT(std::abs(values(1, 6)), WithinAbs(0.21907495227674, 1e-9));
                CHECK_THAT(std::abs(values(1, 7)), WithinAbs(0.0179844363415191, 1e-9));
                CHECK_THAT(std::abs(values(2, 0)), WithinAbs(0.0179844363415261, 1e-9));
                CHECK_THAT(std::abs(values(2, 1)), WithinAbs(0.21907495227676, 1e-9));
                CHECK_THAT(std::abs(values(2, 2)), WithinAbs(0.130694634214872, 1e-9));
                CHECK_THAT(std::abs(values(2, 3)), WithinAbs(0.999999999999829, 1e-9));
                CHECK_THAT(std::abs(values(2, 4)), WithinAbs(0.999999999999829, 1e-9));
                CHECK_THAT(std::abs(values(2, 5)), WithinAbs(0.130694634214872, 1e-9));
                CHECK_THAT(std::abs(values(2, 6)), WithinAbs(0.21907495227676, 1e-9));
                CHECK_THAT(std::abs(values(2, 7)), WithinAbs(0.0179844363415262, 1e-9));
                CHECK_THAT(std::abs(values(3, 0)), WithinAbs(0.0179844363415625, 1e-9));
                CHECK_THAT(std::abs(values(3, 1)), WithinAbs(0.21907495227653, 1e-9));
                CHECK_THAT(std::abs(values(3, 2)), WithinAbs(0.130694634215113, 1e-9));
                CHECK_THAT(std::abs(values(3, 3)), WithinAbs(0.999999999999958, 1e-9));
                CHECK_THAT(std::abs(values(3, 4)), WithinAbs(0.999999999999958, 1e-9));
                CHECK_THAT(std::abs(values(3, 5)), WithinAbs(0.130694634215112, 1e-9));
                CHECK_THAT(std::abs(values(3, 6)), WithinAbs(0.21907495227653, 1e-9));
                CHECK_THAT(std::abs(values(3, 7)), WithinAbs(0.0179844363415626, 1e-9));
                CHECK_THAT(std::abs(values(4, 0)), WithinAbs(0.0179844363414722, 1e-9));
                CHECK_THAT(std::abs(values(4, 1)), WithinAbs(0.219074952276715, 1e-9));
                CHECK_THAT(std::abs(values(4, 2)), WithinAbs(0.130694634214987, 1e-9));
                CHECK_THAT(std::abs(values(4, 3)), WithinAbs(0.999999999999749, 1e-9));
                CHECK_THAT(std::abs(values(4, 4)), WithinAbs(0.999999999999749, 1e-9));
                CHECK_THAT(std::abs(values(4, 5)), WithinAbs(0.130694634214987, 1e-9));
                CHECK_THAT(std::abs(values(4, 6)), WithinAbs(0.219074952276657, 1e-9));
                CHECK_THAT(std::abs(values(4, 7)), WithinAbs(0.0179844363414723, 1e-9));
                CHECK_THAT(std::abs(values(5, 0)), WithinAbs(0.0179844363415668, 1e-9));
                CHECK_THAT(std::abs(values(5, 1)), WithinAbs(0.219074952276781, 1e-9));
                CHECK_THAT(std::abs(values(5, 2)), WithinAbs(0.130694634215074, 1e-9));
                CHECK_THAT(std::abs(values(5, 3)), WithinAbs(0.999999999999841, 1e-9));
                CHECK_THAT(std::abs(values(5, 4)), WithinAbs(0.999999999999841, 1e-9));
                CHECK_THAT(std::abs(values(5, 5)), WithinAbs(0.130694634215074, 1e-9));
                CHECK_THAT(std::abs(values(5, 6)), WithinAbs(0.219074952276729, 1e-9));
                CHECK_THAT(std::abs(values(5, 7)), WithinAbs(0.0179844363415668, 1e-9));
                CHECK_THAT(std::abs(values(6, 0)), WithinAbs(0.0179844363414931, 1e-9));
                CHECK_THAT(std::abs(values(6, 1)), WithinAbs(0.219074952276746, 1e-9));
                CHECK_THAT(std::abs(values(6, 2)), WithinAbs(0.130694634214992, 1e-9));
                CHECK_THAT(std::abs(values(6, 3)), WithinAbs(0.999999999999974, 1e-9));
                CHECK_THAT(std::abs(values(6, 4)), WithinAbs(1, 1e-9));
                CHECK_THAT(std::abs(values(6, 5)), WithinAbs(0.130694634214992, 1e-9));
                CHECK_THAT(std::abs(values(6, 6)), WithinAbs(0.219074952276694, 1e-9));
                CHECK_THAT(std::abs(values(6, 7)), WithinAbs(0.0179844363414932, 1e-9));
                CHECK_THAT(std::abs(values(7, 0)), WithinAbs(0.0179844363414607, 1e-9));
                CHECK_THAT(std::abs(values(7, 1)), WithinAbs(0.219074952276704, 1e-9));
                CHECK_THAT(std::abs(values(7, 2)), WithinAbs(0.130694634214834, 1e-9));
                CHECK_THAT(std::abs(values(7, 3)), WithinAbs(0.999999999999837, 1e-9));
                CHECK_THAT(std::abs(values(7, 4)), WithinAbs(0.999999999999837, 1e-9));
                CHECK_THAT(std::abs(values(7, 5)), WithinAbs(0.130694634214834, 1e-9));
                CHECK_THAT(std::abs(values(7, 6)), WithinAbs(0.219074952276674, 1e-9));
                CHECK_THAT(std::abs(values(7, 7)), WithinAbs(0.0179844363414608, 1e-9));
                CHECK_THAT(std::abs(values(8, 0)), WithinAbs(0.0179844363414607, 1e-9));
                CHECK_THAT(std::abs(values(8, 1)), WithinAbs(0.219074952276704, 1e-9));
                CHECK_THAT(std::abs(values(8, 2)), WithinAbs(0.130694634214834, 1e-9));
                CHECK_THAT(std::abs(values(8, 3)), WithinAbs(0.999999999999837, 1e-9));
                CHECK_THAT(std::abs(values(8, 4)), WithinAbs(0.999999999999837, 1e-9));
                CHECK_THAT(std::abs(values(8, 5)), WithinAbs(0.130694634214834, 1e-9));
                CHECK_THAT(std::abs(values(8, 6)), WithinAbs(0.219074952276674, 1e-9));
                CHECK_THAT(std::abs(values(8, 7)), WithinAbs(0.0179844363414608, 1e-9));
                CHECK_THAT(std::abs(values(9, 0)), WithinAbs(0.0179844363414931, 1e-9));
                CHECK_THAT(std::abs(values(9, 1)), WithinAbs(0.219074952276746, 1e-9));
                CHECK_THAT(std::abs(values(9, 2)), WithinAbs(0.130694634214992, 1e-9));
                CHECK_THAT(std::abs(values(9, 3)), WithinAbs(0.999999999999974, 1e-9));
                CHECK_THAT(std::abs(values(9, 4)), WithinAbs(1, 1e-9));
                CHECK_THAT(std::abs(values(9, 5)), WithinAbs(0.130694634214992, 1e-9));
                CHECK_THAT(std::abs(values(9, 6)), WithinAbs(0.219074952276694, 1e-9));
                CHECK_THAT(std::abs(values(9, 7)), WithinAbs(0.0179844363414932, 1e-9));
                CHECK_THAT(std::abs(values(10, 0)), WithinAbs(0.0179844363415668, 1e-9));
                CHECK_THAT(std::abs(values(10, 1)), WithinAbs(0.219074952276781, 1e-9));
                CHECK_THAT(std::abs(values(10, 2)), WithinAbs(0.130694634215074, 1e-9));
                CHECK_THAT(std::abs(values(10, 3)), WithinAbs(0.999999999999841, 1e-9));
                CHECK_THAT(std::abs(values(10, 4)), WithinAbs(0.999999999999841, 1e-9));
                CHECK_THAT(std::abs(values(10, 5)), WithinAbs(0.130694634215074, 1e-9));
                CHECK_THAT(std::abs(values(10, 6)), WithinAbs(0.219074952276729, 1e-9));
                CHECK_THAT(std::abs(values(10, 7)), WithinAbs(0.0179844363415669, 1e-9));
                CHECK_THAT(std::abs(values(11, 0)), WithinAbs(0.0179844363414709, 1e-9));
                CHECK_THAT(std::abs(values(11, 1)), WithinAbs(0.21907495227672, 1e-9));
                CHECK_THAT(std::abs(values(11, 2)), WithinAbs(0.130694634215029, 1e-9));
                CHECK_THAT(std::abs(values(11, 3)), WithinAbs(0.999999999999804, 1e-9));
                CHECK_THAT(std::abs(values(11, 4)), WithinAbs(0.999999999999804, 1e-9));
                CHECK_THAT(std::abs(values(11, 5)), WithinAbs(0.130694634215029, 1e-9));
                CHECK_THAT(std::abs(values(11, 6)), WithinAbs(0.219074952276756, 1e-9));
                CHECK_THAT(std::abs(values(11, 7)), WithinAbs(0.017984436341471, 1e-9));
                CHECK_THAT(std::abs(values(12, 0)), WithinAbs(0.0179844363415625, 1e-9));
                CHECK_THAT(std::abs(values(12, 1)), WithinAbs(0.21907495227653, 1e-9));
                CHECK_THAT(std::abs(values(12, 2)), WithinAbs(0.130694634215113, 1e-9));
                CHECK_THAT(std::abs(values(12, 3)), WithinAbs(0.999999999999958, 1e-9));
                CHECK_THAT(std::abs(values(12, 4)), WithinAbs(0.999999999999958, 1e-9));
                CHECK_THAT(std::abs(values(12, 5)), WithinAbs(0.130694634215112, 1e-9));
                CHECK_THAT(std::abs(values(12, 6)), WithinAbs(0.21907495227653, 1e-9));
                CHECK_THAT(std::abs(values(12, 7)), WithinAbs(0.0179844363415626, 1e-9));
                CHECK_THAT(std::abs(values(13, 0)), WithinAbs(0.0179844363415261, 1e-9));
                CHECK_THAT(std::abs(values(13, 1)), WithinAbs(0.21907495227676, 1e-9));
                CHECK_THAT(std::abs(values(13, 2)), WithinAbs(0.130694634214872, 1e-9));
                CHECK_THAT(std::abs(values(13, 3)), WithinAbs(0.999999999999829, 1e-9));
                CHECK_THAT(std::abs(values(13, 4)), WithinAbs(0.999999999999829, 1e-9));
                CHECK_THAT(std::abs(values(13, 5)), WithinAbs(0.130694634214872, 1e-9));
                CHECK_THAT(std::abs(values(13, 6)), WithinAbs(0.21907495227676, 1e-9));
                CHECK_THAT(std::abs(values(13, 7)), WithinAbs(0.0179844363415262, 1e-9));
                CHECK_THAT(std::abs(values(14, 0)), WithinAbs(0.017984436341519, 1e-9));
                CHECK_THAT(std::abs(values(14, 1)), WithinAbs(0.219074952276687, 1e-9));
                CHECK_THAT(std::abs(values(14, 2)), WithinAbs(0.130694634214944, 1e-9));
                CHECK_THAT(std::abs(values(14, 3)), WithinAbs(0.999999999999826, 1e-9));
                CHECK_THAT(std::abs(values(14, 4)), WithinAbs(0.999999999999837, 1e-9));
                CHECK_THAT(std::abs(values(14, 5)), WithinAbs(0.130694634214944, 1e-9));
                CHECK_THAT(std::abs(values(14, 6)), WithinAbs(0.21907495227674, 1e-9));
                CHECK_THAT(std::abs(values(14, 7)), WithinAbs(0.0179844363415191, 1e-9));
                CHECK_THAT(std::abs(values(15, 0)), WithinAbs(0.0179844363415044, 1e-9));
                CHECK_THAT(std::abs(values(15, 1)), WithinAbs(0.219074952276568, 1e-9));
                CHECK_THAT(std::abs(values(15, 2)), WithinAbs(0.130694634215046, 1e-9));
                CHECK_THAT(std::abs(values(15, 3)), WithinAbs(0.99999999999972, 1e-9));
                CHECK_THAT(std::abs(values(15, 4)), WithinAbs(0.99999999999972, 1e-9));
                CHECK_THAT(std::abs(values(15, 5)), WithinAbs(0.130694634215046, 1e-9));
                CHECK_THAT(std::abs(values(15, 6)), WithinAbs(0.219074952276612, 1e-9));
                CHECK_THAT(std::abs(values(15, 7)), WithinAbs(0.0179844363415045, 1e-9));
                CHECK_THAT(std::arg(values(0, 0)), WithinAbs(0.377707600726311, 1e-6));
                CHECK_THAT(std::arg(values(0, 1)), WithinAbs(-2.64634107528761, 1e-6));
                CHECK_THAT(std::arg(values(0, 2)), WithinAbs(0.575625867868319, 1e-6));
                CHECK_THAT(std::arg(values(0, 3)), WithinAbs(0.514275616713248, 1e-6));
                CHECK_THAT(std::arg(values(0, 4)), WithinAbs(0.514275616713248, 1e-6));
                CHECK_THAT(std::arg(values(0, 5)), WithinAbs(0.575625867868318, 1e-6));
                CHECK_THAT(std::arg(values(0, 6)), WithinAbs(-2.64634107528697, 1e-6));
                CHECK_THAT(std::arg(values(0, 7)), WithinAbs(0.377707600726312, 1e-6));
                CHECK_THAT(std::arg(values(1, 0)), WithinAbs(0.377707600730267, 1e-6));
                CHECK_THAT(std::arg(values(1, 1)), WithinAbs(-2.64634107528804, 1e-6));
                CHECK_THAT(std::arg(values(1, 2)), WithinAbs(0.575625867868742, 1e-6));
                CHECK_THAT(std::arg(values(1, 3)), WithinAbs(0.514275616713096, 1e-6));
                CHECK_THAT(std::arg(values(1, 4)), WithinAbs(0.514275616713161, 1e-6));
                CHECK_THAT(std::arg(values(1, 5)), WithinAbs(0.575625867868742, 1e-6));
                CHECK_THAT(std::arg(values(1, 6)), WithinAbs(-2.64634107528794, 1e-6));
                CHECK_THAT(std::arg(values(1, 7)), WithinAbs(0.37770760073027, 1e-6));
                CHECK_THAT(std::arg(values(2, 0)), WithinAbs(0.377707600727274, 1e-6));
                CHECK_THAT(std::arg(values(2, 1)), WithinAbs(-2.64634107528773, 1e-6));
                CHECK_THAT(std::arg(values(2, 2)), WithinAbs(0.575625867868465, 1e-6));
                CHECK_THAT(std::arg(values(2, 3)), WithinAbs(0.514275616713431, 1e-6));
                CHECK_THAT(std::arg(values(2, 4)), WithinAbs(0.514275616713431, 1e-6));
                CHECK_THAT(std::arg(values(2, 5)), WithinAbs(0.575625867868464, 1e-6));
                CHECK_THAT(std::arg(values(2, 6)), WithinAbs(-2.64634107528773, 1e-6));
                CHECK_THAT(std::arg(values(2, 7)), WithinAbs(0.377707600727276, 1e-6));
                CHECK_THAT(std::arg(values(3, 0)), WithinAbs(0.377707600724902, 1e-6));
                CHECK_THAT(std::arg(values(3, 1)), WithinAbs(-2.64634107528776, 1e-6));
                CHECK_THAT(std::arg(values(3, 2)), WithinAbs(0.575625867868195, 1e-6));
                CHECK_THAT(std::arg(values(3, 3)), WithinAbs(0.514275616712955, 1e-6));
                CHECK_THAT(std::arg(values(3, 4)), WithinAbs(0.514275616712955, 1e-6));
                CHECK_THAT(std::arg(values(3, 5)), WithinAbs(0.575625867868194, 1e-6));
                CHECK_THAT(std::arg(values(3, 6)), WithinAbs(-2.64634107528776, 1e-6));
                CHECK_THAT(std::arg(values(3, 7)), WithinAbs(0.3777076007249, 1e-6));
                CHECK_THAT(std::arg(values(4, 0)), WithinAbs(0.377707600735099, 1e-6));
                CHECK_THAT(std::arg(values(4, 1)), WithinAbs(-2.6463410752884, 1e-6));
                CHECK_THAT(std::arg(values(4, 2)), WithinAbs(0.575625867868665, 1e-6));
                CHECK_THAT(std::arg(values(4, 3)), WithinAbs(0.514275616713339, 1e-6));
                CHECK_THAT(std::arg(values(4, 4)), WithinAbs(0.514275616713339, 1e-6));
                CHECK_THAT(std::arg(values(4, 5)), WithinAbs(0.575625867868664, 1e-6));
                CHECK_THAT(std::arg(values(4, 6)), WithinAbs(-2.64634107528799, 1e-6));
                CHECK_THAT(std::arg(values(4, 7)), WithinAbs(0.377707600735099, 1e-6));
                CHECK_THAT(std::arg(values(5, 0)), WithinAbs(0.37770760073266, 1e-6));
                CHECK_THAT(std::arg(values(5, 1)), WithinAbs(-2.64634107528749, 1e-6));
                CHECK_THAT(std::arg(values(5, 2)), WithinAbs(0.575625867867775, 1e-6));
                CHECK_THAT(std::arg(values(5, 3)), WithinAbs(0.514275616712961, 1e-6));
                CHECK_THAT(std::arg(values(5, 4)), WithinAbs(0.514275616712961, 1e-6));
                CHECK_THAT(std::arg(values(5, 5)), WithinAbs(0.575625867867774, 1e-6));
                CHECK_THAT(std::arg(values(5, 6)), WithinAbs(-2.64634107528738, 1e-6));
                CHECK_THAT(std::arg(values(5, 7)), WithinAbs(0.377707600732659, 1e-6));
                CHECK_THAT(std::arg(values(6, 0)), WithinAbs(0.377707600727765, 1e-6));
                CHECK_THAT(std::arg(values(6, 1)), WithinAbs(-2.64634107528739, 1e-6));
                CHECK_THAT(std::arg(values(6, 2)), WithinAbs(0.575625867867103, 1e-6));
                CHECK_THAT(std::arg(values(6, 3)), WithinAbs(0.514275616713199, 1e-6));
                CHECK_THAT(std::arg(values(6, 4)), WithinAbs(0.514275616713259, 1e-6));
                CHECK_THAT(std::arg(values(6, 5)), WithinAbs(0.575625867867103, 1e-6));
                CHECK_THAT(std::arg(values(6, 6)), WithinAbs(-2.64634107528728, 1e-6));
                CHECK_THAT(std::arg(values(6, 7)), WithinAbs(0.377707600727765, 1e-6));
                CHECK_THAT(std::arg(values(7, 0)), WithinAbs(0.377707600731951, 1e-6));
                CHECK_THAT(std::arg(values(7, 1)), WithinAbs(-2.64634107528807, 1e-6));
                CHECK_THAT(std::arg(values(7, 2)), WithinAbs(0.575625867867917, 1e-6));
                CHECK_THAT(std::arg(values(7, 3)), WithinAbs(0.51427561671336, 1e-6));
                CHECK_THAT(std::arg(values(7, 4)), WithinAbs(0.51427561671336, 1e-6));
                CHECK_THAT(std::arg(values(7, 5)), WithinAbs(0.575625867867916, 1e-6));
                CHECK_THAT(std::arg(values(7, 6)), WithinAbs(-2.64634107528785, 1e-6));
                CHECK_THAT(std::arg(values(7, 7)), WithinAbs(0.377707600731954, 1e-6));
                CHECK_THAT(std::arg(values(8, 0)), WithinAbs(0.377707600731951, 1e-6));
                CHECK_THAT(std::arg(values(8, 1)), WithinAbs(-2.64634107528807, 1e-6));
                CHECK_THAT(std::arg(values(8, 2)), WithinAbs(0.575625867867917, 1e-6));
                CHECK_THAT(std::arg(values(8, 3)), WithinAbs(0.51427561671336, 1e-6));
                CHECK_THAT(std::arg(values(8, 4)), WithinAbs(0.51427561671336, 1e-6));
                CHECK_THAT(std::arg(values(8, 5)), WithinAbs(0.575625867867916, 1e-6));
                CHECK_THAT(std::arg(values(8, 6)), WithinAbs(-2.64634107528785, 1e-6));
                CHECK_THAT(std::arg(values(8, 7)), WithinAbs(0.377707600731954, 1e-6));
                CHECK_THAT(std::arg(values(9, 0)), WithinAbs(0.377707600727764, 1e-6));
                CHECK_THAT(std::arg(values(9, 1)), WithinAbs(-2.64634107528739, 1e-6));
                CHECK_THAT(std::arg(values(9, 2)), WithinAbs(0.575625867867103, 1e-6));
                CHECK_THAT(std::arg(values(9, 3)), WithinAbs(0.514275616713199, 1e-6));
                CHECK_THAT(std::arg(values(9, 4)), WithinAbs(0.514275616713259, 1e-6));
                CHECK_THAT(std::arg(values(9, 5)), WithinAbs(0.575625867867103, 1e-6));
                CHECK_THAT(std::arg(values(9, 6)), WithinAbs(-2.64634107528728, 1e-6));
                CHECK_THAT(std::arg(values(9, 7)), WithinAbs(0.377707600727763, 1e-6));
                CHECK_THAT(std::arg(values(10, 0)), WithinAbs(0.377707600732658, 1e-6));
                CHECK_THAT(std::arg(values(10, 1)), WithinAbs(-2.64634107528749, 1e-6));
                CHECK_THAT(std::arg(values(10, 2)), WithinAbs(0.575625867867775, 1e-6));
                CHECK_THAT(std::arg(values(10, 3)), WithinAbs(0.514275616712961, 1e-6));
                CHECK_THAT(std::arg(values(10, 4)), WithinAbs(0.514275616712961, 1e-6));
                CHECK_THAT(std::arg(values(10, 5)), WithinAbs(0.575625867867774, 1e-6));
                CHECK_THAT(std::arg(values(10, 6)), WithinAbs(-2.64634107528738, 1e-6));
                CHECK_THAT(std::arg(values(10, 7)), WithinAbs(0.377707600732659, 1e-6));
                CHECK_THAT(std::arg(values(11, 0)), WithinAbs(0.37770760072846, 1e-6));
                CHECK_THAT(std::arg(values(11, 1)), WithinAbs(-2.64634107528704, 1e-6));
                CHECK_THAT(std::arg(values(11, 2)), WithinAbs(0.575625867869018, 1e-6));
                CHECK_THAT(std::arg(values(11, 3)), WithinAbs(0.514275616713732, 1e-6));
                CHECK_THAT(std::arg(values(11, 4)), WithinAbs(0.514275616713732, 1e-6));
                CHECK_THAT(std::arg(values(11, 5)), WithinAbs(0.575625867869018, 1e-6));
                CHECK_THAT(std::arg(values(11, 6)), WithinAbs(-2.64634107528754, 1e-6));
                CHECK_THAT(std::arg(values(11, 7)), WithinAbs(0.37770760072846, 1e-6));
                CHECK_THAT(std::arg(values(12, 0)), WithinAbs(0.377707600724902, 1e-6));
                CHECK_THAT(std::arg(values(12, 1)), WithinAbs(-2.64634107528776, 1e-6));
                CHECK_THAT(std::arg(values(12, 2)), WithinAbs(0.575625867868195, 1e-6));
                CHECK_THAT(std::arg(values(12, 3)), WithinAbs(0.514275616712955, 1e-6));
                CHECK_THAT(std::arg(values(12, 4)), WithinAbs(0.514275616712955, 1e-6));
                CHECK_THAT(std::arg(values(12, 5)), WithinAbs(0.575625867868194, 1e-6));
                CHECK_THAT(std::arg(values(12, 6)), WithinAbs(-2.64634107528776, 1e-6));
                CHECK_THAT(std::arg(values(12, 7)), WithinAbs(0.377707600724902, 1e-6));
                CHECK_THAT(std::arg(values(13, 0)), WithinAbs(0.377707600727274, 1e-6));
                CHECK_THAT(std::arg(values(13, 1)), WithinAbs(-2.64634107528773, 1e-6));
                CHECK_THAT(std::arg(values(13, 2)), WithinAbs(0.575625867868465, 1e-6));
                CHECK_THAT(std::arg(values(13, 3)), WithinAbs(0.514275616713431, 1e-6));
                CHECK_THAT(std::arg(values(13, 4)), WithinAbs(0.514275616713431, 1e-6));
                CHECK_THAT(std::arg(values(13, 5)), WithinAbs(0.575625867868464, 1e-6));
                CHECK_THAT(std::arg(values(13, 6)), WithinAbs(-2.64634107528773, 1e-6));
                CHECK_THAT(std::arg(values(13, 7)), WithinAbs(0.377707600727276, 1e-6));
                CHECK_THAT(std::arg(values(14, 0)), WithinAbs(0.377707600730268, 1e-6));
                CHECK_THAT(std::arg(values(14, 1)), WithinAbs(-2.64634107528804, 1e-6));
                CHECK_THAT(std::arg(values(14, 2)), WithinAbs(0.575625867868742, 1e-6));
                CHECK_THAT(std::arg(values(14, 3)), WithinAbs(0.514275616713096, 1e-6));
                CHECK_THAT(std::arg(values(14, 4)), WithinAbs(0.514275616713161, 1e-6));
                CHECK_THAT(std::arg(values(14, 5)), WithinAbs(0.575625867868742, 1e-6));
                CHECK_THAT(std::arg(values(14, 6)), WithinAbs(-2.64634107528794, 1e-6));
                CHECK_THAT(std::arg(values(14, 7)), WithinAbs(0.377707600730268, 1e-6));
                CHECK_THAT(std::arg(values(15, 0)), WithinAbs(0.377707600726309, 1e-6));
                CHECK_THAT(std::arg(values(15, 1)), WithinAbs(-2.64634107528761, 1e-6));
                CHECK_THAT(std::arg(values(15, 2)), WithinAbs(0.575625867868319, 1e-6));
                CHECK_THAT(std::arg(values(15, 3)), WithinAbs(0.514275616713248, 1e-6));
                CHECK_THAT(std::arg(values(15, 4)), WithinAbs(0.514275616713248, 1e-6));
                CHECK_THAT(std::arg(values(15, 5)), WithinAbs(0.575625867868319, 1e-6));
                CHECK_THAT(std::arg(values(15, 6)), WithinAbs(-2.64634107528697, 1e-6));
                CHECK_THAT(std::arg(values(15, 7)), WithinAbs(0.377707600726311, 1e-6));
            }

            // Verify page 2
            {
                auto& values = data[2];

                // Next, find the maximum of the physical values and normalize for better comparison
                double const abs_max = normalize(values);
                CHECK_THAT(abs_max, WithinAbs(0.04147891840252747, 1e-9));

                // Verify physical values along the surface
                CHECK_THAT(std::abs(values(0, 0)), WithinAbs(0.18273276267708, 1e-9));
                CHECK_THAT(std::abs(values(0, 1)), WithinAbs(0.0933965070365704, 1e-9));
                CHECK_THAT(std::abs(values(0, 2)), WithinAbs(0.429713741670144, 1e-9));
                CHECK_THAT(std::abs(values(0, 3)), WithinAbs(0.999999999999868, 1e-9));
                CHECK_THAT(std::abs(values(0, 4)), WithinAbs(0.999999999999868, 1e-9));
                CHECK_THAT(std::abs(values(0, 5)), WithinAbs(0.429713741670144, 1e-9));
                CHECK_THAT(std::abs(values(0, 6)), WithinAbs(0.0933965070366334, 1e-9));
                CHECK_THAT(std::abs(values(0, 7)), WithinAbs(0.18273276267708, 1e-9));
                CHECK_THAT(std::abs(values(1, 0)), WithinAbs(0.182732762677051, 1e-9));
                CHECK_THAT(std::abs(values(1, 1)), WithinAbs(0.0933965070366252, 1e-9));
                CHECK_THAT(std::abs(values(1, 2)), WithinAbs(0.42971374167009, 1e-9));
                CHECK_THAT(std::abs(values(1, 3)), WithinAbs(0.999999999999903, 1e-9));
                CHECK_THAT(std::abs(values(1, 4)), WithinAbs(0.999999999999907, 1e-9));
                CHECK_THAT(std::abs(values(1, 5)), WithinAbs(0.42971374167009, 1e-9));
                CHECK_THAT(std::abs(values(1, 6)), WithinAbs(0.0933965070366272, 1e-9));
                CHECK_THAT(std::abs(values(1, 7)), WithinAbs(0.182732762677051, 1e-9));
                CHECK_THAT(std::abs(values(2, 0)), WithinAbs(0.182732762677067, 1e-9));
                CHECK_THAT(std::abs(values(2, 1)), WithinAbs(0.0933965070366563, 1e-9));
                CHECK_THAT(std::abs(values(2, 2)), WithinAbs(0.429713741670057, 1e-9));
                CHECK_THAT(std::abs(values(2, 3)), WithinAbs(0.999999999999891, 1e-9));
                CHECK_THAT(std::abs(values(2, 4)), WithinAbs(0.999999999999891, 1e-9));
                CHECK_THAT(std::abs(values(2, 5)), WithinAbs(0.429713741670057, 1e-9));
                CHECK_THAT(std::abs(values(2, 6)), WithinAbs(0.0933965070366563, 1e-9));
                CHECK_THAT(std::abs(values(2, 7)), WithinAbs(0.182732762677067, 1e-9));
                CHECK_THAT(std::abs(values(3, 0)), WithinAbs(0.182732762676932, 1e-9));
                CHECK_THAT(std::abs(values(3, 1)), WithinAbs(0.0933965070365914, 1e-9));
                CHECK_THAT(std::abs(values(3, 2)), WithinAbs(0.429713741670191, 1e-9));
                CHECK_THAT(std::abs(values(3, 3)), WithinAbs(0.999999999999951, 1e-9));
                CHECK_THAT(std::abs(values(3, 4)), WithinAbs(0.999999999999951, 1e-9));
                CHECK_THAT(std::abs(values(3, 5)), WithinAbs(0.429713741670191, 1e-9));
                CHECK_THAT(std::abs(values(3, 6)), WithinAbs(0.0933965070365914, 1e-9));
                CHECK_THAT(std::abs(values(3, 7)), WithinAbs(0.182732762676932, 1e-9));
                CHECK_THAT(std::abs(values(4, 0)), WithinAbs(0.182732762677024, 1e-9));
                CHECK_THAT(std::abs(values(4, 1)), WithinAbs(0.0933965070366629, 1e-9));
                CHECK_THAT(std::abs(values(4, 2)), WithinAbs(0.429713741670126, 1e-9));
                CHECK_THAT(std::abs(values(4, 3)), WithinAbs(0.999999999999872, 1e-9));
                CHECK_THAT(std::abs(values(4, 4)), WithinAbs(0.999999999999872, 1e-9));
                CHECK_THAT(std::abs(values(4, 5)), WithinAbs(0.429713741670126, 1e-9));
                CHECK_THAT(std::abs(values(4, 6)), WithinAbs(0.093396507036612, 1e-9));
                CHECK_THAT(std::abs(values(4, 7)), WithinAbs(0.182732762677024, 1e-9));
                CHECK_THAT(std::abs(values(5, 0)), WithinAbs(0.182732762677045, 1e-9));
                CHECK_THAT(std::abs(values(5, 1)), WithinAbs(0.0933965070366875, 1e-9));
                CHECK_THAT(std::abs(values(5, 2)), WithinAbs(0.429713741670141, 1e-9));
                CHECK_THAT(std::abs(values(5, 3)), WithinAbs(0.999999999999911, 1e-9));
                CHECK_THAT(std::abs(values(5, 4)), WithinAbs(0.999999999999911, 1e-9));
                CHECK_THAT(std::abs(values(5, 5)), WithinAbs(0.429713741670141, 1e-9));
                CHECK_THAT(std::abs(values(5, 6)), WithinAbs(0.0933965070366601, 1e-9));
                CHECK_THAT(std::abs(values(5, 7)), WithinAbs(0.182732762677045, 1e-9));
                CHECK_THAT(std::abs(values(6, 0)), WithinAbs(0.18273276267704, 1e-9));
                CHECK_THAT(std::abs(values(6, 1)), WithinAbs(0.0933965070367514, 1e-9));
                CHECK_THAT(std::abs(values(6, 2)), WithinAbs(0.429713741670093, 1e-9));
                CHECK_THAT(std::abs(values(6, 3)), WithinAbs(0.999999999999972, 1e-9));
                CHECK_THAT(std::abs(values(6, 4)), WithinAbs(1, 1e-9));
                CHECK_THAT(std::abs(values(6, 5)), WithinAbs(0.429713741670093, 1e-9));
                CHECK_THAT(std::abs(values(6, 6)), WithinAbs(0.0933965070367087, 1e-9));
                CHECK_THAT(std::abs(values(6, 7)), WithinAbs(0.18273276267704, 1e-9));
                CHECK_THAT(std::abs(values(7, 0)), WithinAbs(0.182732762677094, 1e-9));
                CHECK_THAT(std::abs(values(7, 1)), WithinAbs(0.0933965070367039, 1e-9));
                CHECK_THAT(std::abs(values(7, 2)), WithinAbs(0.429713741670054, 1e-9));
                CHECK_THAT(std::abs(values(7, 3)), WithinAbs(0.999999999999931, 1e-9));
                CHECK_THAT(std::abs(values(7, 4)), WithinAbs(0.999999999999931, 1e-9));
                CHECK_THAT(std::abs(values(7, 5)), WithinAbs(0.429713741670054, 1e-9));
                CHECK_THAT(std::abs(values(7, 6)), WithinAbs(0.0933965070366516, 1e-9));
                CHECK_THAT(std::abs(values(7, 7)), WithinAbs(0.182732762677094, 1e-9));
                CHECK_THAT(std::abs(values(8, 0)), WithinAbs(0.182732762677094, 1e-9));
                CHECK_THAT(std::abs(values(8, 1)), WithinAbs(0.0933965070367039, 1e-9));
                CHECK_THAT(std::abs(values(8, 2)), WithinAbs(0.429713741670054, 1e-9));
                CHECK_THAT(std::abs(values(8, 3)), WithinAbs(0.999999999999931, 1e-9));
                CHECK_THAT(std::abs(values(8, 4)), WithinAbs(0.999999999999931, 1e-9));
                CHECK_THAT(std::abs(values(8, 5)), WithinAbs(0.429713741670054, 1e-9));
                CHECK_THAT(std::abs(values(8, 6)), WithinAbs(0.0933965070366516, 1e-9));
                CHECK_THAT(std::abs(values(8, 7)), WithinAbs(0.182732762677094, 1e-9));
                CHECK_THAT(std::abs(values(9, 0)), WithinAbs(0.18273276267704, 1e-9));
                CHECK_THAT(std::abs(values(9, 1)), WithinAbs(0.0933965070367514, 1e-9));
                CHECK_THAT(std::abs(values(9, 2)), WithinAbs(0.429713741670093, 1e-9));
                CHECK_THAT(std::abs(values(9, 3)), WithinAbs(0.999999999999972, 1e-9));
                CHECK_THAT(std::abs(values(9, 4)), WithinAbs(1, 1e-9));
                CHECK_THAT(std::abs(values(9, 5)), WithinAbs(0.429713741670093, 1e-9));
                CHECK_THAT(std::abs(values(9, 6)), WithinAbs(0.0933965070367087, 1e-9));
                CHECK_THAT(std::abs(values(9, 7)), WithinAbs(0.18273276267704, 1e-9));
                CHECK_THAT(std::abs(values(10, 0)), WithinAbs(0.182732762677045, 1e-9));
                CHECK_THAT(std::abs(values(10, 1)), WithinAbs(0.0933965070366875, 1e-9));
                CHECK_THAT(std::abs(values(10, 2)), WithinAbs(0.429713741670141, 1e-9));
                CHECK_THAT(std::abs(values(10, 3)), WithinAbs(0.999999999999911, 1e-9));
                CHECK_THAT(std::abs(values(10, 4)), WithinAbs(0.999999999999911, 1e-9));
                CHECK_THAT(std::abs(values(10, 5)), WithinAbs(0.429713741670141, 1e-9));
                CHECK_THAT(std::abs(values(10, 6)), WithinAbs(0.0933965070366601, 1e-9));
                CHECK_THAT(std::abs(values(10, 7)), WithinAbs(0.182732762677045, 1e-9));
                CHECK_THAT(std::abs(values(11, 0)), WithinAbs(0.182732762677025, 1e-9));
                CHECK_THAT(std::abs(values(11, 1)), WithinAbs(0.0933965070366845, 1e-9));
                CHECK_THAT(std::abs(values(11, 2)), WithinAbs(0.429713741670111, 1e-9));
                CHECK_THAT(std::abs(values(11, 3)), WithinAbs(0.999999999999891, 1e-9));
                CHECK_THAT(std::abs(values(11, 4)), WithinAbs(0.999999999999891, 1e-9));
                CHECK_THAT(std::abs(values(11, 5)), WithinAbs(0.429713741670111, 1e-9));
                CHECK_THAT(std::abs(values(11, 6)), WithinAbs(0.0933965070367077, 1e-9));
                CHECK_THAT(std::abs(values(11, 7)), WithinAbs(0.182732762677025, 1e-9));
                CHECK_THAT(std::abs(values(12, 0)), WithinAbs(0.182732762676932, 1e-9));
                CHECK_THAT(std::abs(values(12, 1)), WithinAbs(0.0933965070365915, 1e-9));
                CHECK_THAT(std::abs(values(12, 2)), WithinAbs(0.429713741670191, 1e-9));
                CHECK_THAT(std::abs(values(12, 3)), WithinAbs(0.999999999999951, 1e-9));
                CHECK_THAT(std::abs(values(12, 4)), WithinAbs(0.999999999999951, 1e-9));
                CHECK_THAT(std::abs(values(12, 5)), WithinAbs(0.429713741670191, 1e-9));
                CHECK_THAT(std::abs(values(12, 6)), WithinAbs(0.0933965070365914, 1e-9));
                CHECK_THAT(std::abs(values(12, 7)), WithinAbs(0.182732762676932, 1e-9));
                CHECK_THAT(std::abs(values(13, 0)), WithinAbs(0.182732762677067, 1e-9));
                CHECK_THAT(std::abs(values(13, 1)), WithinAbs(0.0933965070366563, 1e-9));
                CHECK_THAT(std::abs(values(13, 2)), WithinAbs(0.429713741670057, 1e-9));
                CHECK_THAT(std::abs(values(13, 3)), WithinAbs(0.999999999999891, 1e-9));
                CHECK_THAT(std::abs(values(13, 4)), WithinAbs(0.999999999999891, 1e-9));
                CHECK_THAT(std::abs(values(13, 5)), WithinAbs(0.429713741670057, 1e-9));
                CHECK_THAT(std::abs(values(13, 6)), WithinAbs(0.0933965070366563, 1e-9));
                CHECK_THAT(std::abs(values(13, 7)), WithinAbs(0.182732762677067, 1e-9));
                CHECK_THAT(std::abs(values(14, 0)), WithinAbs(0.182732762677051, 1e-9));
                CHECK_THAT(std::abs(values(14, 1)), WithinAbs(0.0933965070366252, 1e-9));
                CHECK_THAT(std::abs(values(14, 2)), WithinAbs(0.42971374167009, 1e-9));
                CHECK_THAT(std::abs(values(14, 3)), WithinAbs(0.999999999999903, 1e-9));
                CHECK_THAT(std::abs(values(14, 4)), WithinAbs(0.999999999999907, 1e-9));
                CHECK_THAT(std::abs(values(14, 5)), WithinAbs(0.42971374167009, 1e-9));
                CHECK_THAT(std::abs(values(14, 6)), WithinAbs(0.0933965070366271, 1e-9));
                CHECK_THAT(std::abs(values(14, 7)), WithinAbs(0.182732762677051, 1e-9));
                CHECK_THAT(std::abs(values(15, 0)), WithinAbs(0.18273276267708, 1e-9));
                CHECK_THAT(std::abs(values(15, 1)), WithinAbs(0.0933965070365703, 1e-9));
                CHECK_THAT(std::abs(values(15, 2)), WithinAbs(0.429713741670144, 1e-9));
                CHECK_THAT(std::abs(values(15, 3)), WithinAbs(0.999999999999868, 1e-9));
                CHECK_THAT(std::abs(values(15, 4)), WithinAbs(0.999999999999868, 1e-9));
                CHECK_THAT(std::abs(values(15, 5)), WithinAbs(0.429713741670144, 1e-9));
                CHECK_THAT(std::abs(values(15, 6)), WithinAbs(0.0933965070366334, 1e-9));
                CHECK_THAT(std::abs(values(15, 7)), WithinAbs(0.18273276267708, 1e-9));
                CHECK_THAT(std::arg(values(0, 0)), WithinAbs(1.55375648968762, 1e-6));
                CHECK_THAT(std::arg(values(0, 1)), WithinAbs(1.50296496872655, 1e-6));
                CHECK_THAT(std::arg(values(0, 2)), WithinAbs(-1.56778243604873, 1e-6));
                CHECK_THAT(std::arg(values(0, 3)), WithinAbs(-1.57833127447549, 1e-6));
                CHECK_THAT(std::arg(values(0, 4)), WithinAbs(-1.57833127447549, 1e-6));
                CHECK_THAT(std::arg(values(0, 5)), WithinAbs(-1.56778243604873, 1e-6));
                CHECK_THAT(std::arg(values(0, 6)), WithinAbs(1.50296496872701, 1e-6));
                CHECK_THAT(std::arg(values(0, 7)), WithinAbs(1.55375648968762, 1e-6));
                CHECK_THAT(std::arg(values(1, 0)), WithinAbs(1.55375648968767, 1e-6));
                CHECK_THAT(std::arg(values(1, 1)), WithinAbs(1.50296496872625, 1e-6));
                CHECK_THAT(std::arg(values(1, 2)), WithinAbs(-1.56778243604838, 1e-6));
                CHECK_THAT(std::arg(values(1, 3)), WithinAbs(-1.57833127447559, 1e-6));
                CHECK_THAT(std::arg(values(1, 4)), WithinAbs(-1.57833127447556, 1e-6));
                CHECK_THAT(std::arg(values(1, 5)), WithinAbs(-1.56778243604838, 1e-6));
                CHECK_THAT(std::arg(values(1, 6)), WithinAbs(1.50296496872654, 1e-6));
                CHECK_THAT(std::arg(values(1, 7)), WithinAbs(1.55375648968767, 1e-6));
                CHECK_THAT(std::arg(values(2, 0)), WithinAbs(1.55375648968793, 1e-6));
                CHECK_THAT(std::arg(values(2, 1)), WithinAbs(1.50296496872634, 1e-6));
                CHECK_THAT(std::arg(values(2, 2)), WithinAbs(-1.56778243604858, 1e-6));
                CHECK_THAT(std::arg(values(2, 3)), WithinAbs(-1.5783312744754, 1e-6));
                CHECK_THAT(std::arg(values(2, 4)), WithinAbs(-1.5783312744754, 1e-6));
                CHECK_THAT(std::arg(values(2, 5)), WithinAbs(-1.56778243604858, 1e-6));
                CHECK_THAT(std::arg(values(2, 6)), WithinAbs(1.50296496872634, 1e-6));
                CHECK_THAT(std::arg(values(2, 7)), WithinAbs(1.55375648968793, 1e-6));
                CHECK_THAT(std::arg(values(3, 0)), WithinAbs(1.5537564896873, 1e-6));
                CHECK_THAT(std::arg(values(3, 1)), WithinAbs(1.50296496872593, 1e-6));
                CHECK_THAT(std::arg(values(3, 2)), WithinAbs(-1.56778243604858, 1e-6));
                CHECK_THAT(std::arg(values(3, 3)), WithinAbs(-1.57833127447572, 1e-6));
                CHECK_THAT(std::arg(values(3, 4)), WithinAbs(-1.57833127447572, 1e-6));
                CHECK_THAT(std::arg(values(3, 5)), WithinAbs(-1.56778243604858, 1e-6));
                CHECK_THAT(std::arg(values(3, 6)), WithinAbs(1.50296496872593, 1e-6));
                CHECK_THAT(std::arg(values(3, 7)), WithinAbs(1.5537564896873, 1e-6));
                CHECK_THAT(std::arg(values(4, 0)), WithinAbs(1.55375648968684, 1e-6));
                CHECK_THAT(std::arg(values(4, 1)), WithinAbs(1.50296496872599, 1e-6));
                CHECK_THAT(std::arg(values(4, 2)), WithinAbs(-1.56778243604852, 1e-6));
                CHECK_THAT(std::arg(values(4, 3)), WithinAbs(-1.57833127447542, 1e-6));
                CHECK_THAT(std::arg(values(4, 4)), WithinAbs(-1.57833127447542, 1e-6));
                CHECK_THAT(std::arg(values(4, 5)), WithinAbs(-1.56778243604852, 1e-6));
                CHECK_THAT(std::arg(values(4, 6)), WithinAbs(1.50296496872616, 1e-6));
                CHECK_THAT(std::arg(values(4, 7)), WithinAbs(1.55375648968684, 1e-6));
                CHECK_THAT(std::arg(values(5, 0)), WithinAbs(1.5537564896872, 1e-6));
                CHECK_THAT(std::arg(values(5, 1)), WithinAbs(1.50296496872672, 1e-6));
                CHECK_THAT(std::arg(values(5, 2)), WithinAbs(-1.5677824360485, 1e-6));
                CHECK_THAT(std::arg(values(5, 3)), WithinAbs(-1.57833127447573, 1e-6));
                CHECK_THAT(std::arg(values(5, 4)), WithinAbs(-1.57833127447573, 1e-6));
                CHECK_THAT(std::arg(values(5, 5)), WithinAbs(-1.5677824360485, 1e-6));
                CHECK_THAT(std::arg(values(5, 6)), WithinAbs(1.50296496872668, 1e-6));
                CHECK_THAT(std::arg(values(5, 7)), WithinAbs(1.5537564896872, 1e-6));
                CHECK_THAT(std::arg(values(6, 0)), WithinAbs(1.55375648968717, 1e-6));
                CHECK_THAT(std::arg(values(6, 1)), WithinAbs(1.50296496872661, 1e-6));
                CHECK_THAT(std::arg(values(6, 2)), WithinAbs(-1.56778243604868, 1e-6));
                CHECK_THAT(std::arg(values(6, 3)), WithinAbs(-1.57833127447557, 1e-6));
                CHECK_THAT(std::arg(values(6, 4)), WithinAbs(-1.57833127447548, 1e-6));
                CHECK_THAT(std::arg(values(6, 5)), WithinAbs(-1.56778243604869, 1e-6));
                CHECK_THAT(std::arg(values(6, 6)), WithinAbs(1.50296496872633, 1e-6));
                CHECK_THAT(std::arg(values(6, 7)), WithinAbs(1.55375648968717, 1e-6));
                CHECK_THAT(std::arg(values(7, 0)), WithinAbs(1.55375648968761, 1e-6));
                CHECK_THAT(std::arg(values(7, 1)), WithinAbs(1.50296496872594, 1e-6));
                CHECK_THAT(std::arg(values(7, 2)), WithinAbs(-1.56778243604873, 1e-6));
                CHECK_THAT(std::arg(values(7, 3)), WithinAbs(-1.57833127447549, 1e-6));
                CHECK_THAT(std::arg(values(7, 4)), WithinAbs(-1.57833127447549, 1e-6));
                CHECK_THAT(std::arg(values(7, 5)), WithinAbs(-1.56778243604873, 1e-6));
                CHECK_THAT(std::arg(values(7, 6)), WithinAbs(1.50296496872613, 1e-6));
                CHECK_THAT(std::arg(values(7, 7)), WithinAbs(1.55375648968761, 1e-6));
                CHECK_THAT(std::arg(values(8, 0)), WithinAbs(1.55375648968761, 1e-6));
                CHECK_THAT(std::arg(values(8, 1)), WithinAbs(1.50296496872594, 1e-6));
                CHECK_THAT(std::arg(values(8, 2)), WithinAbs(-1.56778243604873, 1e-6));
                CHECK_THAT(std::arg(values(8, 3)), WithinAbs(-1.57833127447549, 1e-6));
                CHECK_THAT(std::arg(values(8, 4)), WithinAbs(-1.57833127447549, 1e-6));
                CHECK_THAT(std::arg(values(8, 5)), WithinAbs(-1.56778243604873, 1e-6));
                CHECK_THAT(std::arg(values(8, 6)), WithinAbs(1.50296496872613, 1e-6));
                CHECK_THAT(std::arg(values(8, 7)), WithinAbs(1.55375648968761, 1e-6));
                CHECK_THAT(std::arg(values(9, 0)), WithinAbs(1.55375648968717, 1e-6));
                CHECK_THAT(std::arg(values(9, 1)), WithinAbs(1.50296496872661, 1e-6));
                CHECK_THAT(std::arg(values(9, 2)), WithinAbs(-1.56778243604869, 1e-6));
                CHECK_THAT(std::arg(values(9, 3)), WithinAbs(-1.57833127447557, 1e-6));
                CHECK_THAT(std::arg(values(9, 4)), WithinAbs(-1.57833127447548, 1e-6));
                CHECK_THAT(std::arg(values(9, 5)), WithinAbs(-1.56778243604869, 1e-6));
                CHECK_THAT(std::arg(values(9, 6)), WithinAbs(1.50296496872633, 1e-6));
                CHECK_THAT(std::arg(values(9, 7)), WithinAbs(1.55375648968717, 1e-6));
                CHECK_THAT(std::arg(values(10, 0)), WithinAbs(1.5537564896872, 1e-6));
                CHECK_THAT(std::arg(values(10, 1)), WithinAbs(1.50296496872672, 1e-6));
                CHECK_THAT(std::arg(values(10, 2)), WithinAbs(-1.5677824360485, 1e-6));
                CHECK_THAT(std::arg(values(10, 3)), WithinAbs(-1.57833127447573, 1e-6));
                CHECK_THAT(std::arg(values(10, 4)), WithinAbs(-1.57833127447573, 1e-6));
                CHECK_THAT(std::arg(values(10, 5)), WithinAbs(-1.5677824360485, 1e-6));
                CHECK_THAT(std::arg(values(10, 6)), WithinAbs(1.50296496872668, 1e-6));
                CHECK_THAT(std::arg(values(10, 7)), WithinAbs(1.5537564896872, 1e-6));
                CHECK_THAT(std::arg(values(11, 0)), WithinAbs(1.55375648968751, 1e-6));
                CHECK_THAT(std::arg(values(11, 1)), WithinAbs(1.5029649687272, 1e-6));
                CHECK_THAT(std::arg(values(11, 2)), WithinAbs(-1.56778243604821, 1e-6));
                CHECK_THAT(std::arg(values(11, 3)), WithinAbs(-1.57833127447522, 1e-6));
                CHECK_THAT(std::arg(values(11, 4)), WithinAbs(-1.57833127447522, 1e-6));
                CHECK_THAT(std::arg(values(11, 5)), WithinAbs(-1.56778243604821, 1e-6));
                CHECK_THAT(std::arg(values(11, 6)), WithinAbs(1.50296496872635, 1e-6));
                CHECK_THAT(std::arg(values(11, 7)), WithinAbs(1.55375648968751, 1e-6));
                CHECK_THAT(std::arg(values(12, 0)), WithinAbs(1.5537564896873, 1e-6));
                CHECK_THAT(std::arg(values(12, 1)), WithinAbs(1.50296496872593, 1e-6));
                CHECK_THAT(std::arg(values(12, 2)), WithinAbs(-1.56778243604858, 1e-6));
                CHECK_THAT(std::arg(values(12, 3)), WithinAbs(-1.57833127447572, 1e-6));
                CHECK_THAT(std::arg(values(12, 4)), WithinAbs(-1.57833127447572, 1e-6));
                CHECK_THAT(std::arg(values(12, 5)), WithinAbs(-1.56778243604858, 1e-6));
                CHECK_THAT(std::arg(values(12, 6)), WithinAbs(1.50296496872593, 1e-6));
                CHECK_THAT(std::arg(values(12, 7)), WithinAbs(1.5537564896873, 1e-6));
                CHECK_THAT(std::arg(values(13, 0)), WithinAbs(1.55375648968793, 1e-6));
                CHECK_THAT(std::arg(values(13, 1)), WithinAbs(1.50296496872634, 1e-6));
                CHECK_THAT(std::arg(values(13, 2)), WithinAbs(-1.56778243604858, 1e-6));
                CHECK_THAT(std::arg(values(13, 3)), WithinAbs(-1.5783312744754, 1e-6));
                CHECK_THAT(std::arg(values(13, 4)), WithinAbs(-1.5783312744754, 1e-6));
                CHECK_THAT(std::arg(values(13, 5)), WithinAbs(-1.56778243604858, 1e-6));
                CHECK_THAT(std::arg(values(13, 6)), WithinAbs(1.50296496872634, 1e-6));
                CHECK_THAT(std::arg(values(13, 7)), WithinAbs(1.55375648968793, 1e-6));
                CHECK_THAT(std::arg(values(14, 0)), WithinAbs(1.55375648968767, 1e-6));
                CHECK_THAT(std::arg(values(14, 1)), WithinAbs(1.50296496872625, 1e-6));
                CHECK_THAT(std::arg(values(14, 2)), WithinAbs(-1.56778243604838, 1e-6));
                CHECK_THAT(std::arg(values(14, 3)), WithinAbs(-1.57833127447559, 1e-6));
                CHECK_THAT(std::arg(values(14, 4)), WithinAbs(-1.57833127447556, 1e-6));
                CHECK_THAT(std::arg(values(14, 5)), WithinAbs(-1.56778243604838, 1e-6));
                CHECK_THAT(std::arg(values(14, 6)), WithinAbs(1.50296496872654, 1e-6));
                CHECK_THAT(std::arg(values(14, 7)), WithinAbs(1.55375648968767, 1e-6));
                CHECK_THAT(std::arg(values(15, 0)), WithinAbs(1.55375648968762, 1e-6));
                CHECK_THAT(std::arg(values(15, 1)), WithinAbs(1.50296496872655, 1e-6));
                CHECK_THAT(std::arg(values(15, 2)), WithinAbs(-1.56778243604873, 1e-6));
                CHECK_THAT(std::arg(values(15, 3)), WithinAbs(-1.57833127447549, 1e-6));
                CHECK_THAT(std::arg(values(15, 4)), WithinAbs(-1.57833127447549, 1e-6));
                CHECK_THAT(std::arg(values(15, 5)), WithinAbs(-1.56778243604873, 1e-6));
                CHECK_THAT(std::arg(values(15, 6)), WithinAbs(1.50296496872701, 1e-6));
                CHECK_THAT(std::arg(values(15, 7)), WithinAbs(1.55375648968762, 1e-6));
            }
        }
    }
}
