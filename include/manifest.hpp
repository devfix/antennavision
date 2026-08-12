//
// Created by Tristan Krause on 2026-06-02.
//

#pragma once

#include <array>
#include <string_view>

static constexpr std::string_view APPLICATION_NAME("AntennaVision");
static constexpr std::array<int, 3> APPLICATION_VERSION = {0, 1, 3};
extern std::string_view const BANNER;
extern std::size_t const N_POINTS_THREE_CURVE;
extern std::size_t const N_POINTS_THREE_SURFACE;
extern std::size_t const N_BATCH_PROGRESS_REPORT;
