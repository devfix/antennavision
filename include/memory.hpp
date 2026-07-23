//
// Created by core on 2026-07-23.
//

#pragma once

#include <memory>
#include <utility>

/**
 * @brief Re-constructs an object in-place at its existing memory location.
 * @tparam T The type of the object.
 * @tparam Args Argument types to pass to T's constructor.
 * @param obj Reference to the object to reconstruct.
 * @param args Arguments forwarded to construct the new object state.
 */
template <typename T, typename... Args>
void reconstruct_at(T& obj, Args&&... args)
{
    std::destroy_at(&obj);
    std::construct_at(&obj, std::forward<Args>(args)...);
}
