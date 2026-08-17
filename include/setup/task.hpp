//
// Created by Tristan Krause on 2026-07-28.
//

#pragma once
#include "geometry.hpp"
#include "sweep.hpp"

#include <string_view>

#include "components/antenna.hpp"
#include "context.hpp"

namespace setup::task
{
    enum struct OutputType
    {
        JSON,
        BSON,
        CBOR,
        MSGPACK,
        UBJSON
    };

    struct DirectivityOverPolarAtAzimuth
    {
        static constexpr std::string_view name = "DirectivityOverPolar@Azimuth";
        template <AnyJson JsonType>
        [[nodiscard]] static DirectivityOverPolarAtAzimuth from_json(JsonType& js, Context const& ctx);

        template <AnyJson JsonType>
        void to_json(JsonType& js) const;

        std::filesystem::path output_path;
        components::Antenna const& tx;
        double wavelength;
        sweep::Sweep sweep_azimuth;

        [[nodiscard]] std::string id() const { return std::format("{}.{}.{}", name, components::antenna::get_id(tx), sweep::get_id(sweep_azimuth)); }
    };

    struct VoltGainOverPoints
    {
        static constexpr std::string_view name = "VoltGain(Points)";
        template <AnyJson JsonType>
        [[nodiscard]] static VoltGainOverPoints from_json(JsonType& js, Context const& ctx);

        template <AnyJson JsonType>
        void to_json(JsonType& js) const;

        std::filesystem::path output_path;
        reference::Reference const& ref;
        components::Antenna const& tx;
        components::Antenna const& rx;
        std::vector<std::string> tx_codebook;
        std::vector<std::string> rx_codebook;
        std::vector<Pos> points;
        double wavelength;

        [[nodiscard]] std::string id() const
        {
            return std::format("{}.{}.{}.{}.{}.{:06.0f}",
                name,
                reference::get_id(ref),
                components::antenna::get_id(tx),
                components::antenna::get_id(rx),
                points.size(),
                wavelength * 1e6);
        }
    };

    struct VoltGainOverGeometry
    {
        static constexpr std::string_view name = "VoltGain(Geometry)";
        template <AnyJson JsonType>
        [[nodiscard]] static VoltGainOverGeometry from_json(JsonType& js, Context const& ctx);

        template <AnyJson JsonType>
        void to_json(JsonType& js) const;

        std::filesystem::path output_path;
        reference::Reference const& ref;
        components::Antenna const& tx;
        components::Antenna const& rx;
        std::vector<std::string> tx_codebook;
        std::vector<std::string> rx_codebook;
        geometry::Geometry const& geo;
        std::size_t n_dim1;
        std::size_t n_dim2;
        double wavelength;

        [[nodiscard]] std::string id() const
        { //
            return std::format("{}.{}.{}.{}.{}.{}.{}.{:06.0f}",
                name,
                reference::get_id(ref),
                components::antenna::get_id(tx),
                components::antenna::get_id(rx),
                geometry::get_id(geo),
                n_dim1,
                n_dim2,
                wavelength * 1e6);
        }
    };

    struct VoltGainOverGeometryAtWavelength
    {
        static constexpr std::string_view name = "VoltGain(Geometry)@Wavelength";
        template <AnyJson JsonType>
        [[nodiscard]] static VoltGainOverGeometryAtWavelength from_json(JsonType& js, Context const& ctx);

        template <AnyJson JsonType>
        void to_json(JsonType& js) const;

        std::filesystem::path output_path;
        reference::Reference const& ref;
        components::Antenna const& tx;
        components::Antenna const& rx;
        std::vector<std::string> tx_codebook;
        std::vector<std::string> rx_codebook;
        geometry::Geometry geo;
        std::size_t n_dim1;
        std::size_t n_dim2;
        sweep::Sweep sweep_wavelength;

        [[nodiscard]] std::string id() const
        { //
            return std::format("{}.{}.{}.{}.{}.{}.{}",
                name,
                reference::get_id(ref),
                components::antenna::get_id(tx),
                components::antenna::get_id(rx),
                geometry::get_id(geo),
                n_dim1,
                n_dim2,
                sweep::get_id(sweep_wavelength));
        }
    };

    struct VoltGainPeakAndCutoffs
    {
        static constexpr std::string_view name = "VoltGain.PeakAndCutoffs";
        template <AnyJson JsonType>
        [[nodiscard]] static VoltGainPeakAndCutoffs from_json(JsonType& js, Context const& ctx);

        template <AnyJson JsonType>
        void to_json(JsonType& js) const;

        std::filesystem::path output_path;
        reference::Reference const& ref;
        components::Antenna const& tx;
        components::Antenna const& rx;
        std::vector<std::string> tx_codebook;
        std::vector<std::string> rx_codebook;
        geometry::Curve curve;
        std::size_t n_scan;
        double ratio;
        double wavelength;

        [[nodiscard]] std::string id() const
        { //
            return std::format("{}.{}.{}.{}.{}.{}.{:03.0f}.{:06.0f}",
                name,
                reference::get_id(ref),
                components::antenna::get_id(tx),
                components::antenna::get_id(rx),
                geometry::get_id(curve),
                n_scan,
                ratio * 1e3,
                wavelength * 1e6);
        }
    };

    using Task = std::variant< //
        DirectivityOverPolarAtAzimuth,
        VoltGainOverPoints,
        VoltGainOverGeometry,
        VoltGainOverGeometryAtWavelength,
        VoltGainPeakAndCutoffs //
        >;

    template <AnyJson JsonType>
    Task from_json(JsonType& js, Context const& ctx);

    template <AnyJson JsonType>
    void to_json(JsonType& js, Task const& task);

    std::optional<OutputType> output_type_from_ext(std::string_view ext);

    [[nodiscard]] constexpr std::string_view get_name(Task const& task) noexcept
    {
        return task.visit([](auto const& t) -> std::string_view { return t.name; });
    }

    [[nodiscard]] constexpr std::filesystem::path const& get_output_path(Task const& task) noexcept
    {
        return task.visit([](auto const& t) -> std::filesystem::path const& { return t.output_path; });
    }

    [[nodiscard]] OutputType get_output_type(Task const& task);

    [[nodiscard]] constexpr std::string get_id(Task const& task) noexcept
    {
        return task.visit([](auto const& t) -> std::string { return t.id(); });
    }
} // namespace setup::task
