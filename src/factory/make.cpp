//
// Created by core on 18.06.26.
//

#include "factory/make.hpp"
#include <ansi_color.hpp>
#include <locale>
#include <nlohmann/json.hpp>
#include "factory/find.hpp"
#include "factory/get.hpp"
#include "factory/parse.hpp"
#include "print.hpp"
#include "simulationerror.hpp"

using namespace ansi_color;

namespace factory
{
    namespace
    {
        bool assert_valid_id(std::string_view id)
        {
            for (char c : id)
            {
                if (!std::isalnum(c, std::locale::classic()) and c != '_' and c != '-')
                {
                    throw SimulationError("Invalid id '{}', bad character: '{}'", id, c);
                }
            }
            return true;
        }
    } // namespace

    Reference& make_reference(ojson& reference_desc, std::list<Reference>& references, std::map<std::string, double> const& variables)
    {
        auto const id = get_string(reference_desc, "id");
        assert_valid_id(id);
        auto const origin_id = get_string(reference_desc, "origin");
        auto const pos = get_pos(reference_desc, "pos", variables, true, true);
        auto const rotation = get_quaternion(reference_desc, "rot", variables, true, true);
        std::println(
            "{}Creating reference [id: '{}', origin: '{}', pos: (x={:.3f}, y={:.3f}, z={:.3f}), rotation: (yaw={:.3f}π, pitch={:.3f}π, roll={:.3f}π]{}",
            fg4::bright_black, id, origin_id, pos.x, pos.y, pos.z, rotation.yaw() / pi, rotation.pitch() / pi, rotation.roll() / pi, reset);
        Reference& origin = find_reference_by_id(references, origin_id);
        assert_empty(reference_desc);
        return references.emplace_back(id, &origin, pos, rotation);
    }

    namespace
    {
        Radiator make_radiator(ojson& desc, Context& context)
        {
            auto const id = get_string(desc, "id");
            auto const origin_id = get_string(desc, "ref", true, true);
            auto const type = get_string(desc, "type");
            std::println("{}Creating radiator [id: '{}', origin: '{}', type: '{}']{}", fg4::bright_black, id, origin_id, type, reset);
            Reference& origin = find_reference_by_id(context.references, origin_id);
            if (type == "HertzianDipole") { return Radiator::HertzianDipole::create(id, origin); }
            if (type == "StandingWaveDipole")
            {
                auto const dipole_length = get_double(desc, "dipole_length", context.variables);
                return Radiator::StandingWaveDipole::create(id, origin, dipole_length);
            }
            if (type == "CustomRadiator")
            {
                auto const effective_length_defs = get_string_vec3(desc, "effective_length");
                std::array<std::function<complex_t(double, double, double)>, 3> effective_length_parts;
                std::ranges::transform(effective_length_defs, effective_length_parts.begin(), parse_polar_azimuth_function);
                auto effective_length = [effective_length_parts](double const polar, double const azimuth,
                                                                 double const wavelength) -> ComplexArray
                {
                    return {effective_length_parts[0](polar, azimuth, wavelength), effective_length_parts[1](polar, azimuth, wavelength),
                            effective_length_parts[2](polar, azimuth, wavelength)};
                };
                return {id, origin, std::move(effective_length)};
            }
            throw SimulationError("Unknown radiator type '{}'", type);
        }

