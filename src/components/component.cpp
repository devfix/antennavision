//
// Created by Tristan Krause on 2026-05-26.
//

#include "components/component.hpp"

Component::Component(std::string_view const id, std::size_t const num_inputs, std::size_t const num_outputs) : id(id), input_components(num_inputs, nullptr), output_components(num_outputs, nullptr) {}
