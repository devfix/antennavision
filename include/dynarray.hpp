//
// Created by Tristan Krause on 2026-06-05.
//

#pragma once

#include <iostream>
#include <memory>
#include <span>

template <typename T>
class DynArray
{
    std::unique_ptr<T[]> data_;
    std::size_t size_;

public:
    // Constructor allocates exactly the requested size at runtime
    explicit DynArray(std::size_t const size) : data_(std::make_unique<T[]>(size)), size_(size) {}

    // Enforce that the array itself cannot be copied (preventing accidental duplicates)
    DynArray(const DynArray &) = delete;
    DynArray &operator=(const DynArray &) = delete;

    // Allow moving the container ownership
    DynArray(DynArray &&) noexcept = default;
    DynArray &operator=(DynArray &&) noexcept = default;

    [[nodiscard]] std::size_t size() const { return size_; }

    T &operator[](std::size_t index) { return data_[index]; }

    const T &operator[](std::size_t index) const { return data_[index]; }

    T *data() { return data_.get(); }

    const T *data() const { return data_.get(); }

    // --- SPAN & ITERATION SUPPORT ---

    // Explicitly grab a span
    std::span<T> span() { return {data_.get(), size_}; }

    std::span<const T> span() const { return {data_.get(), size_}; }

    // Implicit conversion to std::span (allows passing this object directly to functions requiring a span)
    explicit operator std::span<T>() { return span(); }

    explicit operator std::span<const T>() const { return span(); }

    // Explicit iterator hooks so range-based for loops work directly on the object
    auto begin() { return span().begin(); }

    auto end() { return span().end(); }

    auto begin() const { return span().begin(); }

    auto end() const { return span().end(); }
};
