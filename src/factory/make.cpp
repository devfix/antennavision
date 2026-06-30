//
// Created by core on 18.06.26.
//

#include "factory/make.hpp"
#include <locale>
#include <nlohmann/json.hpp>
#include "factory/find.hpp"
#include "factory/get.hpp"
#include "factory/parse.hpp"
#include "print.hpp"

struct RadiatorArray;

namespace factory
{
    namespace
    {
        bool assert_valid_id(std::string_view id)
        {
            for (char c : id)
            {
                if (!std::isalnum(c, std::locale::classic()) and c != '_' and c != '-') { throw std::runtime_error(std::format("Invalid id '{}', bad character: '{}'", id, c)); }
            }
            return true;
        }

    } // namespace

    Reference& make_reference(nlohmann::ordered_json& reference_desc, std::list<Reference>& references, std::map<std::string, double> const& variables)
    {
        auto const id = get_string(reference_desc, "id");
        assert_valid_id(id);
        auto const origin_id = get_string(reference_desc, "origin");
        auto const pos = get_pos(reference_desc, "pos", variables, true, true);
        auto const rotation = get_quaternion(reference_desc, "rot", variables, true, true);
        std::println("Creating reference [id: '{}', origin: '{}', pos: (x={:.3f}, y={:.3f}, z={:.3f}), rotation: (yaw={:.3f}π, pitch={:.3f}π, roll={:.3f}π]", id, origin_id, pos.x, pos.y, pos.z,
                     rotation.yaw() / pi, rotation.pitch() / pi, rotation.roll() / pi);
        Reference & origin = find_reference_by_id(references, origin_id);
        assert_empty(reference_desc);
        return references.emplace_back(id, &origin, pos, rotation);
    }

    std::vector<std::reference_wrapper<Radiator>> make_radiator(nlohmann::ordered_json& radiator_desc, std::list<Reference>& references, std::list<std::unique_ptr<Radiator>>& radiators,
                                                                std::map<std::string, double> const& variables, std::map<std::string, RadiatorArray>& radiator_arrays, bool const generate)
    {
        auto const id = get_string(radiator_desc, "id");
        if (!generate) { assert_valid_id(id); }
        auto const origin_id = get_string(radiator_desc, "ref", true, true);
        auto const type = get_string(radiator_desc, "type");
        std::println("Creating radiator [id: '{}', origin: '{}', type: '{}']", id, origin_id, type);
        Reference & origin = find_reference_by_id(references, origin_id);
        if (type == "HertzianDipole")
        {
            assert_empty(radiator_desc);
            radiators.push_back(std::unique_ptr<Radiator>(new Radiator(Radiator::HertzianDipole::create(id, origin)))); // NOLINT(*-make-unique)
            return {std::ref(*radiators.back())};
        }
        if (type == "CustomRadiator")
        {
            auto const effective_length_defs = get_string_vec3(radiator_desc, "effective_length");
            std::array<std::function<std::complex<double>(double, double, double)>, 3> effective_length_parts;
            std::ranges::transform(effective_length_defs, effective_length_parts.begin(), parse_polar_azimuth_function);
            auto effective_length = [effective_length_parts](double const polar, double const azimuth, double const wavelength) -> nc::NdArray<std::complex<double>>
            { return {effective_length_parts[0](polar, azimuth, wavelength), effective_length_parts[1](polar, azimuth, wavelength), effective_length_parts[2](polar, azimuth, wavelength)}; };
            assert_empty(radiator_desc);
            radiators.push_back(std::make_unique<Radiator>(id, origin, std::move(effective_length)));
            return {std::ref(*radiators.back())};
        }
        if (type == "ULA")
        {
            auto const spacing = get_double(radiator_desc, "spacing", variables);
            auto const count = get_uint(radiator_desc, "count", variables);
            auto dir = get_pos(radiator_desc, "dir", variables);
            auto const rot = get_quaternion(radiator_desc, "rot", variables, true, true);
            auto const prototype_desc = radiator_desc.at("radiator");
            radiator_desc.erase("radiator");

            if (dir.norm() < NUMERICAL_MARGIN) { throw std::runtime_error(std::format("Invalid direction for ULA '{}'", id)); }
            dir = dir.normalize();

            double const length = spacing * (count - 1);
            std::vector<std::reference_wrapper<Reference>> array_references;
            std::vector<std::reference_wrapper<Radiator>> array_radiators;
            for (std::remove_const_t<decltype(count)> i = 0; i < count; i++)
            {
                double const f = static_cast<double>(i) / static_cast<double>(count - 1);
                pos_t const pos = dir * (f - 0.5) * length;
                auto& ref = references.emplace_back(std::format("{}:ref:{}", id, i), &origin, pos, rot);
                array_references.push_back(std::ref(ref));

                // We make a copy of the "backup" description and adapt it for the current element of the ULA
                nlohmann::ordered_json ula_element_desc = prototype_desc;
                ula_element_desc["id"] = std::format("{}:radiator:{}", id, i);
                ula_element_desc["ref"] = ref.id;

                // call the make function recursively and append the Radiators to array_radiators
                std::ranges::move(make_radiator(ula_element_desc, references, radiators, variables, radiator_arrays, true), std::back_inserter(array_radiators));
            }
            if (auto [_, success] = radiator_arrays.try_emplace(id, id, array_radiators); !success)
            {
                throw std::runtime_error(std::format("Could not create radiator array, id '{}' already exists", id));
            }
            return std::move(array_radiators);
        }
        throw std::runtime_error(std::format("Unknown radiator type '{}'", type));
    }
} // namespace factory
