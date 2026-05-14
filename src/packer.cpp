#include "bp/packer.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

#include "bp/algorithms/algorithms.hpp"
#include "bp/validation.hpp"

namespace bp {

namespace {

std::string algo_to_string(AlgorithmId id) {
    switch (id) {
    case AlgorithmId::FFD:
        return "ffd";
    case AlgorithmId::NFDH:
        return "nfdh";
    case AlgorithmId::Layered:
        return "layer";
    case AlgorithmId::Guillotine:
        return "guillotine";
    case AlgorithmId::MaximalSpace:
        return "maxspace";
    case AlgorithmId::MetaGA:
        return "meta-ga";
    case AlgorithmId::MetaGRASP:
        return "meta-grasp";
    case AlgorithmId::MetaSA:
        return "meta-sa";
    case AlgorithmId::OnlineFFD:
        return "online-ffd";
    }
    return "unknown";
}

PackingStats compute_stats(const Instance &instance, const PackingResult &result) {
    PackingStats s;
    s.boxes_total = static_cast<int>(instance.boxes.size());
    s.boxes_placed = static_cast<int>(result.placements.size());
    s.boxes_unplaced = static_cast<int>(result.unplaced_box_ids.size());
    for (const auto &b : instance.boxes) {
        s.volume_boxes += volume(b.size);
    }
    std::unordered_set<std::string> used_bins;
    for (const auto &p : result.placements) {
        s.volume_packed += volume(p.orientation);
        used_bins.insert(p.bin_id);
    }
    for (const auto &bin : instance.bins) {
        if (used_bins.contains(bin.id)) {
            s.volume_bins_used += volume(bin.size);
        }
    }
    if (s.volume_bins_used > 0) {
        s.fill_ratio = static_cast<double>(s.volume_packed) / static_cast<double>(s.volume_bins_used);
    }
    return s;
}

std::string utc_timestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto now_time = std::chrono::system_clock::to_time_t(now);
    const auto *utc = std::gmtime(&now_time);
    if (utc == nullptr) {
        return {};
    }
    std::ostringstream out;
    out << std::put_time(utc, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

} // namespace

PackingResult pack(const Instance &instance, const PackOptions &options) {
    const auto instance_report = validate_instance(instance);
    if (!instance_report.valid) {
        std::ostringstream message;
        message << "Invalid packing instance";
        for (const auto &error : instance_report.errors) {
            message << "; " << error;
        }
        throw std::runtime_error(message.str());
    }

    using namespace bp::algorithms;
    PackingResult res;
    switch (options.algorithm) {
    case AlgorithmId::FFD:
        res = first_fit_decreasing(instance, options.seed);
        break;
    case AlgorithmId::NFDH:
        res = next_fit_decreasing_height(instance, options.seed);
        break;
    case AlgorithmId::Layered:
        res = layered_maximal_space(instance, options.seed);
        break;
    case AlgorithmId::Guillotine:
        res = guillotine_ffd(instance, options.seed);
        break;
    case AlgorithmId::MaximalSpace:
        res = maximal_space_ffd(instance, options.seed);
        break;
    case AlgorithmId::MetaGA:
        res = meta_ga(instance, options.seed, options.iterations);
        break;
    case AlgorithmId::MetaGRASP:
        res = meta_grasp(instance, options.seed, options.iterations);
        break;
    case AlgorithmId::MetaSA:
        res = meta_sa(instance, options.seed, options.iterations);
        break;
    case AlgorithmId::OnlineFFD:
        res = online_first_fit(instance, options.seed);
        break;
    default:
        throw std::runtime_error("Unsupported algorithm id");
    }
    res.unplaced_box_ids = unplaced_box_ids(instance, res);
    res.metadata.algorithm = algo_to_string(options.algorithm);
    res.metadata.seed = options.seed;
    res.metadata.iterations = options.iterations;
    res.metadata.timestamp = utc_timestamp();

    const auto result_report = validate_result(instance, res);
    res.validation_errors = result_report.errors;
    if (!result_report.valid) {
        res.status = PackingStatus::Invalid;
        res.feasible = false;
    }
    res.stats = compute_stats(instance, res);
    return res;
}

} // namespace bp
