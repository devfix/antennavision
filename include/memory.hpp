//
// Created by Tristan Krause on 2026-07-23.
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

// Metafunction to find the index of type T in Variant
template <typename T, typename Variant>
struct variant_index;

template <typename T, typename... Types>
struct variant_index<T, std::variant<Types...>> {
private:
    template <std::size_t Index = 0>
    static constexpr std::size_t find() {
        if constexpr (Index >= sizeof...(Types)) {
            static_assert(Index < sizeof...(Types), "Type T is not in std::variant!");
            return 0;
        } else if constexpr (std::is_same_v<T, std::variant_alternative_t<Index, std::variant<Types...>>>) {
            return Index;
        } else {
            return find<Index + 1>();
        }
    }
public:
    static constexpr std::size_t value = find();
};

template <typename T, typename Variant>
inline constexpr std::size_t variant_index_v = variant_index<T, Variant>::value;
