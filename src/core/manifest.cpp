//
// Created by Tristan Krause on 2026-05-29.
//

#include "manifest.hpp"

// https://patorjk.com/software/taag/#p=display&f=Big&t=AntennaVision&x=none&v=4&h=4&w=80&we=false

namespace
{
#ifndef NDEBUG
    constexpr bool DEBUG_MODE = true;
#else
    constexpr bool DEBUG_MODE = false;
#endif
} // namespace

std::string_view const BANNER = R"(
                 _                      __      ___     _
     /\         | |                     \ \    / (_)   (_)
    /  \   _ __ | |_ ___ _ __  _ __   __ \ \  / / _ ___ _  ___  _ __
   / /\ \ | '_ \| __/ _ \ '_ \| '_ \ / _` \ \/ / | / __| |/ _ \| '_ \
  / ____ \| | | | ||  __/ | | | | | | (_| |\  /  | \__ \ | (_) | | | |
 /_/    \_\_| |_|\__\___|_| |_|_| |_|\__,_| \/   |_|___/_|\___/|_| |_|

)";

std::size_t const N_POINTS_THREE_CURVE = 32;
std::size_t const N_POINTS_THREE_SURFACE = 32;
std::size_t const N_BATCH_PROGRESS_REPORT = 64 * (DEBUG_MODE ? 1 : 8);
