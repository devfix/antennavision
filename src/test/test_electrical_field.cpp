//
// Created by Tristan Krause on 2026-08-09.
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "components/radiator.hpp"
#include "math/coords.hpp"
#include "math/functions.hpp"
#include "setup/setup.hpp"
#include "testutil.hpp"

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

namespace
{
    struct TestPoint
    {
        std::string name;
        double r;
        double theta;
        double phi;
    };

    double constexpr frequency = 60e9;
    double constexpr wavelength = c0 / frequency;
    Complex constexpr coeff{1.0};
    auto constexpr sim_params = setup::SimParams{.system_wavelength = wavelength};
    double constexpr r_far_field = wavelength * 1000; // important: if r is too large, the magnitude-zero detection will fail!
    Complex constexpr i_exc = {1.5, 0.5}; // excitation current

    // vector of test points
    std::vector<TestPoint> const target_points = {
        {"Broadside (X-axis)", r_far_field, pi / 2.0, 0.0},
        {"Broadside (Y-axis)", r_far_field, pi / 2.0, pi / 2.0},
        {"Null Point (Z-axis)", r_far_field, 0.0, 0.0}, // Exact pole, E-field should be zero
        {"Diagonal 1 (theta=pi/4, phi=0)", r_far_field, pi / 4.0, 0.0},
        {"Diagonal 2 (theta=pi/4, phi=pi/4)", r_far_field, pi / 4.0, pi / 4.0} //
    };

    /**
     * @brief Computes the exact analytical far-field electric field vector for a Z-directed Hertzian dipole.
     * * Formula: E_theta = j * eta * (k * I_0 * l) / (4 * pi * r) * exp(-j * k * r) * sin(theta)
     * E_r = 0, E_phi = 0 (in the strict far-field limit)
     * * @param r Distance from origin (m)
     * @param theta Elevation angle from Z-axis (rad)
     * @param phi Azimuth angle from X-axis in XY plane (rad)
     * @param wavelength System wavelength (m)
     * @param i_exc Complex excitation current (A)
     * @return 3D Cartesian Electric Field Vector (Ex, Ey, Ez)
     */
    Vec calc_analytical_hertzian_far_field(double r, double theta, double phi, double wavelength, Complex i_exc)
    {
        double const k = 2.0 * pi / wavelength; // wave number

        // Far field magnitude/phase term for E_theta
        Complex const E_theta_mag = j * Z0 * (k * i_exc * components::Radiator::HERTZIAN_DIPOLE_LENGTH) / (4.0 * pi * r) * std::exp(-j * k * r) * std::sin(theta);

        // Convert spherical unit vector e_theta to Cartesian coordinates
        return nc::dot(math::get_rot_mat_from_spherical(theta, phi), math::vec<Complex>(0, E_theta_mag, 0));
    }
} // namespace

TEST_CASE("Electric Field: Z-Directed Hertzian Dipole in the Far-Field", "[radiator][hertzian_dipole][e_field]")
{
    // Hertzian Dipole aligned with z-axis, placed in the origin
    reference::Reference origin_ref{.id = "origin_ref"};
    components::Antenna ant = components::Radiator::HertzianDipole::create("dipole_tx", "origin_ref");
    components::antenna::rebind_origin_pointers({ant}, {origin_ref});

    for (auto const& pt : target_points)
    {
        SECTION(pt.name)
        {
            Pos const pos_cartesian = math::cartesian_from_spherical_pos(pt.r, pt.theta, pt.phi);
            Vec const field_expected = calc_analytical_hertzian_far_field(pt.r, pt.theta, pt.phi, wavelength, i_exc);
            Vec const field_actual = components::antenna::calc_electrical_field(ant, pos_cartesian, i_exc, wavelength, std::span{&coeff, 1}, sim_params);

            // Compare Magnitude
            double const mag_expected = std::abs(nc::norm(field_expected).item());
            double const mag_actual = std::abs(nc::norm(field_actual).item());

            if (mag_expected < 1e-9) // critical threshold, no global constant here!
            {
                // For the null point (Z-axis), expect near zero absolute value
                REQUIRE_THAT(mag_actual, WithinAbs(0.0, DELTA_MAG));
            }
            else
            {
                // For radiating directions, expect matching relative magnitude
                REQUIRE_THAT(mag_actual, WithinRel(mag_expected, EPSILON_MAG));

                // Check Vector Components (Complex X, Y, Z)
                CHECK_THAT(std::abs(field_actual[0].real() - field_expected[0].real()), WithinAbs(0.0, DELTA_MAG));
                CHECK_THAT(std::abs(field_actual[0].imag() - field_expected[0].imag()), WithinAbs(0.0, DELTA_MAG));
                CHECK_THAT(std::abs(field_actual[1].real() - field_expected[1].real()), WithinAbs(0.0, DELTA_MAG));
                CHECK_THAT(std::abs(field_actual[1].imag() - field_expected[1].imag()), WithinAbs(0.0, DELTA_MAG));
                CHECK_THAT(std::abs(field_actual[2].real() - field_expected[2].real()), WithinAbs(0.0, DELTA_MAG));
                CHECK_THAT(std::abs(field_actual[2].imag() - field_expected[2].imag()), WithinAbs(0.0, DELTA_MAG));
            }
        }
    }
}

