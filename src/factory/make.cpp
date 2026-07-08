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

    std::unique_ptr<Antenna> make_antenna(ojson& desc, Context & context, bool is_generated)
    {
        auto const id = get_string(desc, "id");
        if (!is_generated) { assert_valid_id(id); }
        auto const origin_id = get_string(desc, "ref", true, true);
        auto const type = get_string(desc, "type");
        std::println("{}Creating radiator [id: '{}', origin: '{}', type: '{}']{}", fg4::bright_black, id, origin_id, type, reset);
        Reference& origin = find_reference_by_id(context.references, origin_id);
        if (type == "HertzianDipole") { return std::unique_ptr<Antenna>(new Antenna(Radiator::HertzianDipole::create(id, origin))); }
        if (type == "StandingWaveDipole")
        {
            auto const dipole_length = get_double(desc, "dipole_length", context.variables);
            return std::unique_ptr<Radiator>(new Radiator(Radiator::StandingWaveDipole::create(id, origin, dipole_length)));
        }
        if (type == "CustomRadiator")
        {
            auto const effective_length_defs = get_string_vec3(desc, "effective_length");
            std::array<std::function<std::complex<double>(double, double, double)>, 3> effective_length_parts;
            std::ranges::transform(effective_length_defs, effective_length_parts.begin(), parse_polar_azimuth_function);
            auto effective_length = [effective_length_parts](double const polar, double const azimuth,
                                                             double const wavelength) -> nc::NdArray<std::complex<double>>
            {
                return {effective_length_parts[0](polar, azimuth, wavelength), effective_length_parts[1](polar, azimuth, wavelength),
                        effective_length_parts[2](polar, azimuth, wavelength)};
            };
            return std::make_unique<Radiator>(id, origin, std::move(effective_length));
        }
        throw SimulationError("Unknown radiator type '{}'", type);
    }

    GenericRadiatorArray make_radiator_array(ojson& radiator_desc, Context & context, bool is_generated)
    {
        auto const id = get_string(radiator_desc, "id");
        if (!is_generated) { assert_valid_id(id); }
        auto const origin_id = get_string(radiator_desc, "ref", true, true);
        auto const type = get_string(radiator_desc, "type");
        std::println("{}Creating radiator [id: '{}', origin: '{}', type: '{}']{}", fg4::bright_black, id, origin_id, type, reset);
        Reference& origin = find_reference_by_id(context.references, origin_id);
        if (type == "ULA")
        {
            auto const spacing = get_double(radiator_desc, "spacing", context.variables);
            auto const count = get_uint(radiator_desc, "count", context.variables);
            auto const path_codebook = get_string(radiator_desc, "codebook", true, true);
            auto dir = get_pos(radiator_desc, "dir", context.variables);
            auto const rot = get_quaternion(radiator_desc, "rot", context.variables, true, true);
            auto const prototype_desc = radiator_desc.at("radiator");
            radiator_desc.erase("radiator");

            if (dir.norm() < NUMERICAL_MARGIN) { throw SimulationError("Invalid direction for ULA '{}'", id); }
            dir = dir.normalize();

            double const length = spacing * (count - 1);
            std::list<Reference> array_references;
            std::vector<std::unique_ptr<Radiator>> array_radiators(count);
            for (std::remove_const_t<decltype(count)> i = 0; i < count; i++)
            {
                double const f = static_cast<double>(i) / static_cast<double>(count - 1);
                pos_t const pos = dir * (f - 0.5) * length;
                auto& ref = array_references.emplace_back(std::format("{}:ref:{}", id, i), &origin, pos, rot);

                // We make a copy of the "backup" description and adapt it for the current element of the ULA
                ojson ula_element_desc = prototype_desc;
                ula_element_desc["id"] = std::format("{}:radiator:{}", id, i);
                ula_element_desc["ref"] = ref.id;

                // call the make function recursively and append the Radiators to array_radiators
                array_radiators.at(i) = make_radiator(ula_element_desc, context, true);
            }
            return UniformLinearArray(id, std::move(array_references), std::move(array_radiators));
        }
        throw SimulationError("Unknown radiator type '{}'", type);
    }
} // namespace factory
