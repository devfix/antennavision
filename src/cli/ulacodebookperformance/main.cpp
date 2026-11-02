//
// Created by core on 2026-10-31.
//

#include <algorithm>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <nlohmann/json.hpp>
#include <numbers>
#include <print>
#include <string>
#include <vector>
#include "appparams.hpp"
#include "codebooks/ulaftcodebook.hpp"
#include "components/antenna.hpp"
#include "components/radiatorarray.hpp"
#include "math/coords.hpp"
#include "spaces.hpp"
#include "three.hpp"

using namespace antennavision;
using namespace components;
using reference::Reference;

constexpr double ABS_THRESHOLD_TOLERANCE = 0.0001;

// ---- begin ccdf
namespace
{
    /// n values evenly spaced on [lo, hi], both ends included (same as numpy.linspace). These are the CCDF thresholds;
    /// the python side reconstructs them from (min, max, n_bins).
    std::vector<double> evenly_spaced(double lo, double hi, std::size_t n)
    {
        std::vector<double> values(n, lo);
        for (std::size_t i = 1; i < n; i++) values[i] = std::lerp(lo, hi, static_cast<double>(i) / static_cast<double>(n - 1));
        return values;
    }

    /// Complementary CDFs of a set of (linear) gains
    struct Ccdfs
    {
        std::vector<double> linear; ///< P(gain >= t), t evenly spaced on [linear_min, linear_max]
        std::vector<double> decibel; ///< P(10 log10(gain) >= t), t evenly spaced on [decibel_min, decibel_max]
        double gain_min{};
        double gain_max{};
        double coverage{};
    };

    Ccdfs calc_ccdfs(std::vector<double> gains, std::size_t n_bins, double target_ratio, double linear_min, double linear_max, double decibel_min, double decibel_max)
    {
        if (gains.empty()) throw SimulationError("calc_ccdfs: no gains");
        std::ranges::sort(gains);

        // fraction of gains >= threshold (linear)
        auto const ccdf = [&gains](double threshold)
        {
            auto const first = std::ranges::lower_bound(gains, threshold - ABS_THRESHOLD_TOLERANCE);
            return static_cast<double>(gains.end() - first) / static_cast<double>(gains.size());
        };

        const auto [min, max] = std::ranges::minmax(gains);

        Ccdfs result;
        result.linear.reserve(n_bins);
        result.decibel.reserve(n_bins);
        result.gain_min = min;
        result.gain_max = max;

        auto const first = std::ranges::lower_bound(gains, target_ratio);
        result.coverage = static_cast<double>(gains.end() - first) / static_cast<double>(gains.size());

        for (double t : evenly_spaced(linear_min, linear_max, n_bins)) result.linear.push_back(ccdf(t));
        // compare in the linear domain, so gains of 0 (-inf dB) need no special handling
        for (double t_db : evenly_spaced(decibel_min, decibel_max, n_bins)) result.decibel.push_back(ccdf(std::pow(10.0, t_db / 10.0)));
        return result;
    }
} // namespace

// ---- end ccdf

// ---- begin capacity
namespace
{
    /// Gains below this are clipped in -E[log2(eta)], otherwise a single deep null dominates the mean
    constexpr double ETA_FLOOR = 1e-6;

    /// Capacity metrics of a set of normalized gains eta = G / G_max (single stream, SNR_0 = SNR of an ideally focused beam)
    struct CapacityMetrics
    {
        double mean_eta{}; ///< E[eta]
        double mean_eta_db{}; ///< 10 log10 E[eta]
        double delta_c_inf{}; ///< -E[log2 eta] in bit/s/Hz: capacity loss versus ideal focusing for SNR_0 -> infinity
        double clipped_fraction{}; ///< fraction of the samples below ETA_FLOOR (clipped in delta_c_inf)
        double eta_outage{}; ///< eps-quantile of eta: exceeded by (1 - eps) of the positions
        std::vector<double> snr_db; ///< SNR_0 grid in dB
        std::vector<double> capacity_ideal; ///< log2(1 + SNR_0)
        std::vector<double> capacity_ergodic; ///< E[log2(1 + SNR_0 eta)]
        std::vector<double> capacity_outage; ///< log2(1 + SNR_0 eta_outage): rate that (1 - eps) of the positions reach
        std::vector<double> capacity_efficiency; ///< capacity_ergodic / capacity_ideal
    };