TEST_CASE("Electric Field: Superposition of Z-Directed Hertzian Dipoles in the Far-Field", "[radiator][hertzian_dipole][e_field]")
{
    // Hertzian Dipoles aligned with z-axis, placed at different positions
    std::vector<reference::Reference> references{
        reference::Reference{.id = "ref1", .pos = {wavelength, 0, 0}},
        reference::Reference{.id = "ref2", .pos = {0, wavelength, 0}},
        reference::Reference{.id = "ref3", .pos = {0, 0, wavelength}} //
    };
    std::vector elements = {
        components::Radiator::HertzianDipole::create("tx1", "ref1"),
        components::Radiator::HertzianDipole::create("tx2", "ref2"),
        components::Radiator::HertzianDipole::create("tx3", "ref3") //
    };
    components::Antenna ant = components::RadiatorArray{
        .type = components::RadiatorArray::Type::CustomArray,
        .id = "array",
        .origin_id = "",
        .references = references,
        .elements = elements //
    };
    components::antenna::rebind_origin_pointers({ant}, {});
    std::array<Complex, 3> constexpr coeffs = {1.0, 1.0, 1.0};

    for (auto const& pt : target_points)
    {
        SECTION(pt.name)
        {
            Pos const pos_cartesian = math::cartesian_from_spherical_pos(pt.r, pt.theta, pt.phi);
            auto pos_spherical0 = math::spherical_from_cartesian_pos(pos_cartesian - references[0].pos);
            auto pos_spherical1 = math::spherical_from_cartesian_pos(pos_cartesian - references[1].pos);
            auto pos_spherical2 = math::spherical_from_cartesian_pos(pos_cartesian - references[2].pos);
            Vec const field_expected = //
                calc_analytical_hertzian_far_field(pos_spherical0[0], pos_spherical0[1], pos_spherical0[2], wavelength, i_exc) +
                calc_analytical_hertzian_far_field(pos_spherical1[0], pos_spherical1[1], pos_spherical1[2], wavelength, i_exc) +
                calc_analytical_hertzian_far_field(pos_spherical2[0], pos_spherical2[1], pos_spherical2[2], wavelength, i_exc);
            Vec const field_actual = components::antenna::calc_electrical_field(ant, pos_cartesian, i_exc, wavelength, coeffs, sim_params);

            // Compare Magnitude
            double const mag_expected = std::abs(nc::norm(field_expected).item());
            double const mag_actual = std::abs(nc::norm(field_actual).item());

            if (mag_expected < 1e-9) // critical threshold, no global constant here!
            {
                // For the null point (Z-axis), expect near zero absolute value
                REQUIRE_THAT(mag_actual, WithinAbs(0.0, DELTA_MAG));
            }
            else
            {
                // For radiating directions, expect matching relative magnitude
                REQUIRE_THAT(mag_actual, WithinRel(mag_expected, EPSILON_MAG));

                // Check Vector Components (Complex X, Y, Z)
                CHECK_THAT(std::abs(field_actual[0].real() - field_expected[0].real()), WithinAbs(0.0, DELTA_MAG));
                CHECK_THAT(std::abs(field_actual[0].imag() - field_expected[0].imag()), WithinAbs(0.0, DELTA_MAG));
                CHECK_THAT(std::abs(field_actual[1].real() - field_expected[1].real()), WithinAbs(0.0, DELTA_MAG));
                CHECK_THAT(std::abs(field_actual[1].imag() - field_expected[1].imag()), WithinAbs(0.0, DELTA_MAG));
                CHECK_THAT(std::abs(field_actual[2].real() - field_expected[2].real()), WithinAbs(0.0, DELTA_MAG));
                CHECK_THAT(std::abs(field_actual[2].imag() - field_expected[2].imag()), WithinAbs(0.0, DELTA_MAG));
            }
        }
    }
}
