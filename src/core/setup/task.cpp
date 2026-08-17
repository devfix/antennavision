//
// Created by Tristan Krause on 2026-07-29.
//

#include "setup/task.hpp"
#include <nlohmann/json.hpp>
#include "factory/find.hpp"
#include "factory/parse.hpp"

namespace setup::task
{
    template <AnyJson JsonType>
    DirectivityOverPolarAtAzimuth DirectivityOverPolarAtAzimuth::from_json(JsonType& js, Context const& ctx)
    {
        using TaskType = DirectivityOverPolarAtAzimuth;
        factory::try_resolve_double_expressions(js, ctx.variables, "wavelength");
        serialization::assert_structure(js,
            name,
            {
                {"output_path", json::value_t::string},
                {"type", json::value_t::string},
                {"tx", json::value_t::string},
                {"wavelength", json::value_t::number_float},
                {"sweep", json::value_t::string},
            },
            {});
        return TaskType{
            .output_path = js.at("output_path").template get<std::string>(),
            .tx = components::antenna::get(ctx.antennas, js.at("tx").template get<std::string>()),
            .wavelength = js.at("wavelength").template get<double>(),
            .sweep_azimuth = sweep::get(ctx.sweeps, js.at("sweep_azimuth").template get<std::string>()), //
        };
    }

    template <AnyJson JsonType>
    void DirectivityOverPolarAtAzimuth::to_json(JsonType& js) const
    {
        js = JsonType{
            {"type", name},
            {"output_path", output_path},
            {"tx", components::antenna::get_id(tx)},
            {"wavelength", wavelength},
            {"sweep", sweep::get_id(sweep_azimuth)} //
        };
    }

    template <AnyJson JsonType>
    VoltGainOverPoints VoltGainOverPoints::from_json(JsonType& js, Context const& ctx)
    {
        using TaskType = VoltGainOverPoints;
        factory::try_resolve_double_expressions(js, ctx.variables, "points");
        factory::try_resolve_double_expressions(js, ctx.variables, "wavelength");
        serialization::assert_structure(js,
            name,
            {
                {"output_path", json::value_t::string},
                {"type", json::value_t::string},
                {"ref", json::value_t::string},
                {"tx", json::value_t::string},
                {"rx", json::value_t::string},
                {"tx_codebook", json::value_t::array},
                {"rx_codebook", json::value_t::array},
                {"points", json::value_t::array},
                {"wavelength", json::value_t::number_float},
            },
            {});
        return TaskType{
            .output_path = js.at("output_path").template get<std::string>(),
            .ref = reference::get(ctx.references, js.at("ref").template get<std::string>()),
            .tx = components::antenna::get(ctx.antennas, js.at("tx").template get<std::string>()),
            .rx = components::antenna::get(ctx.antennas, js.at("rx").template get<std::string>()),
            .tx_codebook = js.at("tx_codebook").template get<std::vector<std::string>>(),
            .rx_codebook = js.at("rx_codebook").template get<std::vector<std::string>>(),
            .points = js.at("points").template get<std::vector<Pos>>(),
            .wavelength = js.at("wavelength").template get<double>(), //
        };
    }

    template <AnyJson JsonType>
    void VoltGainOverPoints::to_json(JsonType& js) const
    {
        js = JsonType{
            {"type", name},
            {"output_path", output_path},
            {"ref", ref.id},
            {"tx", components::antenna::get_id(tx)},
            {"rx", components::antenna::get_id(rx)},
            {"tx_codebook", tx_codebook},
            {"rx_codebook", rx_codebook},
            {"points", points},
            {"wavelength", wavelength} //
        };
    }

