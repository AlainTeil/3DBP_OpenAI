#include "bp/packer.hpp"

#include <stdexcept>
#include <chrono>
#include <unordered_set>
#include <ctime>

#include "bp/algorithms/algorithms.hpp"

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

PackingStats compute_stats(const Instance& instance, const PackingResult& result) {
    PackingStats s;
    s.boxes_total = static_cast<int>(instance.boxes.size());
    s.boxes_placed = static_cast<int>(result.placements.size());
    for (const auto& b : instance.boxes) {
        s.volume_boxes += volume(b.size);
    }
    std::unordered_set<std::string> used_bins;
    for (const auto& p : result.placements) {
        s.volume_packed += volume(p.orientation);
        used_bins.insert(p.bin_id);
    }
    for (const auto& bin : instance.bins) {
        if (used_bins.count(bin.id)) {
            s.volume_bins_used += volume(bin.size);
        }
    }
    if (s.volume_bins_used > 0) {
        s.fill_ratio = static_cast<double>(s.volume_packed) / static_cast<double>(s.volume_bins_used);
    }
    return s;
}

}  // namespace

PackingResult pack(const Instance& instance, const PackOptions& options) {
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
    res.stats = compute_stats(instance, res);
    res.metadata.algorithm = algo_to_string(options.algorithm);
    res.metadata.seed = options.seed;
    res.metadata.iterations = options.iterations;
    const auto now = std::chrono::system_clock::now();
    const auto now_time = std::chrono::system_clock::to_time_t(now);
    res.metadata.timestamp = std::string(std::ctime(&now_time));
    if (!res.metadata.timestamp.empty() && res.metadata.timestamp.back() == '\n') {
        res.metadata.timestamp.pop_back();
    }
    return res;
    }

}  // namespace bp
