//
// Created by Tristan Krause on 2026-07-14.
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <nlohmann/json.hpp>
#include "../include/setup/setup.hpp"
#include "eval/voltagefield.hpp"

using Catch::Matchers::WithinAbs;
using geometry::Geometry;

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
  ],
  "tasks": [
    {
      "type": "builtin",
      "key": "t00_compare_beamwidth"
    }
  ]
}
)JSON");

TEST_CASE("ArgMax returns the correct position", "[ScalarField][VoltageField][ArgMax]")
{
    SECTION("Correct Maximum on geometry::Line")
    {
        Setup setup(SETUP_JSON);
        setup.export_to_three("/home/core");
        auto& wavelength = setup.num_params().system_wavelength;
        auto const distance = setup.get_double("distance");
        auto const& tx = setup.get_antenna("ula1");
        auto& rx = setup.get_antenna("receiver");

        auto voltage_field = VoltageField(tx, rx, setup.num_params());
        voltage_field.num_params.n_linear1 = 101;
        {
            auto line = geometry::Line("", Pos(0, distance, -0.5 * distance), Pos(0, distance, 0.5 * distance));
            auto result = voltage_field.argmax_curve_abs(line, wavelength);
            CHECK_THAT(result.pos.x, WithinAbs(0.0, 1e-9));
            CHECK_THAT(result.pos.y, WithinAbs(100.0, 1e-9));
            CHECK_THAT(result.pos.z, WithinAbs(0, 1e-9));
        }
    }

    SECTION("Correct Maximum on geometry::CircleArc")
    {
        Setup setup(SETUP_JSON);
        auto& wavelength = setup.num_params().system_wavelength;
        auto const distance = setup.get_double("distance");
        auto const& tx = setup.get_antenna("ula1");
        auto& rx = setup.get_antenna("receiver");

        auto voltage_field = VoltageField(tx, rx, setup.num_params());
        {
            auto arc = geometry::CircleArc("", POS_ZERO, Pos(1.0, 0.0, 0.0), Pos(0.0, distance, 0), POS_ZERO, distance, 0.5 * pi).normalized();
            auto result = voltage_field.argmax_curve_abs(arc, wavelength);
            CHECK_THAT(result.pos.x, WithinAbs(0.0, 1e-9));
            CHECK_THAT(result.pos.y, WithinAbs(100.0, 1e-9));
            CHECK_THAT(result.pos.z, WithinAbs(0, 1e-9));
        }
    }
}

TEST_CASE("beamwidth", "[ScalarField][VoltageField][beamwidth]")
{
    Setup setup(SETUP_JSON);
    auto& wavelength = setup.num_params().system_wavelength;
    auto const distance = setup.get_double("distance");
    auto const& tx = setup.get_antenna("ula1");
    auto& rx = setup.get_antenna("receiver");

    auto voltage_field = VoltageField(tx, rx, setup.num_params());
    {
        auto arc = geometry::CircleArc("", POS_ZERO, Pos(1.0, 0.0, 0.0), Pos(0.0, 1.0, 0.0), POS_ZERO, distance, 0.5 * pi).normalized();
        auto [pos_beam, beamwidth] = voltage_field.calc_beamwidth(arc, wavelength, sqrt2_2);
        CHECK_THAT(beamwidth, WithinAbs(0.10915247360799513, 1e-9));
    }
}

