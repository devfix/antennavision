//
// Created by core on 21.06.26.
//

#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include "setup.hpp"

namespace builtin
{
    struct FunctionRegistry {
        using FuncType = std::function<void(Setup& setup)>;

        static FunctionRegistry& instance() {
            static FunctionRegistry reg;
            return reg;
        }

        void register_func(std::string const& name, FuncType func) {
            registry[name] = func;
        }

        void call(std::string const& name, Setup& setup) {
            if (auto it = registry.find(name); it != registry.end()) {
                it->second(setup);
            } else {
                throw SimulationError("Builtin task not found: {}", name);
            }
        }

    private:
        std::unordered_map<std::string, FuncType> registry;
    };

    struct Registerer {
        Registerer(const std::string& name, FunctionRegistry::FuncType func) {
            FunctionRegistry::instance().register_func(name, func);
        }
    };

    void t00_ula_beamwidth(Setup& setup);
    void t01_upa_beam_shape(Setup& setup);
} // namespace builtin
