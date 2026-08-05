//
// Created by Tristan Krause on 2026-07-07.
//

#include "codebook.hpp"
#include <print>
#include <nlohmann/json.hpp>
#include "simulationerror.hpp"

//
Codebook::Codebook(std::string_view id, std::filesystem::path const& p) : id(id)
{
    std::println("Loading codebook file '{}'", p.string());
    std::ifstream file(p);
    if (!file.is_open()) { throw SimulationError("Could not open codebook file '{}'", p.string()); }
    auto const js = nlohmann::json::parse(file);
    file.close();

    js.at("n_elements").get_to(n_elements_);
    js.at("oversampling_factor").get_to(oversampling_factor_);
    js.at("n_dim1").get_to(n_dim1_);
    js.at("n_dim2").get_to(n_dim2_);
    if (n_dim1_ * n_dim2_ != n_elements_)
        throw SimulationError("Codebook assertion failed: n_dim1 ({}) * n_dim2 ({}) ?= n_elements ({})", n_dim1_, n_dim2_, n_elements_);
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

