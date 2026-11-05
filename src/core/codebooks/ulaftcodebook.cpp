//
// Created by core on 31.10.26.
//

#include "codebooks/ulaftcodebook.hpp"

#include <algorithm>
#include <cmath>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

#include "serialization.hpp"
#include "types/json.hpp"

namespace antennavision::codebooks
{
    namespace
    {
        /// iterator to the element of the sorted range [first, last) that is nearest to value
        template <class It, class Key>
        It nearest(It first, It last, double value, Key key)
        {
            It it = std::lower_bound(first, last, value, [&](const auto& element, double v) { return key(element) < v; });
            if (it == first) return first;
            if (it == last) return last - 1;
            return value - key(*(it - 1)) <= key(*it) - value ? it - 1 : it;
        }
    } // namespace

    UlaFtCodebook::UlaFtCodebook(const std::filesystem::path& path)
    {
        const json doc = serialization::load_file<json>(path);

        description_ = doc.at("description").get<std::string>();
        n_elements_ = doc.at("n_elements").get<std::size_t>();
        wavelength_ = doc.at("wavelength").get<double>();
        d_rel_ = doc.at("d_rel").get<double>();
        power_ratio_ = doc.at("power_ratio").get<double>();
        r_min_rel_ = doc.at("r_min_rel").get<double>();
        n_sectors_ = doc.at("n_sectors").get<std::size_t>();

        if (n_elements_ == 0) throw std::runtime_error("UlaFtCodebook: n_elements must be positive");
        trs_.reserve(n_sectors_);
        weights_.reserve(n_elements_ * n_sectors_);

        // "weights": [ [tas, [ [trs, [ [re, im], ... ]], ... ]], ... ]
        for (const auto& col : doc.at("weights"))
        {
            const double tas = col.at(0).get<double>();
            if (!columns_.empty() && !(tas > columns_.back().tas)) throw std::runtime_error("UlaFtCodebook: tas must increase from column to column");
            columns_.push_back({tas, trs_.size(), 0});

            for (const auto& row : col.at(1))
            {
                const double trs = row.at(0).get<double>();
                if (columns_.back().count > 0 && !(trs > trs_.back())) throw std::runtime_error("UlaFtCodebook: trs must increase within a column");

                const auto& vector = row.at(1);
                if (vector.size() != n_elements_)
                    throw std::runtime_error("UlaFtCodebook: vector with " + std::to_string(vector.size()) + " elements, expected " + std::to_string(n_elements_));
                for (const auto& element : vector) weights_.emplace_back(element.at(0).get<double>(), element.at(1).get<double>());

                trs_.push_back(trs);
                ++columns_.back().count;
            }
            if (columns_.back().count == 0) throw std::runtime_error("UlaFtCodebook: empty column");
        }

        if (columns_.empty()) throw std::runtime_error("UlaFtCodebook: no weights in '" + path.string() + "'");
        if (n_sectors_ != trs_.size())
            throw std::runtime_error("UlaFtCodebook: n_sectors = " + std::to_string(n_sectors_) + " but the file has " + std::to_string(trs_.size()) + " vectors");
        weights_.shrink_to_fit();
    }

    const std::string& UlaFtCodebook::description() const { return description_; }

    std::size_t UlaFtCodebook::n_elements() const { return n_elements_; }

    double UlaFtCodebook::wavelength() const { return wavelength_; }

    double UlaFtCodebook::relative_spacing() const { return d_rel_; }

    double UlaFtCodebook::power_ratio() const { return power_ratio_; }

    double UlaFtCodebook::r_min_rel() const { return r_min_rel_; }

    std::size_t UlaFtCodebook::n_sectors() const { return n_sectors_; }

    std::size_t UlaFtCodebook::column_count() const { return columns_.size(); }

    std::size_t UlaFtCodebook::nearest_index(double tas, double trs) const
    {
        if (std::isnan(tas) || std::isnan(trs)) throw std::invalid_argument("UlaFtCodebook: tas and trs must not be NaN");
        const auto column = nearest(columns_.begin(), columns_.end(), tas, [](const Column& c) { return c.tas; });
        const auto first = trs_.begin() + static_cast<std::ptrdiff_t>(column->first);
        const auto row = nearest(first, first + static_cast<std::ptrdiff_t>(column->count), trs, [](double t) { return t; });
        return static_cast<std::size_t>(row - trs_.begin());
    }

    std::pair<double, double> UlaFtCodebook::nearest_point(double tas, double trs) const
    {
        const std::size_t i = nearest_index(tas, trs);
        const auto column = std::upper_bound(columns_.begin(), columns_.end(), i, [](std::size_t index, const Column& c) { return index < c.first; }) - 1;
        return {column->tas, trs_[i]};
    }

    std::span<const Complex> UlaFtCodebook::get_vector(double tas, double trs) { return {weights_.data() + nearest_index(tas, trs) * n_elements_, n_elements_}; }

    std::vector<std::pair<double, double>> UlaFtCodebook::get_sector_centers()
    {
        std::vector<std::pair<double, double>> centers;
        centers.reserve(trs_.size());
        for (const Column& column : columns_)
            for (std::size_t i = column.first; i < column.first + column.count; ++i) centers.emplace_back(column.tas, trs_[i]);
        return centers;
    }
} // namespace antennavision::codebooks
