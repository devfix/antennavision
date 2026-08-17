//
// Created by Tristan Krause on 2026-08-17.
//

#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>
#include "components/radiatorarray.hpp"
#include "math/functions.hpp"

namespace components
{
    RadiatorArray RadiatorArray::create_ula(Desciptor const& desc)
    {
        auto const& params = std::get<UniformLinearParameters>(desc.parameters);
        auto const& size = params.size;
        auto const& spacing = params.spacing;

        Pos constexpr dir(1.0, 0.0, 0.0);
        double const length = spacing * (size - 1);
        std::vector<Radiator> elements;
        elements.reserve(size);
        std::vector<reference::Reference> references;
        references.reserve(size);
        auto prototype_desc = desc.prototype_desc;
        for (std::size_t k = 0; k < size; k++)
        {
            double const t = math::nidx(k, size);
            Pos const pos = dir * (t - 0.5) * length;
            references.push_back(reference::Reference{
                .id = std::format("{}:ref:{}", desc.id, k),
                .origin_id = desc.origin_id,
                .pos = pos,
                .rot = desc.rot //
            });

            prototype_desc.id = std::format("{}:rad:{}", desc.id, k);
            prototype_desc.origin_id = references.back().id;

            // call the make function recursively and append the Radiators to array_radiators
            elements.push_back(Radiator::create(prototype_desc));
        }
        return {
            .type = Type::UniformLinearArray,
            .id = desc.id,
            .origin_id = desc.origin_id,
            .references = std::move(references),
            .elements = std::move(elements),
            .params = params //
        };
    }

    RadiatorArray RadiatorArray::create_upa(Desciptor const& desc)
    {
        auto const& params = std::get<UniformPlanarParameters>(desc.parameters);
        auto const spacing_x = params.spacing_x;
        auto const spacing_y = params.spacing_y;
        auto const size_x = params.size_x;
        auto const size_y = params.size_y;

        Quaternion rot = desc.rot;
        auto prototype_desc = desc.prototype_desc;
        double const length_x = spacing_x * (size_x - 1);
        double const length_y = spacing_y * (size_y - 1);
        std::vector<Radiator> elements;
        elements.reserve(size_x * size_y);
        std::vector<reference::Reference> references;
        references.reserve(size_x * size_y);
        for (std::decay_t<decltype(size_y)> y = 0; y < size_y; y++)
        {
            for (std::decay_t<decltype(size_x)> x = 0; x < size_x; x++)
            {
                double const tx = math::nidx(x, size_x);
                double const ty = math::nidx(y, size_y);
                Pos const pos = Pos(1.0, 0.0, 0.0) * (tx - 0.5) * length_x + Pos(0.0, 1.0, 0.0) * (ty - 0.5) * length_y;
                references.push_back(reference::Reference{
                    .id = std::format("{}:ref:{}:{}", desc.id, x, y),
                    .origin_id = desc.origin_id,
                    .pos = pos,
                    .rot = rot //
                });

                prototype_desc.id = std::format("{}:rad:{}:{}", desc.id, x, y);
                prototype_desc.origin_id = references.back().id;

                // call the make function recursively and append the Radiators to array_radiators
                elements.push_back(Radiator::create(prototype_desc));
            }
        }

        return {
            .type = Type::UniformPlanarArray,
            .id = desc.id,
            .origin_id = desc.origin_id,
            .references = std::move(references),
            .params = params //
        };
    }

    RadiatorArray RadiatorArray::create(Desciptor const& desc)
    {
        switch (desc.type)
        {
            case Type::UniformLinearArray: return create_ula(desc);
            case Type::UniformPlanarArray: return create_upa(desc);
            default:
                throw SimulationError("Invalid radiator array type (index: {}, name: {})",
                    static_cast<std::size_t>(desc.type),
                    magic_enum::enum_name(desc.type));
        }
    }

    reference::Reference const& RadiatorArray::get_reference(std::size_t idx_x, std::size_t idx_y) const
    {
        switch (type)
        {
            case Type::CustomArray: [[fallthrough]];
            case Type::UniformLinearArray:
            {
                if (idx_y != 0) throw SimulationError("RadiatorArray has no y-dimension");
                auto const ptr = elements.at(idx_x).origin;
                if (!ptr) { throw SimulationError("Element {} in RadiatorArray '{}' has unconfigured origin", idx_x, id); }
                return *ptr;
            }
            case Type::UniformPlanarArray:
            {
                auto const& upa = std::get<UniformPlanarParameters>(params);
                auto const ptr = elements.at(idx_y * upa.size_x + idx_x).origin;
                if (!ptr) { throw SimulationError("Element {}:{} in UniformPlanarArray '{}' has unconfigured origin", idx_x, idx_y, id); }
                return *ptr;
            }
            default: throw SimulationError("Invalid radiator array type");
        }
    }
} // namespace components
