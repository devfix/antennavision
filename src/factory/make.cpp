//
// Created by core on 18.06.26.
//

#include "factory/make.hpp"
#include <locale>
#include "components/customradiator.hpp"
#include "components/hertziandipole.hpp"
#include "factory/find.hpp"
#include "factory/get.hpp"
#include "factory/parse.hpp"
#include "print.hpp"

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

    Reference make_reference(json &reference_desc, std::list<Reference> const &references, std::map<std::string, double> const& variables)
    {
        auto const id = get_string(reference_desc, "id");
        assert_valid_id(id);
        auto const origin_id = get_string(reference_desc, "origin");
        auto const translation = get_vec3(reference_desc, "translation", &variables, true, true);
        auto const rotation = get_quaternion(reference_desc, "rotation", &variables, true, true);
        std::println("Creating reference [id: '{}', origin: '{}', translation: (x={:.3f}, y={:.3f}, z={:.3f}), rotation: (yaw={:.3f}π, pitch={:.3f}π, roll={:.3f}π]", id, origin_id, translation.x,
                     translation.y, translation.z, rotation.yaw() / pi, rotation.pitch() / pi, rotation.roll() / pi);
        Reference const &origin = find_reference_by_id(references, origin_id);
        assert_empty(reference_desc);
        return {id, &origin, translation, rotation};
    }

    std::unique_ptr<Radiator> make_radiator(json &radiator_desc, std::list<Reference> const &references, std::map<std::string, double> const& variables)
    {
        auto const id = get_string(radiator_desc, "id");
        assert_valid_id(id);
        auto const origin_id = get_string(radiator_desc, "ref", true, true);
        auto const type = get_string(radiator_desc, "type");
        std::println("Creating radiator [id: '{}', origin: '{}', type: '{}']", id, origin_id, type);
        Reference const &origin = find_reference_by_id(references, origin_id);
        if (type == "HertzianDipole")
        {
            double length = get_double(radiator_desc, "length", &variables);
            assert_empty(radiator_desc);
            return std::make_unique<HertzianDipole>(id, origin, length);
        }
        else if (type == "CustomRadiator")
        {
            auto const effective_length_defs = get_string_vec3(radiator_desc, "effective_length");
            std::array<std::function<double(double, double)>, 3> effective_length_parts;
            std::ranges::transform(effective_length_defs, effective_length_parts.begin(), parse_theta_phi_function);
            auto effective_length = [effective_length_parts](double const theta, double const phi) -> Vec3
            { return {effective_length_parts[0](theta, phi), effective_length_parts[1](theta, phi), effective_length_parts[2](theta, phi)}; };
            assert_empty(radiator_desc);
            return std::make_unique<CustomRadiator>(id, origin, std::move(effective_length));
        }
        throw std::runtime_error(std::format("Unknown radiator type '{}'", type));
    }
} // namespace factory