    template <AnyJson JsonType>
    VoltGainOverGeometry VoltGainOverGeometry::from_json(JsonType& js, Context const& ctx)
    {
        using TaskType = VoltGainOverGeometry;
        factory::try_resolve_int_expressions(js, ctx.variables, "n_dim1");
        factory::try_resolve_int_expressions(js, ctx.variables, "n_dim2");
        factory::try_resolve_double_expressions(js, ctx.variables, "wavelength");
        serialization::assert_structure(js,
            name,
            {
                {"output_path", json::value_t::string},
                {"type", json::value_t::string},
                {"ref", json::value_t::string},
                {"tx", json::value_t::string},
                {"rx", json::value_t::string},
                {"tx_codebook", json::value_t::array},
                {"rx_codebook", json::value_t::array},
                {"geo", json::value_t::string},
                {"n_dim1", json::value_t::number_integer},
                {"n_dim2", json::value_t::number_integer},
                {"wavelength", json::value_t::number_float},
            },
            {});
        return TaskType{
            .output_path = js.at("output_path").template get<std::string>(),
            .ref = reference::get(ctx.references, js.at("ref").template get<std::string>()),
            .tx = components::antenna::get(ctx.antennas, js.at("tx").template get<std::string>()),
            .rx = components::antenna::get(ctx.antennas, js.at("rx").template get<std::string>()),
            .tx_codebook = js.at("tx_codebook").template get<std::vector<std::string>>(),
            .rx_codebook = js.at("rx_codebook").template get<std::vector<std::string>>(),
            .geo = geometry::get(ctx.geometries, js.at("geo").template get<std::string>()),
            .n_dim1 = js.at("n_dim1").template get<std::size_t>(),
            .n_dim2 = js.at("n_dim2").template get<std::size_t>(),
            .wavelength = js.at("wavelength").template get<double>(), //
        };
    }

    template <AnyJson JsonType>
    void VoltGainOverGeometry::to_json(JsonType& js) const
    {
        js = JsonType{
            {"type", name},
            {"output_path", output_path},
            {"ref", ref.id},
            {"tx", components::antenna::get_id(tx)},
            {"rx", components::antenna::get_id(rx)},
            {"tx_codebook", tx_codebook},
            {"rx_codebook", rx_codebook},
            {"geo", geometry::get_id(geo)},
            {"n_dim1", n_dim1},
            {"n_dim2", n_dim2},
            {"wavelength", wavelength} //
        };
    }

    template <AnyJson JsonType>
    VoltGainOverGeometryAtWavelength VoltGainOverGeometryAtWavelength::from_json(JsonType& js, Context const& ctx)
    {
        using TaskType = VoltGainOverGeometryAtWavelength;
        factory::try_resolve_int_expressions(js, ctx.variables, "n_dim1");
        factory::try_resolve_int_expressions(js, ctx.variables, "n_dim2");
        serialization::assert_structure(js,
            name,
            {
                {"output_path", json::value_t::string},
                {"type", json::value_t::string},
                {"ref", json::value_t::string},
                {"tx", json::value_t::string},
                {"rx", json::value_t::string},
                {"tx_codebook", json::value_t::array},
                {"rx_codebook", json::value_t::array},
                {"geo", json::value_t::string},
                {"n_dim1", json::value_t::number_integer},
                {"n_dim2", json::value_t::number_integer},
                {"sweep_wavelength", json::value_t::string},
            },
            {});
        return TaskType{
            .output_path = js.at("output_path").template get<std::string>(),
            .ref = reference::get(ctx.references, js.at("ref").template get<std::string>()),
            .tx = components::antenna::get(ctx.antennas, js.at("tx").template get<std::string>()),
            .rx = components::antenna::get(ctx.antennas, js.at("rx").template get<std::string>()),
            .tx_codebook = js.at("tx_codebook").template get<std::vector<std::string>>(),
            .rx_codebook = js.at("rx_codebook").template get<std::vector<std::string>>(),
            .geo = geometry::get(ctx.geometries, js.at("geo").template get<std::string>()),
            .n_dim1 = js.at("n_dim1").template get<std::size_t>(),
            .n_dim2 = js.at("n_dim2").template get<std::size_t>(),
            .sweep_wavelength = sweep::get(ctx.sweeps, js.at("sweep").template get<std::string>()), //
        };
    }

    template <AnyJson JsonType>
    void VoltGainOverGeometryAtWavelength::to_json(JsonType& js) const
    {
        js = JsonType{
            {"type", name},
            {"output_path", output_path},
            {"ref", ref.id},
            {"tx", components::antenna::get_id(tx)},
            {"rx", components::antenna::get_id(rx)},
            {"tx_codebook", tx_codebook},
            {"rx_codebook", rx_codebook},
            {"geo", geometry::get_id(geo)},
            {"n_dim1", n_dim1},
            {"n_dim2", n_dim2},
            {"sweep_wavelength", sweep::get_id(sweep_wavelength)} //
        };
    }

