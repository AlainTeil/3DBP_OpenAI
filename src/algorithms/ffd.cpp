#include "common.hpp"

#include <random>

#include "bp/algorithms/algorithms.hpp"

namespace bp::algorithms {

PackingResult first_fit_decreasing(const Instance &instance, unsigned int seed) {
    (void)seed;
    auto ordered = sort_by_volume_desc(instance.boxes);
    return greedy_pack(instance, ordered, GreedyConfig{SplitPolicy::Maximal});
}

} // namespace bp::algorithms