TEST_CASE("VoltageField eval_geometry and eval_geometry_sweep over all geometries", "[ScalarField][VoltageField][eval_geometry]")
{
    using std::ranges::max;
    using std::ranges::transform;

    Setup setup(SETUP_JSON);
    auto& wavelength = setup.num_params().system_wavelength;
    auto const distance = setup.get_double("distance");
    auto const& tx = setup.get_antenna("ula1");
    auto& rx = setup.get_antenna("receiver");

    auto voltage_field = VoltageField(tx, rx, setup.num_params());
    auto& num_params = voltage_field.num_params;
    num_params.n_linear1 = 5;
    num_params.n_linear2 = 4;

    // Configure a simple sweep with 3 test frequencies/wavelengths
    auto const sweep = sweep::ListSweep{"test_sweep", {wavelength, 1.5 * wavelength, 2 * wavelength}};

    SECTION("Evaluation over Line geometry")
    {
        Geometry const line = geometry::Line("", Pos(0, distance, -0.5 * distance), Pos(0, distance, 0.5 * distance));

        SECTION("Single wavelength evaluation (eval_geometry)")
        {
            auto [positions, values] = voltage_field.eval_geometry(line, wavelength);

            // Verify array shapes: curves generate shape (n_linear1, 1)
            REQUIRE(positions.shape().rows == num_params.n_linear1);
            REQUIRE(positions.shape().cols == 1);
            REQUIRE(values.shape().rows == num_params.n_linear1);
            REQUIRE(values.shape().cols == 1);

            // Verify positions along the line
            CHECK_THAT(positions(0, 0).z, WithinAbs(-50.0, 1e-9));
            CHECK_THAT(positions(1, 0).z, WithinAbs(-25.0, 1e-9));
            CHECK_THAT(positions(2, 0).z, WithinAbs(00.0, 1e-9));
            CHECK_THAT(positions(3, 0).z, WithinAbs(25.0, 1e-9));
            CHECK_THAT(positions(4, 0).z, WithinAbs(50.0, 1e-9));

            // First, find the maximum of the physical values and normalize for better comparison
            double const abs_max = std::abs(max(values, {}, [](Complex const& v) -> double { return std::abs(v); }));
            CHECK_THAT(abs_max, WithinAbs(0.00816237224653571, 1e-9));
            transform(values, values.begin(), [abs_max](auto v) -> Complex { return v / abs_max; }); // normalize

            // Verify physical values along the curve
            CHECK_THAT(std::abs(values(0, 0)), WithinAbs(0.03372955272133785, 1e-9));
            CHECK_THAT(std::arg(values(0, 0)), WithinAbs(1.32786841400003786, 1e-9));
            CHECK_THAT(std::abs(values(1, 0)), WithinAbs(0.02389929109182131, 1e-9));
            CHECK_THAT(std::arg(values(1, 0)), WithinAbs(3.05394203080116133, 1e-9));
            CHECK_THAT(std::abs(values(2, 0)), WithinAbs(1.00000000000000000, 1e-9));
            CHECK_THAT(std::arg(values(2, 0)), WithinAbs(-1.58748534552424547, 1e-9));
            CHECK_THAT(std::abs(values(3, 0)), WithinAbs(0.02389929109182135, 1e-9));
            CHECK_THAT(std::arg(values(3, 0)), WithinAbs(3.05394203080116311, 1e-9));
            CHECK_THAT(std::abs(values(4, 0)), WithinAbs(0.03372955272133784, 1e-9));
            CHECK_THAT(std::arg(values(4, 0)), WithinAbs(1.32786841400003675, 1e-9));
        }

        SECTION("Wavelength sweep evaluation (eval_geometry_sweep)")
        {
            auto [positions, data] = voltage_field.eval_geometry_sweep(line, sweep);

            // Verify array shapes: curves generate shape (n_linear1, 1)
            REQUIRE(positions.shape().rows == num_params.n_linear1);
            REQUIRE(positions.shape().cols == 1);
            REQUIRE(data.size() == sweep.size());
            for (std::size_t page = 0; page < sweep.size(); ++page)
            {
                REQUIRE(data[page].shape().rows == num_params.n_linear1);
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
                double const abs_max = std::abs(max(values, {}, [](Complex const& v) -> double { return std::abs(v); }));
                CHECK_THAT(abs_max, WithinAbs(0.00816237224653571, 1e-9));
                transform(values, values.begin(), [abs_max](auto v) -> Complex { return v / abs_max; }); // normalize

                // Verify physical values along the curve
                CHECK_THAT(std::abs(values(0, 0)), WithinAbs(0.03372955272133785, 1e-9));
                CHECK_THAT(std::arg(values(0, 0)), WithinAbs(1.32786841400003786, 1e-9));
                CHECK_THAT(std::abs(values(1, 0)), WithinAbs(0.02389929109182131, 1e-9));
                CHECK_THAT(std::arg(values(1, 0)), WithinAbs(3.05394203080116133, 1e-9));
                CHECK_THAT(std::abs(values(2, 0)), WithinAbs(1.00000000000000000, 1e-9));
                CHECK_THAT(std::arg(values(2, 0)), WithinAbs(-1.58748534552424547, 1e-9));
                CHECK_THAT(std::abs(values(3, 0)), WithinAbs(0.02389929109182135, 1e-9));
                CHECK_THAT(std::arg(values(3, 0)), WithinAbs(3.05394203080116311, 1e-9));
                CHECK_THAT(std::abs(values(4, 0)), WithinAbs(0.03372955272133784, 1e-9));
                CHECK_THAT(std::arg(values(4, 0)), WithinAbs(1.32786841400003675, 1e-9));
            }

            // Verify page 1
            {
                auto& values = data[1];

                // First, find the maximum of the physical values and normalize for better comparison
                double const abs_max = std::abs(max(values, {}, [](Complex const& v) -> double { return std::abs(v); }));
                CHECK_THAT(abs_max, WithinAbs(0.02659651050178366, 1e-9));
                transform(values, values.begin(), [abs_max](auto v) -> Complex { return v / abs_max; }); // normalize

                // Verify physical values along the curve
                CHECK_THAT(std::abs(values(0, 0)), WithinAbs(0.07645830975130734, 1e-9));
                CHECK_THAT(std::arg(values(0, 0)), WithinAbs(2.44837837459381547, 1e-9));
                CHECK_THAT(std::abs(values(1, 0)), WithinAbs(0.17122098426494123, 1e-9));
                CHECK_THAT(std::arg(values(1, 0)), WithinAbs(0.37255907698265384, 1e-9));
                CHECK_THAT(std::abs(values(2, 0)), WithinAbs(1.0, 1e-9));
                CHECK_THAT(std::arg(values(2, 0)), WithinAbs(0.51247254004675225, 1e-9));
                CHECK_THAT(std::abs(values(3, 0)), WithinAbs(0.17122098426494126, 1e-9));
                CHECK_THAT(std::arg(values(3, 0)), WithinAbs(0.37255907698265373, 1e-9));
                CHECK_THAT(std::abs(values(4, 0)), WithinAbs(0.07645830975130731, 1e-9));
                CHECK_THAT(std::arg(values(4, 0)), WithinAbs(2.44837837459381547, 1e-9));
            }
            //
            // Verify page 2
            {
                auto& values = data[2];

                // First, find the maximum of the physical values and normalize for better comparison
                double const abs_max = std::abs(max(values, {}, [](Complex const& v) -> double { return std::abs(v); }));
                CHECK_THAT(abs_max, WithinAbs(0.04531595494707993, 1e-9));
                transform(values, values.begin(), [abs_max](auto v) -> Complex { return v / abs_max; }); // normalize

                // Verify physical values along the curve
                CHECK_THAT(std::abs(values(0, 0)), WithinAbs(0.07274536826095429, 1e-9));
                CHECK_THAT(std::arg(values(0, 0)), WithinAbs(1.46374975740341529, 1e-9));
                CHECK_THAT(std::abs(values(1, 0)), WithinAbs(0.02794423377475553, 1e-9));
                CHECK_THAT(std::arg(values(1, 0)), WithinAbs(2.451267059619334, 1e-9));
                CHECK_THAT(std::abs(values(2, 0)), WithinAbs(1.0, 1e-9));
                CHECK_THAT(std::arg(values(2, 0)), WithinAbs(-1.5791410520193585, 1e-9));
                CHECK_THAT(std::abs(values(3, 0)), WithinAbs(0.0279442337747555, 1e-9));
                CHECK_THAT(std::arg(values(3, 0)), WithinAbs(2.451267059619338, 1e-9));
                CHECK_THAT(std::abs(values(4, 0)), WithinAbs(0.07274536826095429, 1e-9));
                CHECK_THAT(std::arg(values(4, 0)), WithinAbs(1.46374975740341506, 1e-9));
            }
        }
    }

    SECTION("Evaluation over CircleArc geometry")
    {
        Geometry const arc = geometry::CircleArc("", POS_ZERO, Pos(1.0, 0.0, 0.0), Pos(0.0, distance, 0), POS_ZERO, distance, 0.5 * pi).normalized();

        SECTION("Single wavelength evaluation (eval_geometry)")
        {
            auto [positions, values] = voltage_field.eval_geometry(arc, wavelength);

            // Verify array shapes: curves generate shape (n_linear1, 1)
            REQUIRE(positions.shape().rows == num_params.n_linear1);
            REQUIRE(positions.shape().cols == 1);
            REQUIRE(values.shape().rows == num_params.n_linear1);
            REQUIRE(values.shape().cols == 1);

            // Verify position mapping along the arc (-90 deg to +90 deg)
            CHECK_THAT(positions(0, 0).z, WithinAbs(-70.71067811865474084, 1e-9)); // t = 0.0 -> -pi/2
            CHECK_THAT(positions(1, 0).z, WithinAbs(-38.26834323650897574, 1e-9)); // t = 0.25
            CHECK_THAT(positions(2, 0).z, WithinAbs(0.0, 1e-9)); // t = 0.5 -> 0 rad
            CHECK_THAT(positions(3, 0).z, WithinAbs(38.26834323650897574, 1e-9)); // t = 0.75
            CHECK_THAT(positions(4, 0).z, WithinAbs(70.71067811865474084, 1e-9)); // t = 1.0 -> +pi/2

            // First, find the maximum of the physical values and normalize for better comparison
            double const abs_max = std::abs(max(values, {}, [](Complex const& v) -> double { return std::abs(v); }));
            CHECK_THAT(abs_max, WithinAbs(0.00816237224653571, 1e-9));
            transform(values, values.begin(), [abs_max](auto v) -> Complex { return v / abs_max; }); // normalize

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
            auto [positions, data] = voltage_field.eval_geometry_sweep(arc, sweep);

            // Verify array shapes: curves generate shape (n_linear1, 1)
            REQUIRE(positions.shape().rows == num_params.n_linear1);
            REQUIRE(positions.shape().cols == 1);
            REQUIRE(data.size() == sweep.size());
            for (std::size_t page = 0; page < sweep.size(); ++page)
            {
                REQUIRE(data[page].shape().rows == num_params.n_linear1);
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
                double const abs_max = std::abs(max(values, {}, [](Complex const& v) -> double { return std::abs(v); }));
                CHECK_THAT(abs_max, WithinAbs(0.00816237224653571, 1e-9));
                transform(values, values.begin(), [abs_max](auto v) -> Complex { return v / abs_max; }); // normalize

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
                double const abs_max = std::abs(max(values, {}, [](Complex const& v) -> double { return std::abs(v); }));
                CHECK_THAT(abs_max, WithinAbs(0.02659651050178366, 1e-9));
                transform(values, values.begin(), [abs_max](auto v) -> Complex { return v / abs_max; }); // normalize

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
                double const abs_max = std::abs(max(values, {}, [](Complex const& v) -> double { return std::abs(v); }));
                CHECK_THAT(abs_max, WithinAbs(0.04531595494707993, 1e-9));
                transform(values, values.begin(), [abs_max](auto v) -> Complex { return v / abs_max; }); // normalize

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

    // SECTION("Evaluation over Rectangle geometry")
    // {
    //     Geometry const rect = geometry::Rectangle{
    //         "rect_01",
    //         {0.0, 0.0, 0.0},
    //         {0.0, 0.0, 1.0},
    //         {1.0, 0.0, 0.0},
    //         {0.0, 1.0, 0.0},
    //         10.0,
    //         20.0
    //     };
    //
    //     SECTION("Single wavelength evaluation (eval_geometry)")
    //     {
    //         auto const [positions, values] = field.eval_geometry(rect, test_wavelength);
    //
    //         // Verify array shapes: surfaces generate grid of shape (n_linear2, n_linear1)
    //         REQUIRE(positions.shape().rows == num_params.n_linear2);
    //         REQUIRE(positions.shape().cols == num_params.n_linear1);
    //         REQUIRE(values.shape().rows == num_params.n_linear2);
    //         REQUIRE(values.shape().cols == num_params.n_linear1);
    //
    //         // Verify grid corner positions
    //         CHECK_THAT(positions(0, 0).x, WithinAbs(-5.0, 1e-9));  // Bottom-left x
    //         CHECK_THAT(positions(0, 0).y, WithinAbs(-10.0, 1e-9)); // Bottom-left y
    //         CHECK_THAT(positions(num_params.n_linear2 - 1, num_params.n_linear1 - 1).x, WithinAbs(5.0, 1e-9));   // Top-right x
    //         CHECK_THAT(positions(num_params.n_linear2 - 1, num_params.n_linear1 - 1).y, WithinAbs(10.0, 1e-9));  // Top-right y
    //
    //         // TODO: Insert expected physical Complex field values below
    //         CHECK(values(0, 0) == Complex(0.0, 0.0)); // Bottom-left corner
    //         CHECK(values(num_params.n_linear2 / 2, num_params.n_linear1 / 2) == Complex(0.0, 0.0)); // Near center
    //         CHECK(values(num_params.n_linear2 - 1, num_params.n_linear1 - 1) == Complex(0.0, 0.0)); // Top-right corner
    //     }
    //
    //     SECTION("Wavelength sweep evaluation (eval_geometry_sweep)")
    //     {
    //         auto const [positions, data] = field.eval_geometry_sweep(rect, test_sweep);
    //
    //         REQUIRE(data.size() == expected_sweep_steps);
    //         for (std::size_t step = 0; step < expected_sweep_steps; ++step)
    //         {
    //             REQUIRE(data[step].shape().rows == num_params.n_linear2);
    //             REQUIRE(data[step].shape().cols == num_params.n_linear1);
    //
    //             // TODO: Insert expected sweep Complex field values for the surface grid
    //             CHECK(data[step](0, 0) == Complex(0.0, 0.0));
    //             CHECK(data[step](num_params.n_linear2 - 1, num_params.n_linear1 - 1) == Complex(0.0, 0.0));
    //         }
    //     }
    // }
    //
    // SECTION("Evaluation over SphericalRectangle geometry")
    // {
    //     double const pi = std::numbers::pi;
    //     Geometry const sr = geometry::SphericalRectangle{
    //         "sr_01",
    //         {0.0, 0.0, 0.0},
    //         {0.0, 0.0, 1.0},
    //         {1.0, 0.0, 0.0},
    //         {0.0, 1.0, 0.0},
    //         5.0,
    //         pi / 2.0,
    //         pi / 2.0
    //     };
    //
    //     SECTION("Single wavelength evaluation (eval_geometry)")
    //     {
    //         auto const [positions, values] = field.eval_geometry(sr, test_wavelength);
    //
    //         REQUIRE(positions.shape().rows == num_params.n_linear2);
    //         REQUIRE(positions.shape().cols == num_params.n_linear1);
    //         REQUIRE(values.shape().rows == num_params.n_linear2);
    //         REQUIRE(values.shape().cols == num_params.n_linear1);
    //
    //         // Verify all generated surface positions lie exactly on the sphere's radius
    //         for (RealArray::index_type r = 0; r < positions.shape().rows; ++r)
    //         {
    //             for (RealArray::index_type c = 0; c < positions.shape().cols; ++c)
    //             {
    //                 CHECK_THAT(positions(r, c).norm(), WithinAbs(5.0, 1e-9));
    //             }
    //         }
    //
    //         // TODO: Insert expected physical Complex field values below
    //         CHECK(values(0, 0) == Complex(0.0, 0.0));
    //         CHECK(values(0, num_params.n_linear1 - 1) == Complex(0.0, 0.0));
    //         CHECK(values(num_params.n_linear2 - 1, 0) == Complex(0.0, 0.0));
    //         CHECK(values(num_params.n_linear2 - 1, num_params.n_linear1 - 1) == Complex(0.0, 0.0));
    //     }
    //
    //     SECTION("Wavelength sweep evaluation (eval_geometry_sweep)")
    //     {
    //         auto const [positions, data] = field.eval_geometry_sweep(sr, test_sweep);
    //
    //         REQUIRE(data.size() == expected_sweep_steps);
    //         for (std::size_t step = 0; step < expected_sweep_steps; ++step)
    //         {
    //             REQUIRE(data[step].shape().rows == num_params.n_linear2);
    //             REQUIRE(data[step].shape().cols == num_params.n_linear1);
    //
    //             // TODO: Insert expected sweep Complex field values
    //             CHECK(data[step](0, 0) == Complex(0.0, 0.0));
    //         }
    //     }
    // }
}
