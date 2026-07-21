//
// Created by Tristan Krause on 2026-05-29.
//

#include "reference.hpp"
#include <ranges>
#include "math.hpp"
#include "simulationerror.hpp"

namespace
{
    void assert_origin(Reference const* ref)
    {
        if (ref->origin_id.length() != 0 && ref->origin == nullptr) { throw SimulationError("Reference '{}' has unresolved origin '{}'", ref->id, ref->origin_id); }
    }

    void resolve_origins_impl(std::span<Reference*> references)
    {
        for (Reference* ref : references)
        {
            if (ref->origin_id.length() != 0)
            {
                if (ref->id == ref->origin_id) { throw SimulationError("Reference '{}' has itself as the origin", ref->id); }
                auto const it = std::ranges::find(references, ref->origin_id, [](Reference* ref) -> std::string const& { return ref->id; });
                if (it == references.end()) { throw SimulationError("Reference '{}' has non-existing origin '{}'", ref->id, ref->origin_id); }
                ref->origin = *it;
            }
        }
    }
} // namespace

Reference Reference::create(std::string const& id, std::string const& origin_id, pos_t const& pos, Quaternion const& rot)
{
    return {.id = id, .origin_id = origin_id, .pos = pos, .rot = rot};
}

pos_t Reference::local_from_global_pos(pos_t const& pos_global) const
{
    assert_origin(this);
    return rot.inverse().rotate((origin ? origin->local_from_global_pos(pos_global) : pos_global) - this->pos);
}

pos_t Reference::global_from_local_pos(pos_t const& pos_local) const
{
    assert_origin(this);
    pos_t const pos_global = rot.rotate(pos_local) + this->pos;
    return origin ? origin->global_from_local_pos(pos_global) : pos_global;
}

vec_t Reference::local_from_global_vec(vec_t const& vec_global) const
{
    assert_origin(this);
    return math::rotate(origin ? origin->local_from_global_vec(vec_global) : vec_global, rot.inverse());
}

vec_t Reference::global_from_local_vec(vec_t const& vec_local) const
{
    assert_origin(this);
    vec_t const vec_parent = math::rotate(vec_local, rot);
    return origin ? origin->global_from_local_vec(vec_parent) : vec_parent;
}

pos_t Reference::localize(Reference const& reference) const { return local_from_global_pos(reference.global_from_local_pos(POS_ZERO)); }

pos_t Reference::global_pos() const { return global_from_local_pos(POS_ZERO); }

void Reference::resolve_origins(std::span<Reference> refs)
{
    std::vector<Reference*> ref_vec = refs | std::views::transform([](Reference& ref) { return std::addressof(ref); }) | std::ranges::to<std::vector>();
    resolve_origins_impl(ref_vec);
}

void Reference::resolve_origins(std::initializer_list<std::reference_wrapper<Reference>> refs)
{
    std::vector<Reference*> ref_vec = refs | std::views::transform([](Reference& ref) { return std::addressof(ref); }) | std::ranges::to<std::vector>();
    resolve_origins_impl(ref_vec);
}
