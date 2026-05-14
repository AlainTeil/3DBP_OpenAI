#include "common.hpp"

#include "bp/algorithms/algorithms.hpp"

namespace bp::algorithms {

PackingResult online_first_fit(const Instance &instance, unsigned int seed) {
    (void)seed;
    return greedy_pack(instance, instance.boxes, GreedyConfig{SplitPolicy::Maximal});
}

} // namespace bp::algorithms
