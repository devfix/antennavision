//
// Created by Tristan Krause on 2026-07-25.
//

#pragma once

#include <string>
#include <map>
#include <variant>
#include <cstdint>

using Var = std::variant<double, std::int64_t>;
using VarMap = std::map<std::string, Var>;
