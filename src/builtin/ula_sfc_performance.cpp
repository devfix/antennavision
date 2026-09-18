//
// Created by core on 07.09.26.
//

#include <filesystem>
#include <nlohmann/json.hpp>
#include <print>
#include <random>
#include <span>
#include "codebooks/ula_sfc.hpp"
#include "components/antenna.hpp"
#include "components/radiatorarray.hpp"
#include "math/coords.hpp"
#include "three.hpp"

using namespace antennavision;
using namespace components;
using reference::Reference;

double constexpr WAVELENGTH = 1.0; // will cancel out in the end, but usefull for convenience

struct PolarPos
{
    double r;
    double polar;
};

namespace
{
    struct TTSSpace
    {
        [[nodiscard]] static TTSSpace create(double tilde_r_p_min, double tilde_r_p_max, double tilde_theta_p_min = -1, double tilde_theta_p_max = +1)
        {
            std::random_device rd;
            return {
                .generator = std::mt19937(rd()),
                .distr_tilde_r = std::uniform_real_distribution(tilde_r_p_min, tilde_r_p_max),
                .distr_tilde_polar = std::uniform_real_distribution(tilde_theta_p_min, tilde_theta_p_max),
            };
        }

        std::pair<Pos, PolarPos> random_pos()
        {
            double tilde_r = distr_tilde_r(generator);
            double tilde_polar = distr_tilde_polar(generator);
            double r = WAVELENGTH / tilde_r;
            double polar = std::acos(tilde_polar);
            Pos pos{r * tilde_polar, r * std::sqrt(1 - math::square(tilde_polar)), 0.0};
            return {pos, {r, polar}};
        }

        std::mt19937 generator;
        std::uniform_real_distribution<> distr_tilde_r;
        std::uniform_real_distribution<> distr_tilde_polar;
    };

    struct CartesianSpace
    {
        [[nodiscard]] static CartesianSpace create(double x_min, double x_max, double y_min, double y_max)
        {
            std::random_device rd;
            return {
                .generator = std::mt19937(rd()),
                .distr_x = std::uniform_real_distribution(-x_min, x_max),
                .distr_y = std::uniform_real_distribution(-y_min, y_max),
            };
        }

        std::pair<Pos, PolarPos> random_pos()
        {
            Pos pos{distr_x(generator), distr_y(generator), 0};
            auto [r, _, angle] = math::spherical_from_cartesian_pos(pos);
            return {pos, {r, angle}};
        }

        std::mt19937 generator;
        std::uniform_real_distribution<> distr_x;
        std::uniform_real_distribution<> distr_y;
    };

    struct PolarSpace
    {
        [[nodiscard]] static PolarSpace create(double r_min, double r_max, double polar_min, double polar_max)
        {
            std::random_device rd;
            return {
                .generator = std::mt19937(rd()),
                .distr_r = std::uniform_real_distribution(r_min, r_max),
                .distr_polar = std::uniform_real_distribution(polar_min, polar_max),
            };
        }

        std::pair<Pos, PolarPos> random_pos()
        {
            double r = distr_r(generator);
            double polar = distr_polar(generator);
            Pos pos{r * std::cos(polar), r * std::sin(polar), 0.0};
            return {pos, {r, polar}};
        }

        std::mt19937 generator;
        std::uniform_real_distribution<> distr_r;
        std::uniform_real_distribution<> distr_polar;
    };
} // namespace

using RandomSpace = std::variant<TTSSpace, CartesianSpace, PolarSpace>;

