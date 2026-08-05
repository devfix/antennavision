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

    Codebook(std::string_view id, std::filesystem::path const& p);

    [[nodiscard]] std::uint32_t n_elements() const { return n_elements_; }

    [[nodiscard]] std::uint32_t oversampling_factor() const { return oversampling_factor_; }

    [[nodiscard]] std::size_t n_dim1() const { return n_dim1_; }

    [[nodiscard]] std::size_t n_dim2() const { return n_dim2_; }

    std::span<Complex const> operator[](std::span<std::string const> key) const;

    std::string id;

private:
    Node root_{.element = std::vector<Node>{}};
    std::uint32_t n_elements_{};
    std::uint32_t oversampling_factor_{};
    std::size_t n_dim1_{};
    std::size_t n_dim2_{};
};