    template <AnyJson JsonType>
    VoltGainPeakAndCutoffs VoltGainPeakAndCutoffs::from_json(JsonType& js, Context const& ctx)
    {
        using TaskType = VoltGainPeakAndCutoffs;
        factory::try_resolve_int_expressions(js, ctx.variables, "n_scan");
        factory::try_resolve_double_expressions(js, ctx.variables, "ratio");
        factory::try_resolve_double_expressions(js, ctx.variables, "wavelength");
        serialization::assert_structure(js,
            name,
            {
                {"output_path", json::value_t::string},
                {"type", json::value_t::string},
                {"ref", json::value_t::string},
                {"tx", json::value_t::string},
                {"rx", json::value_t::string},
                {"tx_codebook", json::value_t::array},
                {"rx_codebook", json::value_t::array},
                {"curve", json::value_t::string},
                {"n_scan", json::value_t::number_integer},
                {"ratio", json::value_t::number_float},
                {"wavelength", json::value_t::number_float},
            },
            {});
        return TaskType{
            .output_path = js.at("output_path").template get<std::string>(),
            .ref = reference::get(ctx.references, js.at("ref").template get<std::string>()),
            .tx = components::antenna::get(ctx.antennas, js.at("tx").template get<std::string>()),
            .rx = components::antenna::get(ctx.antennas, js.at("rx").template get<std::string>()),
            .tx_codebook = js.at("tx_codebook").template get<std::vector<std::string>>(),
            .rx_codebook = js.at("rx_codebook").template get<std::vector<std::string>>(),
            .curve = geometry::curve::get(ctx.geometries, js.at("curve").template get<std::string>()),
            .n_scan = js.at("n_scan").template get<std::size_t>(),
            .ratio = js.at("ratio").template get<double>(),
            .wavelength = js.at("wavelength").template get<double>(), //
        };
    }

    template <AnyJson JsonType>
    void VoltGainPeakAndCutoffs::to_json(JsonType& js) const
    {
        js = JsonType{
            {"type", name},
            {"output_path", output_path},
            {"ref", ref.id},
            {"tx", components::antenna::get_id(tx)},
            {"rx", components::antenna::get_id(rx)},
            {"tx_codebook", tx_codebook},
            {"rx_codebook", rx_codebook},
            {"curve", geometry::get_id(curve)},
            {"n_scan", n_scan},
            {"ratio", ratio},
            {"wavelength", wavelength} //
        };
    }

    template <AnyJson JsonType>
    Task from_json(JsonType& js, Context const& ctx)
    {
        auto s= js.dump(2);
        if (not js.contains("type")) throw SimulationError("Missing task type");
        if (js.at("type").type() != nlohmann::json::value_t::string)
            throw SimulationError("Task attribute type must be string, but is {}", js.at("type").type_name());
        auto const type = js.at("type").template get<std::string>();

        if (type == DirectivityOverPolarAtAzimuth::name) return DirectivityOverPolarAtAzimuth::from_json(js, ctx);
        if (type == VoltGainOverPoints::name) return VoltGainOverPoints::from_json(js, ctx);
        if (type == VoltGainOverGeometry::name) return VoltGainOverGeometry::from_json(js, ctx);
        if (type == VoltGainOverGeometryAtWavelength::name) return VoltGainOverGeometryAtWavelength::from_json(js, ctx);
        if (type == VoltGainPeakAndCutoffs::name) return VoltGainPeakAndCutoffs::from_json(js, ctx);

        throw SimulationError("Unknown task type '{}'", type);
    }

    template <AnyJson JsonType>
    void to_json(JsonType& js, Task const& task)
    {
        task.visit([&js](auto const& t) { t.to_json(js); });
    }

    std::optional<OutputType> output_type_from_ext(std::string_view ext)
    {
        if (ext == ".json") return OutputType::JSON;
        if (ext == ".bson") return OutputType::BSON;
        if (ext == ".cbor") return OutputType::CBOR;
        if (ext == ".mpk" || ext == ".msgpack") return OutputType::MSGPACK;
        if (ext == ".ubj" || ext == ".ubjson") return OutputType::UBJSON;
        return std::nullopt;
    }

    OutputType get_output_type(Task const& task)
    {
        auto ext = get_output_path(task).extension().string();
        auto output_type_opt = output_type_from_ext(ext);
        if (not output_type_opt) throw SimulationError("Task '{}' has invalid output file type '{}'", get_id(task), ext);
        return output_type_opt.value();
    }

    // -----------------------------------------------------------------------------
    // EXPLICIT INSTANTIATIONS
    // -----------------------------------------------------------------------------
    template Task from_json(nlohmann::json&, Context const& ctx);
    template Task from_json(nlohmann::ordered_json&, Context const& ctx);
    template void to_json(nlohmann::json&, Task const&);
    template void to_json(nlohmann::ordered_json&, Task const&);

} // namespace setup::task
