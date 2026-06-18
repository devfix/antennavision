//
// Created by Tristan Krause on 2026-05-26.
//

#pragma once

#include <deque>
#include <filesystem>
#include "components/radiator.hpp"
#include "components/source.hpp"
#include "types.hpp"

struct Setup
{

    struct RadiatorSetup
    {
        std::string type;
        Vec3 position;
        double magnitude;
        double phase;
    };

    static std::unique_ptr<Setup> from_json(json const &js);
    static std::unique_ptr<Setup> from_file(path const &p);
    void export_to_three() const;
    void run_tasks();

    [[nodiscard]] Reference &get_reference_by_id(std::string_view id);
    [[nodiscard]] Radiator const &get_radiator_by_id(std::string_view id) const;

    std::string const name;
    std::deque<Reference> references;
    std::deque<std::unique_ptr<Radiator>> const radiators;
    std::deque<std::pair<std::string, std::function<void()>>> const tasks;

    std::vector<Source> sources;
    std::vector<Component> inter_components;

private:
    Setup(std::string_view name, std::deque<Reference> &&references, std::deque<std::unique_ptr<Radiator>> &&radiators, std::deque<std::pair<std::string, std::function<void()>>> && tasks);
};
