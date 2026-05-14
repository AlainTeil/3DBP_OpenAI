#include "common.hpp"

#include <algorithm>

#include "bp/algorithms/algorithms.hpp"

namespace bp::algorithms {

PackingResult layered_maximal_space(const Instance &instance, unsigned int seed) {
    (void)seed;
    std::vector<Box> ordered = instance.boxes;
    std::sort(ordered.begin(), ordered.end(), [](const Box &a, const Box &b) {
        if (a.size.h != b.size.h) {
            return a.size.h > b.size.h;
        }
        const int area_a = a.size.w * a.size.l;
        const int area_b = b.size.w * b.size.l;
        return area_a > area_b;
    });
    return greedy_pack(instance, ordered, GreedyConfig{SplitPolicy::Maximal});
}

} // namespace bp::algorithms