    /// q-quantile of sorted values, linear interpolation (same as numpy.quantile)
    double quantile_sorted(std::vector<double> const& sorted, double q)
    {
        double const pos = q * static_cast<double>(sorted.size() - 1);
        auto const lo = static_cast<std::size_t>(std::floor(pos));
        auto const hi = std::min(lo + 1, sorted.size() - 1);
        return std::lerp(sorted[lo], sorted[hi], pos - static_cast<double>(lo));
    }

    CapacityMetrics calc_capacity_metrics(std::vector<double> gains, std::size_t n_snr, double snr_db_min, double snr_db_max, double outage_eps)
    {
        if (gains.empty()) throw SimulationError("calc_capacity_metrics: no gains");
        if (not(outage_eps > 0.0 and outage_eps < 1.0)) throw SimulationError("calc_capacity_metrics: invalid outage epsilon {}", outage_eps);
        std::ranges::sort(gains);
        auto const n = static_cast<double>(gains.size());
        double constexpr ln2 = std::numbers::ln2;

        CapacityMetrics m;
        double sum_eta = 0.0;
        double sum_log2 = 0.0;
        std::size_t n_clipped = 0;
        for (double g : gains)
        {
            sum_eta += g;
            if (g < ETA_FLOOR) n_clipped++;
            sum_log2 += std::log2(std::max(g, ETA_FLOOR));
        }
        m.mean_eta = sum_eta / n;
        m.mean_eta_db = 10.0 * std::log10(m.mean_eta);
        m.delta_c_inf = -sum_log2 / n;
        m.clipped_fraction = static_cast<double>(n_clipped) / n;
        m.eta_outage = quantile_sorted(gains, outage_eps);

        for (double db : evenly_spaced(snr_db_min, snr_db_max, n_snr))
        {
            double const snr = std::pow(10.0, db / 10.0);
            double sum_c = 0.0;
            for (double g : gains) sum_c += std::log1p(snr * g);
            double const ideal = std::log1p(snr) / ln2;
            double const ergodic = sum_c / n / ln2;
            m.snr_db.push_back(db);
            m.capacity_ideal.push_back(ideal);
            m.capacity_ergodic.push_back(ergodic);
            // C is monotone in eta, so the eps-quantile of C is C of the eps-quantile of eta
            m.capacity_outage.push_back(std::log1p(snr * m.eta_outage) / ln2);
            m.capacity_efficiency.push_back(ergodic / ideal);
        }
        return m;
    }
} // namespace

// ---- end capacity

