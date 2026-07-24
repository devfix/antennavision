//
// Created by Tristan Krause on 2026-07-24.
//

#include "setup/sweep.hpp"
#include <nlohmann/json.hpp>
#include "serialization.hpp"

#include "memory.hpp"

namespace setup::sweep
{
    template <any_json_t JsonType>
    void to_json(JsonType& js, ListSweep const& sweep)
    {
        js = JsonType{
            {"id", sweep.id()},
            {"values", sweep.values()}, //
        };
    }

    template <any_json_t JsonType>
    void from_json(JsonType const& js, ListSweep& sweep)
    {
        serialization::assert_structure(js,
            "sweep::ListSweep",
            {
                {"id", json::value_t::string},
                {"values", json::value_t::array}, //
            },
            {});
        reconstruct_at(sweep,
            ListSweep( //
                js.at("id").template get<std::string>(),
                js.at("values").template get<std::vector<double>>() //
                ));
    }

    template <any_json_t JsonType>
    void to_json(JsonType& js, LinearSweep const& sweep)
    {
        js = JsonType{
            {"id", sweep.id()},
            {"begin", sweep.begin_val()},
            {"end", sweep.end_val()},
            {"size", sweep.size()},
            {"values", sweep.values()}, //
        };
    }

    template <any_json_t JsonType>
    void from_json(JsonType const& js, LinearSweep& sweep)
    {
        serialization::assert_structure(js,
            "sweep::LinearSweep",
            {
                {"id", json::value_t::string},
                {"begin", json::value_t::number_float},
                {"end", json::value_t::number_float},
                {"size", json::value_t::number_integer}, //
            },
            {
                {"values", json::value_t::array} //
            });
        reconstruct_at(sweep,
            LinearSweep( //
                js.at("id").template get<std::string>(),
                js.at("begin").template get<double>(),
                js.at("end").template get<double>(),
                js.at("size").template get<std::size_t>() //
                ));
    }

    template <any_json_t JsonType>
    void to_json(JsonType& js, LogSweep const& sweep)
    {
        js = JsonType{
            {"id", sweep.id()},
            {"begin", sweep.begin_val()},
            {"end", sweep.end_val()},
            {"size", sweep.size()},
            {"base", sweep.base()},
            {"values", sweep.values()}, //
        };
    }

    template <any_json_t JsonType>
    void from_json(JsonType const& js, LogSweep& sweep)
    {
        serialization::assert_structure(js,
            "sweep::LogSweep",
            {
                {"id", json::value_t::string},
                {"begin", json::value_t::number_float},
                {"end", json::value_t::number_float},
                {"size", json::value_t::number_integer}, //
            },
            {
                {"base", json::value_t::number_float},
                {"values", json::value_t::array}, //
            });
        double base = js.contains("base") ? js.at("base").template get<double>() : 10.0;
        reconstruct_at(sweep,
            LogSweep( //
                js.at("id").template get<std::string>(),
                js.at("begin").template get<double>(),
                js.at("end").template get<double>(),
                js.at("size").template get<std::size_t>(),
                base //
                ));
    }

    std::vector<double> LinearSweep::values() const noexcept
    {
        std::vector<double> values(size_);
        for (std::size_t k = 0; k < size_; ++k)
        {
            double const t = static_cast<double>(k) / static_cast<double>(size_ - 1); // t in [0,1]
            values[k] = value_begin_ + t * (value_end_ - value_begin_);
        }
        return values;
    }

    std::vector<double> LogSweep::values() const noexcept
    {
        std::vector<double> values(size_);
        for (std::size_t k = 0; k < size_; ++k)
        {
            double const t = static_cast<double>(k) / static_cast<double>(size_ - 1); // t in [0,1]
            double const u = (std::pow(base_, t) - 1) / (base_ - 1); // u in [0,1]
            values[k] = value_begin_ + u * (value_end_ - value_begin_);
        }
        return values;
    }

    std::string const& get_id(Sweep const& sweep)
    {
        return std::visit([](auto const& s) -> std::string const& { return s.id(); }, sweep);
    }

    std::size_t get_size(Sweep const& sweep)
    {
        return std::visit([](auto const& s) -> std::size_t { return s.size(); }, sweep);
    }

    std::vector<double> get_values(Sweep const& sweep)
    {
        return std::visit([](auto const& s) { return s.values(); }, sweep);
    }

    double get_begin_val(Sweep const& sweep)
    {
        return std::visit([](auto const& s) -> double { return s.begin_val(); }, sweep);
    }

    double get_end_val(Sweep const& sweep)
    {
        return std::visit([](auto const& s) -> double { return s.end_val(); }, sweep);
    }

    // ListSweep Instantiations
    template void to_json(nlohmann::json&, ListSweep const&);
    template void to_json(nlohmann::ordered_json&, ListSweep const&);
    template void from_json(nlohmann::json const&, ListSweep&);
    template void from_json(nlohmann::ordered_json const&, ListSweep&);

    // LinearSweep Instantiations
    template void to_json(nlohmann::json&, LinearSweep const&);
    template void to_json(nlohmann::ordered_json&, LinearSweep const&);
    template void from_json(nlohmann::json const&, LinearSweep&);
    template void from_json(nlohmann::ordered_json const&, LinearSweep&);

    // LogSweep Instantiations
    template void to_json(nlohmann::json&, LogSweep const&);
    template void to_json(nlohmann::ordered_json&, LogSweep const&);
    template void from_json(nlohmann::json const&, LogSweep&);
    template void from_json(nlohmann::ordered_json const&, LogSweep&);
} // namespace setup::sweep
