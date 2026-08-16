//
// Created by Tristan Krause on 2026-05-29.
//

#include "reference.hpp"
#include <ranges>
#include <nlohmann/json.hpp>
#include "math/coords.hpp"
#include "serialization.hpp"
#include "simulationerror.hpp"

namespace reference
{
    namespace
    {
        void assert_origin(Reference const* ref)
        {
            if (ref->origin_id.length() != 0 && ref->origin == nullptr)
            {
                throw SimulationError("Reference '{}' has unresolved origin '{}'", ref->id, ref->origin_id);
            }
        }
    } // namespace

    Reference Reference::create(std::string const& id, std::string const& origin_id, Pos const& pos, Quaternion const& rot)
    { return {.id = id, .origin_id = origin_id, .pos = pos, .rot = rot}; }

    Pos Reference::local_from_global_pos(Pos const& pos_global) const
    {
        assert_origin(this);
        return rot.inverse().rotate((origin ? origin->local_from_global_pos(pos_global) : pos_global) - this->pos);
    }

    Pos Reference::global_from_local_pos(Pos const& pos_local) const
    {
        assert_origin(this);
        Pos const pos_global = rot.rotate(pos_local) + this->pos;
        return origin ? origin->global_from_local_pos(pos_global) : pos_global;
    }

    Vec Reference::local_from_global_vec(Vec const& vec_global) const
    {
        assert_origin(this);
        return math::rotate(origin ? origin->local_from_global_vec(vec_global) : vec_global, rot.inverse());
    }

    Vec Reference::global_from_local_vec(Vec const& vec_local) const
    {
        assert_origin(this);
        Vec const vec_parent = math::rotate(vec_local, rot);
        return origin ? origin->global_from_local_vec(vec_parent) : vec_parent;
    }

    Pos Reference::localize(Reference const& reference) const { return local_from_global_pos(reference.global_from_local_pos(POS_ZERO)); }

    Pos Reference::global_pos() const { return global_from_local_pos(POS_ZERO); }

    Reference const& get(std::span<Reference const> references, std::string_view target_id)
    {
        auto const it = std::ranges::find(references, target_id, &Reference::id);
        if (it == references.end()) { throw SimulationError("Could not find reference with id '{}'", target_id); }
        return *it;
    }

    Reference& get(std::span<Reference> references, std::string_view target_id)
    {
        // std::as_const converts std::span<Antenna> -> std::span<Antenna const>
        // const_cast safe here because the original span contains non-const elements
        return const_cast<Reference&>(get(std::span<Reference const>(references), target_id));
    }

    void resolve_origins(std::span<Reference*> refs)
    {
        for (Reference* ref : refs)
        {
            if (ref->origin_id.length() != 0)
            {
                if (ref->id == ref->origin_id) { throw SimulationError("Reference '{}' has itself as the origin", ref->id); }
                auto const it = std::ranges::find(refs, ref->origin_id, [](Reference* ref) -> std::string const& { return ref->id; });
                if (it == refs.end()) { throw SimulationError("Reference '{}' has non-existing origin '{}'", ref->id, ref->origin_id); }
                ref->origin = *it;
            }
        }
    }

    void resolve_origins(std::span<Reference> refs)
    {
        std::vector<Reference*> ref_vec = refs | std::views::transform([](Reference& ref) { return std::addressof(ref); }) | std::ranges::to<std::vector>();
        resolve_origins(ref_vec);
    }

    void resolve_origins(std::initializer_list<std::reference_wrapper<Reference>> refs)
    {
        std::vector<Reference*> ref_vec = refs | std::views::transform([](Reference& ref) { return std::addressof(ref); }) | std::ranges::to<std::vector>();
        resolve_origins(ref_vec);
    }

    template <AnyJson JsonType>
    void to_json(JsonType& js, Reference const& ref)
    {
        js = JsonType{
            {"id", ref.id},
            {"origin", ref.origin ? ref.origin->id : ""},
            {"pos", ref.pos},
            {"rot", ref.rot},
        };
    }

    template <AnyJson JsonType>
    void from_json(JsonType const& js, Reference& ref)
    {
        serialization::assert_structure(js,
            "Reference",
            {
                {"id", json::value_t::string},
                {"origin", json::value_t::string},
            },
            {
                {"pos", json::value_t::array},
                {"rot", json::value_t::object},
            });
        std::string id;
        std::string origin;
        Pos pos;
        Quaternion rot;

        js.at("id").get_to(id);
        js.at("origin").get_to(origin);
        if (js.contains("pos")) { js.at("pos").get_to(pos); }
        if (js.contains("rot")) { js.at("rot").get_to(rot); }

        ref = Reference{.id = id, .origin_id = origin, .pos = pos, .rot = rot};
    }

    // Reference Instantiations
    template void to_json(nlohmann::json&, Reference const&);
    template void to_json(nlohmann::ordered_json&, Reference const&);
    template void from_json(nlohmann::json const&, Reference&);
    template void from_json(nlohmann::ordered_json const&, Reference&);
} // namespace reference
