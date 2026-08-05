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
            serialization::assert_structure(js,
                DirectivityOverPolarAtAzimuth::name,
                {
                    {"path_output", json::value_t::string},
                    {"type", json::value_t::string},
                    {"tx", json::value_t::string},
                    {"wavelength", json::value_t::number_float},
                    {"sweep", json::value_t::string},
                },
                {});
            reconstruct_at(t,
                DirectivityOverPolarAtAzimuth{
                    .path_output = js.at("path_output").template get<std::string>(),
                    .antenna_id = js.at("tx").template get<std::string>(),
                    .wavelength = js.at("wavelength").template get<double>(),
                    .sweep_id = js.at("sweep").template get<std::string>(), //
                });
        }

        template <AnyJson JsonType>
        void load_rx_voltage_field_at_wavelength(JsonType const& js, Task& t)
        {
            serialization::assert_structure(js,
                RxVoltageFieldAtWavelength::name,
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
                RxVoltageFieldAtWavelength{
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
    } // namespace

    template <AnyJson JsonType>
    void to_json(JsonType& js, Task const& task)
    {
        if (auto* directivity_over_polar_sweep_azimuth = std::get_if<task::DirectivityOverPolarAtAzimuth>(&task))
        {
            js = JsonType{
                {"type", DirectivityOverPolarAtAzimuth::name},
                {"path_output", directivity_over_polar_sweep_azimuth->path_output},
                {"tx", directivity_over_polar_sweep_azimuth->antenna_id},
                {"wavelength", directivity_over_polar_sweep_azimuth->wavelength},
                {"sweep", directivity_over_polar_sweep_azimuth->sweep_id} //
            };
        }
        else if (auto* rx_voltage_field_at_wavelength = std::get_if<task::RxVoltageFieldAtWavelength>(&task))
        {
            js = JsonType{
                {"type", RxVoltageFieldAtWavelength::name},
                {"path_output", rx_voltage_field_at_wavelength->path_output},
                {"ref", rx_voltage_field_at_wavelength->ref_id},
                {"tx", rx_voltage_field_at_wavelength->tx_id},
                {"rx", rx_voltage_field_at_wavelength->rx_id},
                {"tx_codebook", rx_voltage_field_at_wavelength->tx_codebook},
                {"rx_codebook", rx_voltage_field_at_wavelength->rx_codebook},
                {"geo", rx_voltage_field_at_wavelength->geo_id},
                {"n_dim1", rx_voltage_field_at_wavelength->n_dim1},
                {"n_dim2", rx_voltage_field_at_wavelength->n_dim2},
                {"sweep", rx_voltage_field_at_wavelength->sweep_wavelength_id} //
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

        if (type == "DirectivityOverPolar@Azimuth")
            load_directivity_over_polar_at_azimuth(js, task);
        else if (type == "RxVoltageField@Wavelength")
            load_rx_voltage_field_at_wavelength(js, task);
        else
            throw SimulationError("Unknown geometry type '{}'", type);
    }

    // -----------------------------------------------------------------------------
    // EXPLICIT INSTANTIATIONS
    // -----------------------------------------------------------------------------
    template void to_json(nlohmann::json&, Task const&);
    template void to_json(nlohmann::ordered_json&, Task const&);
    template void from_json(nlohmann::json const&, Task&);
    template void from_json(nlohmann::ordered_json const&, Task&);
} // namespace setup::task
