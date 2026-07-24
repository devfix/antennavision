//
// Created by core on 2026-07-24.
//

#pragma once

#include <cmath>
#include <span>
#include <variant>
#include <vector>
#include <algorithm>

namespace setup::sweep
{
    struct ListSweep
    {
        [[nodiscard]] ListSweep() = default;

        [[nodiscard]] explicit ListSweep(std::span<double> values) : values_(values.begin(), values.end()) { std::ranges::sort(values_); };

        [[nodiscard]] std::size_t size() const noexcept { return values_.size(); }

        [[nodiscard]] std::vector<double> values() const noexcept { return values_; }

        [[nodiscard]] double begin_val() const noexcept { return values_.front(); }

        [[nodiscard]] double end_val() const noexcept { return values_.back(); }

    private:
        std::vector<double> values_;
    };

    struct LinearSweep
    {
        [[nodiscard]] LinearSweep() = default;

        [[nodiscard]] explicit LinearSweep(double value_begin, double value_end, std::size_t n_values) :
            value_begin_(value_begin), value_end_(value_end), n_values_(n_values)
        {}

        [[nodiscard]] std::size_t size() const noexcept { return n_values_; }

        [[nodiscard]] std::vector<double> values() const noexcept
        {
            std::vector<double> values(n_values_);
            for (std::size_t k = 0; k < n_values_; ++k)
            {
                double const t = static_cast<double>(k) / (n_values_ - 1); // t in [0,1]
                values[k] = value_begin_ + t * (value_end_ - value_begin_);
            }
            return values;
        }

        [[nodiscard]] double begin_val() const noexcept { return value_begin_; }

        [[nodiscard]] double end_val() const noexcept { return value_end_; }

    private:
        double value_begin_{};
        double value_end_{};
        std::size_t n_values_{};
    };

    struct LogSweep
    {
        [[nodiscard]] LogSweep() = default;

        [[nodiscard]] explicit LogSweep(double value_begin, double value_end, std::size_t n_values, double base = std::numbers::e) :
            value_begin_(value_begin), value_end_(value_end), n_values_(n_values), base_(base)
        {}

        [[nodiscard]] std::size_t size() const noexcept { return n_values_; }

        [[nodiscard]] std::vector<double> values() const noexcept
        {
            std::vector<double> values(n_values_);
            for (std::size_t k = 0; k < n_values_; ++k)
            {
                double const t = static_cast<double>(k) / (n_values_ - 1); // t in [0,1]
                double const u = (std::pow(base_, t) - 1) / (base_ - 1); // u in [0,1]
                values[k] = value_begin_ + u * (value_end_ - value_begin_);
            }
            return values;
        }

        [[nodiscard]] double begin_val() const noexcept { return value_begin_; }

        [[nodiscard]] double end_val() const noexcept { return value_end_; }

    private:
        double value_begin_{};
        double value_end_{};
        std::size_t n_values_{};
        double base_{};
    };

    using Sweep = std::variant<ListSweep, LinearSweep, LogSweep>;

    [[nodiscard]] inline std::size_t get_size(Sweep const& sweep)
    {
        return std::visit([](auto const& s) -> std::size_t { return s.size(); }, sweep);
    }

    [[nodiscard]] inline std::vector<double> get_values(Sweep const& sweep)
    {
        return std::visit([](auto const& s) { return s.values(); }, sweep);
    }

    [[nodiscard]] inline double get_begin_val(Sweep const& sweep)
    {
        return std::visit([](auto const& s) -> double { return s.begin_val(); }, sweep);
    }

    [[nodiscard]] inline double get_end_val(Sweep const& sweep)
    {
        return std::visit([](auto const& s) -> double { return s.end_val(); }, sweep);
    }
} // namespace setup::sweep
