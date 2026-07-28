//
// Created by Tristan Krause on 2026-06-21.
//

#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include "setup/setup.hpp"

#define BUILTIN_FUNCTION(name, ...)                                                                                                                            \
    void name(__VA_ARGS__); /* declare function */                                                                                                                      \
    [[maybe_unused]] static Registerer reg_##name(#name, name); /* register function */                                                                                               \
    void name(__VA_ARGS__)  /* define function */

namespace builtin
{
    struct FunctionRegistry
    {
        using FuncType = std::function<void(setup::Setup& setup)>;

        static FunctionRegistry& instance()
        {
            static FunctionRegistry reg;
            return reg;
        }

        void register_func(std::string const& name, FuncType func) { registry[name] = func; }

        void call(std::string const& name, setup::Setup& setup)
        {
            if (auto it = registry.find(name); it != registry.end()) { it->second(setup); }
            else
            {
                throw SimulationError("Builtin task not found: {}", name);
            }
        }

    private:
        std::unordered_map<std::string, FuncType> registry;
    };

    struct Registerer
    {
        Registerer(const std::string& name, FunctionRegistry::FuncType func) { FunctionRegistry::instance().register_func(name, func); }
    };
} // namespace builtin
