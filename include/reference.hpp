//
//  Created by Tristan Krause on 2026-05-29.
//

#pragma once

#include <concepts>
#include "simulationerror.hpp"
#include "types.hpp"

struct Reference; // forward declaration

template <typename R>
concept ReferenceContainer = std::ranges::range<R> && std::same_as<std::ranges::range_value_t<R>, Reference>;

/**
 * Class "Reference" of Aggregate Type
 * Also known as POD (Plain Old Data) / PDS (Passive Data Structure) / DTO (Data Transfer Object)
 */
struct Reference
{
    static Reference create(std::string const& id, std::string const& origin_id, pos_t const& pos, Quaternion const& rot);

    [[nodiscard]] pos_t local_from_global_pos(pos_t const& pos_global) const;
    [[nodiscard]] pos_t global_from_local_pos(pos_t const& pos_local) const;
    [[nodiscard]] vec_t local_from_global_vec(vec_t const& vec_global) const;
    [[nodiscard]] vec_t global_from_local_vec(vec_t const& vec_local) const;
    [[nodiscard]] pos_t localize(Reference const& reference) const;
    [[nodiscard]] pos_t global_pos() const;
    void reset();

    /**
     * Search a reference by id and return reference to it
     * @param references container of references
     * @param target_id id of the target references that shall be returned
     * @return reference to matching reference
     */
    [[nodiscard]] static Reference& get(ReferenceContainer auto& references, std::string const& target_id);

    /**
     * Interconnect the references in a container, i.d., resolving non-empty origin ".origin_id" ids to their actual pointer ".origin"
     * @param references std::ranges::range that hold References
     */
    static void resolve_origins(ReferenceContainer auto& references);

    std::string id; /// identifier name for the reference
    std::string origin_id; /// name of the origin, may be empty string
    pos_t pos; /// position relative to the origins' zero
    Quaternion rot; /// rotation relative to the origins' rotation

    // last argument since optional for brace-initializer list
    Reference* origin{}; /// pointer to the origin
};

Reference& Reference::get(ReferenceContainer auto& references, std::string const& target_id)
{
    auto const it = std::ranges::find(references, target_id, &Reference::id);
    if (it == references.end()) { throw SimulationError("Could not find reference with id '{}'", target_id); }
    return *it;
}

void Reference::resolve_origins(ReferenceContainer auto& references)
{
    for (Reference& ref : references)
    {
        if (ref.origin_id.length() != 0)
        {
            if (ref.id == ref.origin_id) { throw SimulationError("Reference '{}' has itself as the origin", ref.id); }
            ref.origin = &Reference::get(references, ref.origin_id);
        }
    }
}
