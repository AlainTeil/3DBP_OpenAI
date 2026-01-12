#include "common.hpp"

#include "bp/algorithms/algorithms.hpp"

namespace bp::algorithms {

PackingResult guillotine_ffd(const Instance& instance, unsigned int seed) {
    (void)seed;
    auto ordered = sort_by_volume_desc(instance.boxes);
    return greedy_pack(instance, ordered, GreedyConfig{SplitPolicy::Guillotine});
}

}  // namespace bp::algorithms
