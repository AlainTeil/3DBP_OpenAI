#pragma once

#include <vector>

#include "bp/model.hpp"

namespace bp::algorithms {

enum class SplitPolicy { Maximal, Guillotine };

/// Configuration for greedy split heuristics.
struct GreedyConfig {
    /// Policy controlling how spaces are subdivided.
    SplitPolicy policy{SplitPolicy::Maximal};
};

PackingResult greedy_pack(const Instance& instance, const std::vector<Box>& ordered_boxes, GreedyConfig config);

std::vector<Box> sort_by_volume_desc(const std::vector<Box>& boxes);

}  // namespace bp::algorithms
