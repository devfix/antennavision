//
// Created by Tristan Krause on 2026-08-17.
//

#include <print>
#include <random>
#include <nlohmann/json.hpp>
#include "codebooks/ula_da.hpp"
#include "components/antenna.hpp"
#include "components/radiatorarray.hpp"
#include "math/coords.hpp"
#include "three.hpp"

using namespace antennavision;
using namespace components;
using reference::Reference;

int main(int argc, char* argv[])
{
    std::filesystem::path path_codebook{argv[1]};
    std::println("path codebook: {}", path_codebook.string());
    std::size_t number_of_points = std::atol(argv[2]);
    std::println("number of points: {}", number_of_points);
    std::size_t number_of_bins = std::atol(argv[3]);
    std::println("number of bins: {}", number_of_bins);
    std::filesystem::path path_result{argv[4]};
    std::println("path result: {}", path_result.string());

    std::ifstream file(path_codebook);
    if (!file.is_open()) { throw SimulationError("Could not open codebook. Does the file exist?"); }
    auto const js = json::parse(file);
    file.close();

    std::size_t number_of_elements = js["n_elements"].get<std::size_t>();
    std::println("number of elements: {}", number_of_elements);
    double wavelength = js["wavelength"].get<double>();
    std::println("λ={:.06f}", wavelength);
    double r_max = js["r_max"].get<std::size_t>();
    std::println("r_max: {:.06} m | {:.02} λ", r_max,r_max/wavelength);


    auto codebook = codebooks::ULA_DA::from_json(js);

    std::array<Antenna, 2> antennas = {
        RadiatorArray::create({
            .type = RadiatorArray::Type::UniformLinearArray,
            .id = "tx",
            .origin_id = "",
            .rot = {},
            .prototype_desc =
                {
                    .type = Radiator::Type::IsotropicRadiator,
                },
            .parameters = RadiatorArray::UniformLinearParameters{.spacing = 0.5 * wavelength, .size = number_of_elements} //
        }),
        Radiator::create({
            .type = Radiator::Type::IsotropicRadiator,
            .id = "rx",
            .origin_id = "ref_rx", //
        }) //
    };
    Antenna& tx = antennas[0];
    Antenna& rx = antennas[1];
    std::array<Reference, 2> references = {
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
    setup::SimParams sim_params{.system_wavelength = wavelength,.enable_path_loss = false};
    Reference& ref_rx = references[0];

    three::export_context(ctx, sim_params.system_wavelength, "/tmp/objects.js");

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis_x(-r_max, r_max);
    std::uniform_real_distribution<> dis_y(0, r_max);

    std::vector<double> gains;
    gains.reserve(number_of_points);

    std::println("Computing gains for {} random points", number_of_points);
    std::cout << std::flush;
    while (gains.size() < number_of_points)
    {
        ref_rx.pos = {dis_x(gen), dis_y(gen), 0};
        // std::print("pos: x={:.04f} y={:.04f} z={:.04f}\n", ref_rx.pos.x, ref_rx.pos.y, ref_rx.pos.z);

        auto [r, _, angle] = math::spherical_from_cartesian_pos(ref_rx.pos);
        // std::print("r={:.03f} theta={:.03f}\n", r/wavelength, angle/pi);

        auto result = codebook.find_closest({r, angle});
        // std::cout << "Spatial Error (Distance): " << result.first << " meters" << '\n';

        auto gain = antenna::calc_power_gain(tx, rx, wavelength, result.second, {1}, sim_params);
        // std::cout << "gain: " << gain << '\n';
        gains.push_back(gain / static_cast<double>(number_of_elements));
    }

    std::println("Calculating CDF");

    std::vector<double> cdf;
    cdf.reserve(number_of_bins);
    for (std::size_t bin = 0; bin < number_of_bins; bin++)
    {
        double threshold = math::nidx(bin, number_of_bins);
        std::size_t n_bin = 0;
        for (auto gain : gains)
        {
            if (gain >= threshold) n_bin++;
        }
        cdf.push_back(static_cast<double>(n_bin) / static_cast<double>(number_of_points));
    }

    json result;
    result["cdf"] = cdf;
    std::ofstream ofs(path_result);
    json::to_msgpack(result, ofs);

    return 0;
}