int main(int argc, char* argv[])
{
    AppParams const params = AppParams::parse(argc, argv);
    params.print();

    auto codebook = codebooks::UlaFtCodebook(params.path_codebook);
    auto const wavelength = codebook.wavelength();
    std::println("UlaFtCodebook");
    std::println("  description: {}", codebook.description());
    std::println("  n_elements: {}", codebook.n_elements());
    std::println("  wavelength: {:.03f} mm", codebook.wavelength() * 1e3);
    std::println("  relative_spacing: {:.03f}", codebook.relative_spacing());
    std::println("  power_ratio: {:.03f}", codebook.power_ratio());
    std::println("  amplitude_ratio: {:.06f}", codebook.amplitude_ratio());
    std::println("  n_sectors: {}", codebook.n_sectors());

    std::array<Antenna, 3> antennas = {
        RadiatorArray::create({
            .type = RadiatorArray::Type::UniformLinearArray,
            .id = "tx",
            .origin_id = "",
            .rot = {},
            .prototype_desc =
                {
                    .type = Radiator::Type::IsotropicRadiator,
                },
            .parameters = RadiatorArray::UniformLinearParameters{.spacing = 0.5 * wavelength, .size = codebook.n_elements()} //
        }),
        Radiator::create({
            .type = Radiator::Type::IsotropicRadiator,
            .id = "tx_norm",
            .origin_id = "", //
        }),
        Radiator::create({
            .type = Radiator::Type::IsotropicRadiator,
            .id = "rx",
            .origin_id = "ref_rx", //
        }) //
    };
    Antenna& tx = antennas[0];
    Antenna& tx_norm = antennas[1];
    Antenna& rx = antennas[2];
    std::array references = {
        Reference{}, // create origin reference
        Reference::create( //
            "ref_rx",
            "",
            {},
            {} //
            ) //
    };
    antenna::rebind_origin_pointers(antennas, references);
    setup::SimParams sim_params{.system_wavelength = wavelength, .enable_path_loss = params.enable_path_loss};
    Reference& ref_rx = references[1];

    // three::export_context(ctx, sim_params.system_wavelength, params.path_objects);

    auto const n_elements = static_cast<double>(codebook.n_elements());

    /// Normalized gain |G_V|^2 / N of the vector (tas, trs) with the receiver at `pos`
    auto const gain_at = [&](Pos const& pos, double tas, double trs)
    {
        ref_rx.pos = pos; // update receiver location
        auto voltage_gain = antenna::calc_voltage_gain(tx, rx, wavelength, codebook.get_vector(tas, trs), {1}, sim_params);
        if (params.enable_path_loss)
        {
            auto gain_norm = antenna::calc_voltage_gain(tx_norm, rx, wavelength, {1}, {1}, sim_params);
            voltage_gain /= gain_norm;
        }
        return math::square(std::abs(voltage_gain)) / n_elements;
    };

    std::println("Pass 1: Running Sector Simulation");
    std::cout << std::flush;

    // ---- sector CCDF: the receiver is placed on every sector center, i.e. in the focal point of its own vector
    // (the gain has to be close to 1 there; quantized codebooks are lower by their quantization loss)
    std::vector<double> sector_gains;
    {
        auto const centers = codebook.get_sector_centers();
        sector_gains.reserve(centers.size());
        for (std::size_t i = 0; i < centers.size(); i++)
        {
            auto const [tas, trs] = centers[i];
            if (not(std::abs(tas) <= 1.0 and trs > 0.0)) throw SimulationError("sector center {} (tas={}, trs={}) is outside of the TTS", i, tas, trs);

            // TTS -> Cartesian, same mapping as TTSSpace: r = lambda_0 / trs, cos(polar) = tas
            double const r = wavelength / trs;
            sector_gains.push_back(gain_at(Pos{r * tas, r * std::sqrt(1.0 - tas * tas), 0.0}, tas, trs));
        }
        assert(sector_gains.size() == codebook.n_sectors());
    }

    std::println("Pass 2: Computing Sector CCDF");
    std::cout << std::flush;

    auto const sector_ccdfs = calc_ccdfs(sector_gains,
        params.n_sector_points,
        codebook.power_ratio(),
        params.sector_ccdf_linear_min,
        params.sector_ccdf_linear_max,
        params.sector_ccdf_decibel_min,
        params.sector_ccdf_decibel_max);
    std::println("Sector CCDF:");
    std::println("  gain min={:.6f}", sector_ccdfs.gain_min);
    std::println("  gain max={:.6f}", sector_ccdfs.gain_max);

    // ---- service CCDF: Monte Carlo simulation at random receiver positions
    std::println("Pass 3: Running Service Simulation");
    std::cout << std::flush;

    RandomSpace rs = PolarNearFieldSpace::create(codebook.n_elements(), wavelength, codebook.amplitude_ratio());

    std::vector<double> gains;
    gains.reserve(params.n_samples);

    std::cout << std::flush;
    double service_gain_min = std::numeric_limits<double>::infinity();
    PolarPos pos_gain_min{};

    auto const progress_steps = std::max<std::size_t>(1, static_cast<std::size_t>(std::round(static_cast<double>(params.n_samples) / 1000.0)));

    while (gains.size() < params.n_samples)
    {
        std::print("\033[2K\rSimulating: {: 5.1f}%", 100.0 * static_cast<double>(gains.size()) / static_cast<double>(params.n_samples));
        std::cout << std::flush;
        for (std::size_t k = 0; k < progress_steps and gains.size() < params.n_samples; k++)
        {
            auto [cart_pos, polar_pos] = rs.visit([](auto& space) -> std::pair<Pos, PolarPos> { return space.random_pos(); });

            double gain = gain_at(cart_pos, std::cos(polar_pos.polar), wavelength / polar_pos.r);
            if (gain < service_gain_min)
            {
                service_gain_min = gain;
                pos_gain_min = {polar_pos.r / wavelength, polar_pos.polar};
            }
            gains.push_back(gain);
        }
    }
    std::println("\033[2K\r");

    assert(gains.size() == params.n_samples);

    std::println("Pass 4: Computing Service Capacity Metrics");
    std::cout << std::flush;

    // gains are G / G_max = eta of the codeword that is selected for each receiver position (see main notes)
    auto const capacity = calc_capacity_metrics(gains, params.n_snr_points, params.snr_db_min, params.snr_db_max, params.outage_epsilon);
    std::println("Service capacity (eps={}):", params.outage_epsilon);
    std::println("  mean eta={:.6f} ({:.3f} dB)", capacity.mean_eta, capacity.mean_eta_db);
    std::println("  high-SNR capacity loss={:.4f} bit/s/Hz ({:.4g} % of the samples clipped at {:g})", capacity.delta_c_inf, 100.0 * capacity.clipped_fraction, ETA_FLOOR);
    std::println("  outage eta={:.6f} ({:.3f} dB)", capacity.eta_outage, 10.0 * std::log10(capacity.eta_outage));
    for (std::size_t i : {std::size_t{0}, capacity.snr_db.size() / 2, capacity.snr_db.size() - 1})
        std::println("  SNR_0={:6.1f} dB: ideal={:.3f} ergodic={:.3f} outage={:.3f} efficiency={:.4f}",
            capacity.snr_db[i],
            capacity.capacity_ideal[i],
            capacity.capacity_ergodic[i],
            capacity.capacity_outage[i],
            capacity.capacity_efficiency[i]);

    std::println("Pass 5: Computing Service CCDF");
    std::cout << std::flush;

    auto const service_ccdfs = calc_ccdfs(std::move(gains),
        params.n_service_points,
        codebook.power_ratio(),
        params.service_ccdf_linear_min,
        params.service_ccdf_linear_max,
        params.service_ccdf_decibel_min,
        params.service_ccdf_decibel_max);
    std::println("Service CCDF:");
    std::println("  gain min={:.6f}", service_ccdfs.gain_min);
    std::println("  gain max={:.6f}", service_ccdfs.gain_max);
    std::println("  worst point: {:.03f} at r={:.06f} lambda, polar={:.06f} pi", service_ccdfs.gain_min, pos_gain_min.r, pos_gain_min.polar / pi);

    // ---- result: the keys are the fields of CodebookPerformanceResult (codebook.py)
    ojson const result = {
        {"enable_path_loss", params.enable_path_loss},
        {"n_samples", params.n_samples},
        {"n_sector_points", params.n_sector_points},
        {"n_service_points", params.n_service_points},
        {"sector_ccdf_linear_min", params.sector_ccdf_linear_min},
        {"sector_ccdf_linear_max", params.sector_ccdf_linear_max},
        {"sector_ccdf_decibel_min", params.sector_ccdf_decibel_min},
        {"sector_ccdf_decibel_max", params.sector_ccdf_decibel_max},
        {"service_ccdf_linear_min", params.service_ccdf_linear_min},
        {"service_ccdf_linear_max", params.service_ccdf_linear_max},
        {"service_ccdf_decibel_min", params.service_ccdf_decibel_min},
        {"service_ccdf_decibel_max", params.service_ccdf_decibel_max},
        {"n_snr_points", params.n_snr_points},
        {"snr_db_min", params.snr_db_min},
        {"snr_db_max", params.snr_db_max},
        {"outage_epsilon", params.outage_epsilon},
        {"service_mean_eta", capacity.mean_eta},
        {"service_mean_eta_db", capacity.mean_eta_db},
        {"service_delta_c_inf", capacity.delta_c_inf},
        {"service_delta_c_inf_clipped_fraction", capacity.clipped_fraction},
        {"service_eta_outage", capacity.eta_outage},
        {"service_capacity_snr_db", capacity.snr_db},
        {"service_capacity_ideal", capacity.capacity_ideal},
        {"service_capacity_ergodic", capacity.capacity_ergodic},
        {"service_capacity_outage", capacity.capacity_outage},
        {"service_capacity_efficiency", capacity.capacity_efficiency},
        {"sector_gain_min", sector_ccdfs.gain_min},
        {"sector_gain_max", sector_ccdfs.gain_max},
        {"sector_coverage", sector_ccdfs.coverage},
        {"service_gain_min", service_ccdfs.gain_min},
        {"service_gain_max", service_ccdfs.gain_max},
        {"service_coverage", service_ccdfs.coverage},
        {"sector_ccdf_linear", sector_ccdfs.linear},
        {"sector_ccdf_decibel", sector_ccdfs.decibel},
        {"service_ccdf_linear", service_ccdfs.linear},
        {"service_ccdf_decibel", service_ccdfs.decibel},
    };

    serialization::store_file(result, params.path_result);
}