        Antenna make_ula(ojson& desc, Context& context)
        {
            auto const id = get_string(desc, "id");
            auto const origin_id = get_string(desc, "ref", true, true);
            auto const type = get_string(desc, "type");
            Reference& origin = find_reference_by_id(context.references, origin_id);
            auto const spacing = get_double(desc, "spacing", context.variables);
            auto const size = get_uint(desc, "size", context.variables);
            auto const path_codebook = get_string(desc, "codebook", true, true);
            auto const rot = get_quaternion(desc, "rot", context.variables, true, true);
            auto const prototype_desc = desc.at("radiator");
            desc.erase("radiator");

            pos_t constexpr dir(1.0, 0.0, 0.0);
            double const length = spacing * (size - 1);
            std::list<Radiator> array_radiators;
            for (std::decay_t<decltype(size)> i = 0; i < size; i++)
            {
                double const f = static_cast<double>(i) / static_cast<double>(size - 1);
                pos_t const pos = dir * (f - 0.5) * length;
                const auto& ref = context.references.emplace_back(std::format("{}:ref:{}", id, i), &origin, pos, rot);

                // We make a copy of the "backup" description and adapt it for the current element of the ULA
                ojson ula_element_desc = prototype_desc;
                ula_element_desc["id"] = std::format("{}:radiator:{}", id, i);
                ula_element_desc["ref"] = ref.id;

                // call the make function recursively and append the Radiators to array_radiators
                array_radiators.push_back(make_radiator(ula_element_desc, context));
            }
            return UniformLinearArray(id, origin, std::move(array_radiators));
        }

        Antenna make_upa(ojson& desc, Context& context)
        {
            auto const id = get_string(desc, "id");
            auto const origin_id = get_string(desc, "ref", true, true);
            auto const type = get_string(desc, "type");
            Reference& origin = find_reference_by_id(context.references, origin_id);
            auto const spacing_x = get_double(desc, "spacing_x", context.variables);
            auto const spacing_y = get_double(desc, "spacing_y", context.variables);
            auto const size_x = get_uint(desc, "size_x", context.variables);
            auto const size_y = get_uint(desc, "size_y", context.variables);
            auto const path_codebook = get_string(desc, "codebook", true, true);
            auto const rot = get_quaternion(desc, "rot", context.variables, true, true);
            auto const prototype_desc = desc.at("radiator");
            desc.erase("radiator");

            double const length_x = spacing_x * (size_x - 1);
            double const length_y = spacing_y * (size_y - 1);
            std::list<Radiator> array_radiators;
            for (std::decay_t<decltype(size_y)> y = 0; y < size_y; y++)
            {
                for (std::decay_t<decltype(size_x)> x = 0; x < size_x; x++)
                {
                    double const fx = static_cast<double>(x) / static_cast<double>(size_x - 1);
                    double const fy = static_cast<double>(y) / static_cast<double>(size_y - 1);
                    pos_t const pos = pos_t(1.0, 0.0, 0.0) * (fx - 0.5) * length_x + pos_t(0.0, 1.0, 0.0) * (fy - 0.5) * length_y;
                    const auto& ref = context.references.emplace_back(std::format("{}:ref:{}:{}", id, x, y), &origin, pos, rot);

                    // We make a copy of the "backup" description and adapt it for the current element of the ULA
                    ojson ula_element_desc = prototype_desc;
                    ula_element_desc["id"] = std::format("{}:radiator:{}:{}", id, x, y);
                    ula_element_desc["ref"] = ref.id;

                    // call the make function recursively and append the Radiators to array_radiators
                    array_radiators.push_back(make_radiator(ula_element_desc, context));
                }
            }
            return UniformPlanarArray(id, origin, std::move(array_radiators), size_x, size_y);
        }
    } // namespace

    Antenna make_antenna(ojson& desc, Context& context)
    {
        // we only read the id, type and ref (origin id) from the description but do not delete them yet
        auto const id = get_string(desc, "id", false);
        assert_valid_id(id);
        auto const type = get_string(desc, "type", false);
        auto const origin_id = get_string(desc, "ref", false, true);
        std::println("{}Creating antenna [id: '{}', origin: '{}', type: '{}']{}", fg4::bright_black, id, origin_id, type, reset);

        // depending on the make a ULA, UPA or single radiator as the antenna
        if (type == "ULA") { return make_ula(desc, context); }
        if (type == "UPA") { return make_upa(desc, context); }
        return make_radiator(desc, context);
    }
}  // namespace factory
