//
//  Created by Tristan Krause on 2026-05-29.
//

#pragma once

#include "simulationerror.hpp"
#include "types.hpp"


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

    /**
     * Search a reference by id and return reference to it
     * @param references container of references
     * @param target_id id of the target references that shall be returned
     * @return reference to matching reference
     */
    [[nodiscard]] static Reference& get(std::span<Reference> references, std::string const& target_id);

    /**
     * Interconnect all references, i.d., resolving non-empty origins ".origin_id" ids to their actual pointer ".origin".
     * Important: After this function call, the references must remain at their memory location.
     * Otherwise, the pointers become invalid which will cause segmentations faults.
     * This function is idempotent.
     * @param refs std::span that holds the references
     */
    static void resolve_origins(std::span<Reference> refs);

    /**
     * Interconnect all references, i.d., resolving non-empty origins ".origin_id" ids to their actual pointer ".origin".
     * Important: After this function call, the references must remain at their memory location.
     * Otherwise, the pointers become invalid which will cause segmentations faults.
     * This function is idempotent.
     * @param refs std::initializer_list that holds the references
     */
    static void resolve_origins(std::initializer_list<std::reference_wrapper<Reference>> refs);

    std::string id; /// identifier name for the reference
    std::string origin_id; /// name of the origin, may be empty string
    pos_t pos; /// position relative to the origins' zero
    Quaternion rot; /// rotation relative to the origins' rotation

    // last argument since optional for brace-initializer list
    Reference* origin{}; /// pointer to the origin
};
