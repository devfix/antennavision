//
// Created by Tristan Krause on 2026-08-17.
//

#include "codebooks/ula_da.hpp"
#include <cmath>
#include <nlohmann/json.hpp>
#include <nanoflann.hpp>

#include "math/coords.hpp"
#include "simulationerror.hpp"
#include "types/math.hpp"

namespace antennavision::codebooks
{
    // 1. Define PointCloud here so it is hidden from the rest of your program
    struct PointCloud
    {
        std::vector<std::array<double, 2>> pts;

        inline size_t kdtree_get_point_count() const { return pts.size(); }

        inline double kdtree_get_pt(const size_t idx, const size_t dim) const { return pts[idx][dim]; }

        template <class BBOX>
        bool kdtree_get_bbox(BBOX& /* bb */) const
        { return false; }
    };

    // 2. Define the Impl struct containing the nanoflann logic and data
    struct ULA_DA::Impl
    {
        PointCloud cartesian_cloud;
        std::vector<cb_vector> beam_vectors;

        using KDTree = nanoflann::KDTreeSingleIndexAdaptor<nanoflann::L2_Simple_Adaptor<double, PointCloud>, PointCloud, 2>;

        std::unique_ptr<KDTree> kd_index;

        // Constructor logic for the implementation
        Impl(const std::vector<std::pair<polar_pos, cb_vector>>& raw_codebook)
        {
            size_t num_points = raw_codebook.size();
            cartesian_cloud.pts.reserve(num_points);
            beam_vectors.reserve(num_points);

            for (const auto& entry : raw_codebook)
            {
                double r = entry.first[0];
                double theta = entry.first[1];

                double x = r * std::cos(theta);
                double y = r * std::sin(theta);

                cartesian_cloud.pts.push_back({x, y});
                beam_vectors.push_back(entry.second);
            }

            kd_index = std::make_unique<KDTree>(2, cartesian_cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10));
            kd_index->buildIndex();
        }
    };

    // 3. Class Method Implementations

    // Initialize the pimpl pointer
    ULA_DA ULA_DA::from_vector(std::vector<std::pair<polar_pos, cb_vector>> const& raw_codebook)
    {
        return {std::make_unique<Impl>(raw_codebook)};
    }

    ULA_DA ULA_DA::from_json(json const& js)
    {
        std::vector<std::pair<polar_pos, cb_vector>> vectors;
        for (auto const& entry : js.at("weights"))
        {
            auto pos = entry.at(0).get<polar_pos>();
            auto weights_comp = entry.at(1).get<std::vector<std::array<double, 2>>>();
            cb_vector coeffs(weights_comp.size());
            std::ranges::transform(weights_comp, coeffs.begin(), [](auto const& w) -> Complex{ return math::complex_from_polar(w.at(0), w.at(1));; });
            vectors.push_back({pos, coeffs});
        }
        return from_vector(vectors);
    }

    ULA_DA ULA_DA::from_file(std::filesystem::path const& p)
    {
        std::ifstream file(p);
        if (!file.is_open()) { throw SimulationError("Could not open codebook. Does the file exist?"); }
        auto const js = json::parse(file);
        file.close();
        return from_json(js);
    }

    // The destructor must be defined here, where the compiler knows the full size of `Impl`.
    // If you leave this in the header, std::unique_ptr will throw a compilation error.
    ULA_DA::~ULA_DA() = default;

    // Delegate the work to the pimpl object
    std::pair<double, ULA_DA::cb_vector> ULA_DA::find_closest(const polar_pos& target_polar) const
    {

        double target_x = target_polar[0] * std::cos(target_polar[1]);
        double target_y = target_polar[0] * std::sin(target_polar[1]);
        double query_pt[2] = {target_x, target_y};

        size_t ret_index;
        double out_dist_sqr;
        nanoflann::KNNResultSet<double> resultSet(1);
        resultSet.init(&ret_index, &out_dist_sqr);

        pimpl->kd_index->findNeighbors(resultSet, &query_pt[0], nanoflann::SearchParameters(10));

        double true_distance = std::sqrt(out_dist_sqr);

        return {true_distance, pimpl->beam_vectors[ret_index]};
    }

    ULA_DA::ULA_DA(std::unique_ptr<Impl> pimpl) : pimpl(std::move(pimpl))
    {}
} // namespace antennavision::codebooks
