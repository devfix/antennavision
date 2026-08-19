//
// Created by Tristan Krause on 2026-06-30.
//

#pragma once

#include "components/radiator.hpp"
#include "reference.hpp"

namespace components
{
    struct RadiatorArray
    {
        enum struct Type
        {
            CustomArray = 0,
            UniformLinearArray = 1,
            UniformPlanarArray = 2,
            UniformCircularArray = 3,
        };

        struct UniformLinearParameters
        {
            double spacing;
            std::size_t size;
        };

        struct UniformPlanarParameters
        {
            double spacing_x;
            double spacing_y;
            std::size_t size_x;
            std::size_t size_y;
        };

        using Parameters = std::variant<UniformLinearParameters, UniformPlanarParameters>;

        struct Desciptor
        {
            Type type;
            std::string const& id;
            std::string origin_id;
            Quaternion rot;
            Radiator::Descriptor prototype_desc;
            Parameters parameters;
        };

        [[nodiscard]] static RadiatorArray create_ula(Desciptor const& desc);
        [[nodiscard]] static RadiatorArray create_upa(Desciptor const& desc);

        [[nodiscard]] static RadiatorArray create(Desciptor const& desc);

        [[nodiscard]] reference::Reference const& get_origin(std::size_t idx_x, std::size_t idx_y = 0) const;
        [[nodiscard]] Radiator const& get_element(std::size_t idx_x, std::size_t idx_y = 0) const;

        Type type = Type::CustomArray;
        std::string id;
        std::string origin_id;
        std::vector<reference::Reference> references;
        std::vector<Radiator> elements;
        reference::Reference* origin;
        Parameters params;
    };
} // namespace components
