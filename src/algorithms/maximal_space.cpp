#include "common.hpp"

#include <algorithm>

#include "bp/algorithms/algorithms.hpp"

namespace bp::algorithms {

PackingResult maximal_space_ffd(const Instance &instance, unsigned int seed) {
    (void)seed;
    std::vector<Box> ordered = instance.boxes;
    std::sort(ordered.begin(), ordered.end(), [](const Box &a, const Box &b) {
        const int max_a = std::max({a.size.w, a.size.l, a.size.h});
        const int max_b = std::max({b.size.w, b.size.l, b.size.h});
        if (max_a != max_b) {
            return max_a > max_b;
        }
        return volume(a.size) > volume(b.size);
    });
    return greedy_pack(instance, ordered, GreedyConfig{SplitPolicy::Maximal});
}

} // namespace bp::algorithms
