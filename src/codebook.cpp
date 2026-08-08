//
// Created by Tristan Krause on 2026-07-07.
//

#include "codebook.hpp"
#include <nlohmann/json.hpp>
#include <print>

#include "lg.hpp"
#include "math/coords.hpp"
#include "simulationerror.hpp"

namespace
{
    void load(json const& js, Codebook::Node & node)
    {
        // if (!js.is_d()) throw SimulationError("Expected array, but got {}", js.type_name());
        // if (js.empty()) throw SimulationError("Unexpected empty array");

        if (js.is_array())
        {
            auto const weights_comp = js.get<std::vector<std::array<double, 2>>>();
            Codebook::Entry entry{};
            entry.weights.resize(weights_comp.size(), 0.0);
            std::ranges::transform(weights_comp, entry.weights.begin(), [](auto const& w) -> Complex{ return math::complex_from_polar(w.at(0), w.at(1));; });
            node.element = std::move(entry);
        } else if (js.is_object())
        {
            auto nodes = std::vector<Codebook::Node>{};
            for (auto& [key, val] : js.items())
            {
                load(val, nodes.emplace_back(key));
            }
            node.element = std::move(nodes);
        } else throw SimulationError("Malformed codebook json");
    }
} // namespace

Codebook::Codebook(std::string_view id, std::filesystem::path const& p) : id(id)
{
    try
    {
        lg::println("Loading codebook file '{}'", std::filesystem::weakly_canonical(p).string());
        std::ifstream file(p);
        if (!file.is_open()) { throw SimulationError("Could not open codebook. Does the file exist?"); }
        auto const js = nlohmann::json::parse(file);
        file.close();

        js.at("n_elements").get_to(n_elements_);
        js.at("oversampling_factor").get_to(oversampling_factor_);
        js.at("n_dim1").get_to(n_dim1_);
        js.at("n_dim2").get_to(n_dim2_);
        if (n_dim1_ * n_dim2_ != n_elements_)
            throw SimulationError("Codebook assertion failed: n_dim1 ({}) * n_dim2 ({}) ?= n_elements ({})", n_dim1_, n_dim2_, n_elements_);

        load( js.at("weights"), root_);
    }
    catch (...)
    {
        std::throw_with_nested(SimulationError("Failed to load codebook file '{}'", p.string()));
    }
}

std::span<Complex const> Codebook::operator[](std::span<std::string const> key) const
{
    Node const* current = &root_;
    std::size_t depth = 0;
    for (auto const& id : key)
    {
        auto const& element = current->element;
        if (!std::holds_alternative<std::vector<Node>>(element)) throw SimulationError("Reached codebook entry too early at depth={} (id='{}')", depth, id);
        auto& children = std::get<std::vector<Node>>(element);

        auto it = std::ranges::find(children, id, &Node::id);
        if (it == children.end()) { throw SimulationError("Invalid codebook key at depth={} (id='{}')", depth, id); }

        current = std::to_address(it);
        depth++;
    }

    if (auto const* entry = std::get_if<Entry>(&current->element); !entry)
        throw SimulationError("Invalid codebook key: did not reach entry at depth={}')", depth);
    else
        return entry->weights;
}

