//
// Created by Tristan Krause on 2026-04-28.
//

#pragma once

#include <NumCpp/Rotations/Quaternion.hpp>
#include <nlohmann/json_fwd.hpp> // Lightweight forward-declarations for nlohmann::json


using ojson = nlohmann::ordered_json;
using json = nlohmann::json;
template <typename T>
concept AnyJson = std::same_as<std::decay_t<T>, json> || std::same_as<std::decay_t<T>, ojson>;
