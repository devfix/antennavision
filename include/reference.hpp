//
//  Created by Tristan Krause on 2026-05-29.
//

#pragma once

#include "simulationerror.hpp"
#include "types/json.hpp"
#include "types/math.hpp"

namespace reference
{
    /**
     * Class "Reference" of Aggregate Type
     * Also known as POD (Plain Old Data) / PDS (Passive Data Structure) / DTO (Data Transfer Object)
     */
    struct Reference
    {
        static Reference create(std::string const& id, std::string const& origin_id, Pos const& pos, Quaternion const& rot);

        [[nodiscard]] Pos local_from_global_pos(Pos const& pos_global) const;
        [[nodiscard]] Pos global_from_local_pos(Pos const& pos_local) const;
        [[nodiscard]] Vec local_from_global_vec(Vec const& vec_global) const;
        [[nodiscard]] Vec global_from_local_vec(Vec const& vec_local) const;
        [[nodiscard]] Pos localize(Reference const& reference) const;
        [[nodiscard]] Pos global_pos() const;

        std::string id; /// identifier name for the reference
        std::string origin_id; /// name of the origin, may be empty string
        Pos pos; /// position relative to the origins' zero
        Quaternion rot; /// rotation relative to the origins' rotation

        // last argument since optional for brace-initializer list
        Reference* origin{}; /// pointer to the origin
    };

    /**
     * Search a reference by id and return reference to it
     * @param references container of references
     * @param target_id id of the target references that shall be returned
     * @return reference to matching reference
     */
    [[nodiscard]] Reference const& get(std::span<Reference const> references, std::string const& target_id);
    [[nodiscard]] Reference& get(std::span<Reference> references, std::string const& target_id);


    void resolve_origins(std::span<Reference*> refs);

    /**
     * Interconnect all references, i.d., resolving non-empty origins ".origin_id" ids to their actual pointer ".origin".
     * Important: After this function call, the references must remain at their memory location.
     * Otherwise, the pointers become invalid which will cause segmentations faults.
     * This function is idempotent.
     * @param refs std::span that holds the references
     */
    void resolve_origins(std::span<Reference> refs);

    /**
     * Interconnect all references, i.d., resolving non-empty origins ".origin_id" ids to their actual pointer ".origin".
     * Important: After this function call, the references must remain at their memory location.
     * Otherwise, the pointers become invalid which will cause segmentations faults.
     * This function is idempotent.
     * @param refs std::initializer_list that holds the references
     */
    void resolve_origins(std::initializer_list<std::reference_wrapper<Reference>> refs);

    template <any_json_t JsonType>
    void to_json(JsonType& j, Reference const& ref);

    template <any_json_t JsonType>
    void from_json(JsonType const& j, Reference& ref);
} // namespace reference
