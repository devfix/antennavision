//
// Created by Tristan Krause on 2026-07-24.
//

#pragma once

#include <algorithm>
#include <span>
#include <variant>
#include <vector>
#include "types/json.hpp"

namespace sweep
{
    struct ListSweep
    {
        static constexpr std::string_view name = "ListSweep";

        [[nodiscard]] ListSweep() = default;

        [[nodiscard]] explicit ListSweep(std::string id, std::span<double const> values) : id_(std::move(id)), values_(values.begin(), values.end())
        { std::ranges::sort(values_); };

        [[nodiscard]] std::string const& id() const noexcept { return id_; }

        [[nodiscard]] std::size_t size() const noexcept { return values_.size(); }

        [[nodiscard]] std::vector<double> values() const noexcept { return values_; }

        [[nodiscard]] double begin_val() const noexcept { return values_.front(); }

        [[nodiscard]] double end_val() const noexcept { return values_.back(); }

    private:
        std::string id_;
        std::vector<double> values_;
    };

    struct LinearSweep
    {
        static constexpr std::string_view name = "LinearSweep";

        [[nodiscard]] LinearSweep() = default;

        [[nodiscard]] explicit LinearSweep(std::string id, double value_begin, double value_end, std::size_t size) :
            id_(std::move(id)), value_begin_(value_begin), value_end_(value_end), size_(size)
        {}

        [[nodiscard]] std::string const& id() const noexcept { return id_; }

        [[nodiscard]] std::size_t size() const noexcept { return size_; }

        [[nodiscard]] std::vector<double> values() const noexcept;

        [[nodiscard]] double begin_val() const noexcept { return value_begin_; }

        [[nodiscard]] double end_val() const noexcept { return value_end_; }

    private:
        std::string id_;
        std::size_t size_{};
        double value_begin_{};
        double value_end_{};
    };

    struct ExpSweep
    {
        static constexpr std::string_view name = "ExpSweep";

        [[nodiscard]] ExpSweep() = default;

        [[nodiscard]] explicit ExpSweep(std::string id, double value_begin, double value_end, std::size_t size, double base = 10.0) :
            id_(std::move(id)), value_begin_(value_begin), value_end_(value_end), size_(size), base_(base)
        {}

        [[nodiscard]] std::string const& id() const noexcept { return id_; }

        [[nodiscard]] std::size_t size() const noexcept { return size_; }

        [[nodiscard]] std::vector<double> values() const noexcept;

        [[nodiscard]] double begin_val() const noexcept { return value_begin_; }

        [[nodiscard]] double end_val() const noexcept { return value_end_; }

        [[nodiscard]] double base() const noexcept { return base_; }

    private:
        std::string id_;
        std::size_t size_{};
        double value_begin_{};
        double value_end_{};
        double base_{};
    };

    struct LogSweep
    {
        static constexpr std::string_view name = "LogSweep";

        [[nodiscard]] LogSweep() = default;

        [[nodiscard]] explicit LogSweep(std::string id, double value_begin, double value_end, std::size_t size, double base = 10.0) :
            id_(std::move(id)), value_begin_(value_begin), value_end_(value_end), size_(size), base_(base)
        {}

        [[nodiscard]] std::string const& id() const noexcept { return id_; }

        [[nodiscard]] std::size_t size() const noexcept { return size_; }

        [[nodiscard]] std::vector<double> values() const noexcept;

        [[nodiscard]] double begin_val() const noexcept { return value_begin_; }

        [[nodiscard]] double end_val() const noexcept { return value_end_; }

        [[nodiscard]] double base() const noexcept { return base_; }

    private:
        std::string id_;
        std::size_t size_{};
        double value_begin_{};
        double value_end_{};
        double base_{};
    };

    using Sweep = std::variant<ListSweep, LinearSweep, ExpSweep, LogSweep>;

    template <AnyJson JsonType>
    void to_json(JsonType& js, Sweep const& sweep);

    template <AnyJson JsonType>
    void from_json(JsonType const& js, Sweep& sweep);

    [[nodiscard]] std::string const& get_id(Sweep const& sweep);
    [[nodiscard]] std::size_t get_size(Sweep const& sweep);
    [[nodiscard]] std::vector<double> get_values(Sweep const& sweep);
    [[nodiscard]] double get_begin_val(Sweep const& sweep);
    [[nodiscard]] double get_end_val(Sweep const& sweep);

    Sweep const& get(std::span<Sweep const> sweeps, std::string const& id);
    Sweep& get(std::span<Sweep> sweeps, std::string const& id);
} // namespace sweep
