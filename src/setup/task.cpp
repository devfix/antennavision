//
// Created by Tristan Krause on 2026-07-29.
//

#include "setup/task.hpp"
#include <nlohmann/json.hpp>

#include "memory.hpp"

namespace setup::task
{
    namespace
    {
        template <AnyJson JsonType>
        void load_directivity_over_polar_at_azimuth(JsonType const& js, Task& t)
        {
            using TaskType = DirectivityOverPolarAtAzimuth;
            serialization::assert_structure(js,
                TaskType::name,
                {
                    {"path_output", json::value_t::string},
                    {"type", json::value_t::string},
                    {"tx", json::value_t::string},
                    {"wavelength", json::value_t::number_float},
                    {"sweep", json::value_t::string},
                },
                {});
            reconstruct_at(t,
                TaskType{
                    .path_output = js.at("path_output").template get<std::string>(),
                    .antenna_id = js.at("tx").template get<std::string>(),
                    .wavelength = js.at("wavelength").template get<double>(),
                    .sweep_id = js.at("sweep").template get<std::string>(), //
                });
        }

        template <AnyJson JsonType>
        void load_voltgain_over_points(JsonType const& js, Task& t)
        {
            using TaskType = VoltGainOverPoints;
            serialization::assert_structure(js,
                TaskType::name,
                {
                    {"path_output", json::value_t::string},
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
            reconstruct_at(t,
                TaskType{
                    .path_output = js.at("path_output").template get<std::string>(),
                    .ref_id = js.at("ref").template get<std::string>(),
                    .tx_id = js.at("tx").template get<std::string>(),
                    .rx_id = js.at("rx").template get<std::string>(),
                    .tx_codebook = js.at("tx_codebook").template get<std::vector<std::string>>(),
                    .rx_codebook = js.at("rx_codebook").template get<std::vector<std::string>>(),
                    .points = js.at("points").template get<std::vector<Pos>>(),
                    .wavelength = js.at("wavelength").template get<double>(), //
                });
        }

        template <AnyJson JsonType>
        void load_voltgain_over_geometry(JsonType const& js, Task& t)
        {
            using TaskType = VoltGainOverGeometry;
            serialization::assert_structure(js,
                TaskType::name,
                {
                    {"path_output", json::value_t::string},
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
            reconstruct_at(t,
                TaskType{
                    .path_output = js.at("path_output").template get<std::string>(),
                    .ref_id = js.at("ref").template get<std::string>(),
                    .tx_id = js.at("tx").template get<std::string>(),
                    .rx_id = js.at("rx").template get<std::string>(),
                    .tx_codebook = js.at("tx_codebook").template get<std::vector<std::string>>(),
                    .rx_codebook = js.at("rx_codebook").template get<std::vector<std::string>>(),
                    .geo_id = js.at("geo").template get<std::string>(),
                    .n_dim1 = js.at("n_dim1").template get<std::size_t>(),
                    .n_dim2 = js.at("n_dim2").template get<std::size_t>(),
                    .wavelength = js.at("wavelength").template get<double>(), //
                });
        }

        template <AnyJson JsonType>
        void load_voltgain_over_geometry_at_wavelength(JsonType const& js, Task& t)
        {
            using TaskType = VoltGainOverGeometryAtWavelength;
            serialization::assert_structure(js,
                TaskType::name,
                {
                    {"path_output", json::value_t::string},
                    {"type", json::value_t::string},
                    {"ref", json::value_t::string},
                    {"tx", json::value_t::string},
                    {"rx", json::value_t::string},
                    {"tx_codebook", json::value_t::array},
                    {"rx_codebook", json::value_t::array},
                    {"geo", json::value_t::string},
                    {"n_dim1", json::value_t::number_integer},
                    {"n_dim2", json::value_t::number_integer},
                    {"sweep", json::value_t::string},
                },
                {});
            reconstruct_at(t,
                TaskType{
                    .path_output = js.at("path_output").template get<std::string>(),
                    .ref_id = js.at("ref").template get<std::string>(),
                    .tx_id = js.at("tx").template get<std::string>(),
                    .rx_id = js.at("rx").template get<std::string>(),
                    .tx_codebook = js.at("tx_codebook").template get<std::vector<std::string>>(),
                    .rx_codebook = js.at("rx_codebook").template get<std::vector<std::string>>(),
                    .geo_id = js.at("geo").template get<std::string>(),
                    .n_dim1 = js.at("n_dim1").template get<std::size_t>(),
                    .n_dim2 = js.at("n_dim2").template get<std::size_t>(),
                    .sweep_wavelength_id = js.at("sweep").template get<std::string>(), //
                });
        }

        template <AnyJson JsonType>
        void load_voltgain_peaks_and_cutoffs(JsonType const& js, Task& t)
        {
            using TaskType = VoltGainPeakAndCutoffs;
            serialization::assert_structure(js,
                TaskType::name,
                {
                    {"path_output", json::value_t::string},
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
            reconstruct_at(t,
                TaskType{
                    .path_output = js.at("path_output").template get<std::string>(),
                    .ref_id = js.at("ref").template get<std::string>(),
                    .tx_id = js.at("tx").template get<std::string>(),
                    .rx_id = js.at("rx").template get<std::string>(),
                    .tx_codebook = js.at("tx_codebook").template get<std::vector<std::string>>(),
                    .rx_codebook = js.at("rx_codebook").template get<std::vector<std::string>>(),
                    .curve_id = js.at("curve").template get<std::string>(),
                    .n_scan = js.at("n_scan").template get<std::size_t>(),
                    .ratio = js.at("ratio").template get<double>(), //
                    .wavelength = js.at("wavelength").template get<double>(),
                });
        }
    } // namespace

    template <AnyJson JsonType>
    void to_json(JsonType& js, Task const& task)
    {
        if (auto* t = std::get_if<DirectivityOverPolarAtAzimuth>(&task))
        {
            js = JsonType{
                {"type", t->name},
                {"path_output", t->path_output},
                {"tx", t->antenna_id},
                {"wavelength", t->wavelength},
                {"sweep", t->sweep_id} //
            };
        }
        else if (auto* t = std::get_if<VoltGainOverPoints>(&task))
        {
            js = JsonType{
                {"type", t->name},
                {"path_output", t->path_output},
                {"ref", t->ref_id},
                {"tx", t->tx_id},
                {"rx", t->rx_id},
                {"tx_codebook", t->tx_codebook},
                {"rx_codebook", t->rx_codebook},
                {"points", t->points},
                {"wavelength", t->wavelength} //
            };
        }
        else if (auto* t = std::get_if<VoltGainOverGeometry>(&task))
        {
            js = JsonType{
                {"type", t->name},
                {"path_output", t->path_output},
                {"ref", t->ref_id},
                {"tx", t->tx_id},
                {"rx", t->rx_id},
                {"tx_codebook", t->tx_codebook},
                {"rx_codebook", t->rx_codebook},
                {"geo", t->geo_id},
                {"n_dim1", t->n_dim1},
                {"n_dim2", t->n_dim2},
                {"wavelength", t->wavelength} //
            };
        }
        else if (auto* t = std::get_if<VoltGainOverGeometryAtWavelength>(&task))
        {
            js = JsonType{
                {"type", t->name},
                {"path_output", t->path_output},
                {"ref", t->ref_id},
                {"tx", t->tx_id},
                {"rx", t->rx_id},
                {"tx_codebook", t->tx_codebook},
                {"rx_codebook", t->rx_codebook},
                {"geo", t->geo_id},
                {"n_dim1", t->n_dim1},
                {"n_dim2", t->n_dim2},
                {"sweep", t->sweep_wavelength_id} //
            };
        }
        else if (auto* t = std::get_if<VoltGainPeakAndCutoffs>(&task))
        {
            js = JsonType{
                    {"type", t->name},
                    {"path_output", t->path_output},
                    {"ref", t->ref_id},
                    {"tx", t->tx_id},
                    {"rx", t->rx_id},
                    {"tx_codebook", t->tx_codebook},
                    {"rx_codebook", t->rx_codebook},
                    {"curve", t->curve_id},
                    {"n_scan", t->n_scan},
                    {"ratio", t->ratio},
                    {"wavelength", t->wavelength} //
            };
        }
        else
            throw SimulationError("Unknown task object");
    }

    template <AnyJson JsonType>
    void from_json(JsonType const& js, Task& task)
    {
        if (!js.contains("type")) throw SimulationError("Missing task type");
        if (js.at("type").type() != nlohmann::json::value_t::string)
            throw SimulationError("Task attribute type must be string, but is {}", js.at("type").type_name());
        auto const type = js.at("type").template get<std::string>();

        if (type == DirectivityOverPolarAtAzimuth::name)
            load_directivity_over_polar_at_azimuth(js, task);
        else if (type == VoltGainOverPoints::name)
            load_voltgain_over_points(js, task);
        else if (type == VoltGainOverGeometry::name)
            load_voltgain_over_geometry(js, task);
        else if (type == VoltGainOverGeometryAtWavelength::name)
            load_voltgain_over_geometry_at_wavelength(js, task);
        else if (type == VoltGainPeakAndCutoffs::name)
            load_voltgain_peaks_and_cutoffs(js, task);
        else
            throw SimulationError("Unknown task type '{}'", type);
    }

    // -----------------------------------------------------------------------------
    // EXPLICIT INSTANTIATIONS
    // -----------------------------------------------------------------------------
    template void to_json(nlohmann::json&, Task const&);
    template void to_json(nlohmann::ordered_json&, Task const&);
    template void from_json(nlohmann::json const&, Task&);
    template void from_json(nlohmann::ordered_json const&, Task&);
} // namespace setup::task