int main(int argc, char* argv[])
{
    std::span<char* const> args(argv, argc);
    if (args.size() <= 1)
    {
        std::println(
            "<cb path> <number of points> <number of bins> <enable path loss> <Space> <dim1 min> <dim1 max> <dim2 min> <dim2 max> <db min> <output path>");
        return EXIT_SUCCESS;
    }

    std::size_t idx_arg = 1;
    std::filesystem::path path_codebook{args.at(idx_arg++)};
    std::println("path codebook: {}", path_codebook.string());
    std::size_t number_of_points = std::stol(args.at(idx_arg++));
    std::println("number of points: {}", number_of_points);
    std::size_t number_of_bins = std::stol(args.at(idx_arg++));
    std::println("number of bins: {}", number_of_bins);
    bool enable_path_loss = static_cast<bool>(std::stoi(args.at(idx_arg++)));
    std::println("enable path loss: {}", enable_path_loss);
    std::string space = args.at(idx_arg++);
    std::println("space: {}", space);
    double dim1_min = std::stod(args.at(idx_arg++));
    std::println("dim1_min: {}", dim1_min);
    double dim1_max = std::stod(args.at(idx_arg++));
    std::println("dim1_max: {}", dim1_max);
    double dim2_min = std::stod(args.at(idx_arg++));
    std::println("dim2_min: {}", dim2_min);
    double dim2_max = std::stod(args.at(idx_arg++));
    std::println("dim2_max: {}", dim2_max);
    double db_min = std::stod(args.at(idx_arg++));
    std::println("db min: {}", db_min);
    std::filesystem::path path_result{args.at(idx_arg++)};
    std::println("path result: {}", path_result.string());

    auto codebook = codebooks::ULA_SFC::from_file(path_codebook);

    std::array<Antenna, 3> antennas = {
        RadiatorArray::create({
            .type = RadiatorArray::Type::UniformLinearArray,
            .id = "tx",
            .origin_id = "",
            .rot = {},
            .prototype_desc =
                {
                    .type = Radiator::Type::IsotropicRadiator,
                },
            .parameters = RadiatorArray::UniformLinearParameters{.spacing = 0.5 * WAVELENGTH, .size = codebook.ula_size} //
        }),
        Radiator::create({
            .type = Radiator::Type::IsotropicRadiator,
            .id = "tx_norm",
            .origin_id = "", //
        }),
        Radiator::create({
            .type = Radiator::Type::IsotropicRadiator,
            .id = "rx",
            .origin_id = "ref_rx", //
        }) //
    };
    Antenna& tx = antennas[0];
    Antenna& tx_norm = antennas[1];
    Antenna& rx = antennas[2];
    std::array references = {
        Reference{}, // create origin reference
        Reference::create( //
            "ref_rx",
            "",
            {},
            {} //
            ) //
    };
    antenna::rebind_origin_pointers(antennas, references);
    Context ctx{
        .codebooks = {},
        .variables = {},
        .references = references,
        .antennas = antennas,
        .geometries = {},
        .sweeps = {} //
    };
    setup::SimParams sim_params{.system_wavelength = WAVELENGTH, .enable_path_loss = enable_path_loss};
    Reference& ref_rx = references[1];

    three::export_context(ctx, sim_params.system_wavelength, "/tmp/objects-sfc.js");

    RandomSpace rs;
    if (space == "TTS")
        rs = TTSSpace::create(dim1_min, dim1_max, dim2_min, dim2_max);
    else if (space == "Cartesian")
        rs = CartesianSpace::create(dim1_min, dim1_max, dim2_min, dim2_max);
    else if (space == "Polar")
        rs = PolarSpace::create(dim1_min, dim1_max, dim2_min, dim2_max);
    else
        throw SimulationError("Invalid space name");

    std::vector<double> gains;
    gains.reserve(number_of_points);

    std::println("Computing gains for {} random points", number_of_points);
    std::cout << std::flush;
    double gain_min = static_cast<double>(codebook.ula_size);
    double gain_max = 0;
    PolarPos pos_gain_min{};

    auto progress_steps = static_cast<std::size_t>(std::round(static_cast<double>(number_of_points) / 1000.0));

    while (gains.size() < number_of_points)
    {
        std::print("\033[2K\r{: 5.1f}%", 100.0 * static_cast<double>(gains.size()) / static_cast<double>(number_of_points));
        std::cout << std::flush;
        for (std::size_t k = 0; k < progress_steps and gains.size() < number_of_points; k++)
        {
            auto [cart_pos, polar_pos] = rs.visit([](auto& space) -> std::pair<Pos, PolarPos> { return space.random_pos(); });
            ref_rx.pos = cart_pos; // update receiver location

            // std::print("pos: x={:.04f} y={:.04f} z={:.04f}\n", ref_rx.pos.x, ref_rx.pos.y, ref_rx.pos.z);
            // std::print("r={:.03f} theta={:.03f}\n", polar_pos.r / WAVELENGTH, polar_pos.polar / pi);

            auto beamforming_vector = codebook.get_vector(polar_pos.r / WAVELENGTH, polar_pos.polar);

            auto voltage_gain = antenna::calc_voltage_gain(tx, rx, WAVELENGTH, beamforming_vector, {1}, sim_params);

            if (enable_path_loss)
            {
                auto gain_norm = antenna::calc_voltage_gain(tx_norm, rx, WAVELENGTH, {1}, {1}, sim_params);
                voltage_gain /= gain_norm;
            }
            double gain = math::square(std::abs(voltage_gain)) / static_cast<double>(codebook.ula_size);
            gain_max = std::max(gain_max, gain);
            if (gain < gain_min)
            {
                gain_min = gain;
                pos_gain_min = {polar_pos.r / WAVELENGTH, polar_pos.polar};
            }

            // std::cout << "gain: " << gain << '\n';
            gains.push_back(gain);
        }
    }
    std::println("100.0%");

    assert(gains.size() == number_of_points);

    std::println("min gain: {:.03f}", gain_min);
    std::println("max gain: {:.03f}", gain_max);

    std::println("Calculating complementary CDF");

    std::vector<std::pair<double, double>> ccdf_linear;
    ccdf_linear.reserve(number_of_bins);
    for (std::size_t bin = 0; bin < number_of_bins; bin++)
    {
        double threshold = math::nidx(bin, number_of_bins);
        std::size_t n_bin = 0;
        for (auto gain : gains)
        {
            if (gain >= threshold) n_bin++;
        }
        double val = static_cast<double>(n_bin) / static_cast<double>(number_of_points);
        ccdf_linear.emplace_back(threshold, val);
    }

    std::vector<double> gains_db;
    gains_db.reserve(gains.size());
    std::ranges::transform(gains, std::back_inserter(gains_db), [db_min](double g) { return std::max(10.0 * std::log10(g), db_min); });

    std::vector<std::pair<double, double>> ccdf_db;
    ccdf_db.reserve(number_of_bins);
    for (std::size_t bin = 0; bin < number_of_bins; bin++)
    {
        double threshold = std::lerp(db_min, 0, math::nidx(bin, number_of_bins));
        std::size_t n_bin = 0;
        for (auto gain_db : gains_db)
        {
            if (gain_db >= threshold) n_bin++;
        }
        double val = static_cast<double>(n_bin) / static_cast<double>(number_of_points);
        ccdf_db.emplace_back(threshold, val);
    }

    json result;
    result["gain_min"] = gain_min;
    result["pos_gain_min_r_rel"] = pos_gain_min.r;
    result["pos_gain_min_polar"] = pos_gain_min.polar;
    result["gain_max"] = gain_max;
    result["ccdf_linear"] = ccdf_linear;
    result["ccdf_db"] = ccdf_db;
    std::ofstream ofs(path_result);

    if (path_result.extension() == ".json")
        ofs << result.dump(2) << '\n';
    else
        json::to_msgpack(result, ofs);
}
