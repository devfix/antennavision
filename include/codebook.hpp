//
// Created by Tristan Krause on 2026-07-07.
//

#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>
#include "types/math.hpp"
#include "types/json.hpp"


struct Codebook
{
    struct Entry
    {
        std::vector<Complex> weights;
    };

    struct Node;
    using Element = std::variant<Entry, std::vector<Node>>;
    struct Node
    {
        std::string id{};
        Element element;
    };

    static Codebook from_json(std::string_view id, json const& js);
    static Codebook from_file(std::string_view id, std::filesystem::path const& p);

    std::span<Complex const> operator[](std::span<std::string const> key) const;

    std::string id;
    std::uint32_t n_elements;
    std::uint32_t oversampling_factor;
    std::size_t n_dim1;
    std::size_t n_dim2;
    Node root;
};
