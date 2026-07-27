//
// Created by Tristan Krause on 2026-07-24.
//

#include "setup/sweep.hpp"
#include <nlohmann/json.hpp>
#include "serialization.hpp"

#include "memory.hpp"

namespace sweep
{
    namespace
    {
        template <AnyJson JsonType>
        void load_list_sweep(JsonType const& js, Sweep& sweep)
        {
            serialization::assert_structure(js,
                "sweep::ListSweep",
                {
                    {"type", json::value_t::string},
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

        template <AnyJson JsonType>
        void load_linear_sweep(JsonType const& js, Sweep& sweep)
        {
            serialization::assert_structure(js,
                "sweep::LinearSweep",
                {
                    {"type", json::value_t::string},
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

        template <AnyJson JsonType>
        void load_log_sweep(JsonType const& js, Sweep& sweep)
        {
            serialization::assert_structure(js,
                "sweep::LogSweep",
                {
                    {"type", json::value_t::string},
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
    } // namespace

    template <AnyJson JsonType>
    void to_json(JsonType& js, Sweep const& sweep)
    {
        if (auto* list_sweep = std::get_if<ListSweep>(&sweep); list_sweep)
        {
            js = JsonType{
                {"type", "ListSweep"},
                {"id", list_sweep->id()},
                {"values", list_sweep->values()}, //
            };
        }
        else if (auto* linear_sweep = std::get_if<LinearSweep>(&sweep); linear_sweep)
        {
            js = JsonType{
                {"type", "LinearSweep"},
                {"id", linear_sweep->id()},
                {"begin", linear_sweep->begin_val()},
                {"end", linear_sweep->end_val()},
                {"size", linear_sweep->size()},
                {"values", linear_sweep->values()}, //
            };
        }
        else if (auto* log_sweep = std::get_if<LogSweep>(&sweep); log_sweep)
        {
            js = JsonType{
                {"type", "LogSweep"},
                {"id", log_sweep->id()},
                {"begin", log_sweep->begin_val()},
                {"end", log_sweep->end_val()},
                {"size", log_sweep->size()},
                {"base", log_sweep->base()},
                {"values", log_sweep->values()}, //
            };
        }
        else
            throw SimulationError("Unknown sweep object");
    }

    template <AnyJson JsonType>
    void from_json(JsonType const& js, Sweep& sweep)
    {
        if (!js.contains("type")) throw SimulationError("Missing sweep type");
        if (js.at("type").type() != nlohmann::json::value_t::string)
            throw SimulationError("Sweep attribute type must be string, but is {}", js.at("type").type_name());
        auto const type = js.at("type").template get<std::string>();

        if (type == "ListSweep")
            load_list_sweep(js, sweep);
        else if (type == "LinearSweep")
            load_linear_sweep(js, sweep);
        else if (type == "LogSweep")
            load_log_sweep(js, sweep);
        else
            throw SimulationError("Unknown sweep type '{}'", type);
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

    Sweep& get(std::span<Sweep> sweeps, std::string const& id)
    {
        auto const it = std::ranges::find(sweeps, id, [](auto& sweep) { return std::visit([](auto& s) { return s.id(); }, sweep); });
        if (it == sweeps.end()) { throw SimulationError("Could not find sweep with id '{}'", id); }
        return *it;
    }


    // -----------------------------------------------------------------------------
    // EXPLICIT INSTANTIATIONS
    // -----------------------------------------------------------------------------
    template void to_json(nlohmann::json&, Sweep const&);
    template void to_json(nlohmann::ordered_json&, Sweep const&);
    template void from_json(nlohmann::json const&, Sweep&);
    template void from_json(nlohmann::ordered_json const&, Sweep&);
} // namespace sweep
