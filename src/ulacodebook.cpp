//
// Created by core on 07.07.26.
//

#include "ulacodebook.hpp"
#include <fstream>
#include <nlohmann/json.hpp>

#include "math.hpp"
#include "print.hpp"
#include "simulationerror.hpp"

UlaCodebook::UlaCodebook(std::filesystem::path const& p)
{
    std::println("Loading codebook file '{}'", p.string());
    std::ifstream file(p);
    if (!file.is_open()) { throw SimulationError("Could not open codebook file '{}'", p.string()); }
    auto const js = nlohmann::json::parse(file);
    file.close();

    n_elements = js.at("n_elements").get<decltype(n_elements)>();
    oversampling_factor = js.at("oversampling_factor").get<decltype(oversampling_factor)>();
    distance = js.at("distance").get<decltype(distance)>();
    wavelengths = js.at("wavelengths").get<decltype(wavelengths)>();
    for (const auto& [wavelength_key, sub_book] : js["codebook"].items())
    {
        std::uint32_t wavelength_idx = std::stol(wavelength_key);
        if (!codebook.contains(wavelength_idx)) { codebook[wavelength_idx] = decltype(codebook)::mapped_type(); }
        for (const auto& [entry_key, entry] : sub_book.items())
        {
            std::uint32_t entry_idx = std::stol(entry_key);
            auto const angle = entry.at("angle").get<decltype(Entry::angle)>();
            auto const focus_distance = entry.at("focus_distance").get<decltype(Entry::focus_distance)>();
            auto const weights_comp = entry.at("weights").get<std::vector<std::array<double, 2>>>();
            decltype(Entry::weights) weights(weights_comp.size(), 0.0);
            std::ranges::transform(weights_comp, weights.begin(), [](auto const& w) { return math::complex_from_polar(w.at(0), w.at(1)); });
            codebook[wavelength_idx].emplace(std::piecewise_construct, std::forward_as_tuple(entry_idx), std::forward_as_tuple(angle, focus_distance, weights));
        }
    }
}

std::vector<complex_t> UlaCodebook::get_steering_vector(double wavelength, double angle, double focus_distance)
{
    auto wavelength_idx = math::find_closest_index(wavelengths, wavelength);
    if (!wavelength_idx) { throw SimulationError("Codebook contains no wavelength"); }

    auto const sub_book = codebook.at(wavelength_idx.value());
    std::uint32_t best_entry_idx = 0;
    double min_diff = 2*pi;
    for (auto const& [entry_idx, entry] : sub_book)
    {
        if (auto const diff = std::abs(entry.angle - angle); diff < min_diff) { min_diff = diff; best_entry_idx = entry_idx; }
    }
    Entry const& entry = sub_book.at(best_entry_idx);
    return entry.weights;
}
