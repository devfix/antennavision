//
// Created by Tristan Krause on 2026-05-26.
//


#pragma once
#include <string>
#include <string_view>
#include <vector>
#include "types.hpp"

struct Component {
    virtual ~Component() = default;
    std::string id;
    std::vector<Component*> input_components;
    std::vector<Component*> output_components;

    virtual complex_t calc_path(std::size_t idx_input, std::size_t idx_output) = 0;

protected:
    // protected constructor, only derived classes can instantiate it
    Component(std::string_view id,std::size_t num_inputs, std::size_t num_outputs);
};
