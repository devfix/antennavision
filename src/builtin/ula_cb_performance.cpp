//
// Created by Tristan Krause on 2026-08-17.
//

#include <iostream>
#include "codebooks/ula_da.hpp"

using namespace antennavision;

int main()
{
    // std::vector<std::pair<codebooks::ULA_DA::polar_pos, codebooks::ULA_DA::cb_vector>> mock_data = {
    //     {{10.0, 0.5}, {{1.0, 0.0}, {0.0, -1.0}}}, // r=10, theta=0.5
    //     {{15.0, 0.8}, {{0.7, 0.7}, {0.7, -0.7}}}  // r=15, theta=0.8
    // };

    auto codebook = codebooks::ULA_DA::from_file("/home/core/Documents/ws/def/tud-c/diploma-scripts/p4a.json");

    codebooks::ULA_DA::polar_pos random_target = {3.41097197, 0.20943951}; // (r,theta)

    auto result = codebook.find_closest(random_target);
    std::cout << "Spatial Error (Distance): " << result.first << " meters" << std::endl;
    std::cout << "Optimal Vector Size: " << result.second.size() << " antennas" << std::endl;

    return 0;
}