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
    void load_recursively(json const& js, Codebook::Node & node)
    {
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
                load_recursively(val, nodes.emplace_back(key));
            }
            node.element = std::move(nodes);
        } else throw SimulationError("Malformed codebook json");
    }
} // namespace

Codebook Codebook::from_json(std::string_view id, json const& js)
{
    try
    {
        decltype(n_elements) const n_elements = js.at("n_elements").get<decltype(n_elements)>();
        decltype(n_elements) const oversampling_factor = js.at("oversampling_factor").get<decltype(oversampling_factor)>();
        decltype(n_elements) const n_dim1 = js.at("n_dim1").get<decltype(n_dim1)>();
        decltype(n_elements) const n_dim2 = js.at("n_dim2").get<decltype(n_dim2)>();
        if (n_dim1 * n_dim2 != n_elements)
            throw SimulationError("Codebook assertion failed: n_dim1 ({}) * n_dim2 ({}) ?= n_elements ({})", n_dim1, n_dim2, n_elements);

        Node root{.element = std::vector<Node>{}};
        load_recursively(js.at("weights"), root);

        return Codebook{
            .id = std::string(id),
            .n_elements = n_elements,
            .oversampling_factor = oversampling_factor,
            .n_dim1 = n_dim1,
            .n_dim2 = n_dim2,
            .root = std::move(root)
        };
    } catch (...)
    {
        std::throw_with_nested(SimulationError("Failed to load codebook json:\n{}", js.dump(2)));
    }
}

Codebook Codebook::from_file(std::string_view id, std::filesystem::path const& p)
{
    try
    {
        std::ifstream file(p);
        if (!file.is_open()) { throw SimulationError("Could not open codebook. Does the file exist?"); }
        auto const js = json::parse(file);
        file.close();
        return from_json(id, js);
    } catch (...)
    {
        std::throw_with_nested(SimulationError("Failed to load codebook file '{}'", p.string()));
    }
}

std::span<Complex const> Codebook::operator[](std::span<std::string const> key) const
{
    Node const* current = &root;
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

