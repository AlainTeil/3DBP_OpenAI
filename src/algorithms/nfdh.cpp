#include "common.hpp"

#include <algorithm>

#include "bp/algorithms/algorithms.hpp"

namespace bp::algorithms {

PackingResult next_fit_decreasing_height(const Instance& instance, unsigned int seed) {
    (void)seed;
    std::vector<Box> ordered = instance.boxes;
    std::sort(ordered.begin(), ordered.end(), [](const Box& a, const Box& b) {
        if (a.size.h != b.size.h) {
            return a.size.h > b.size.h;
        }
        return volume(a.size) > volume(b.size);
    });
    return greedy_pack(instance, ordered, GreedyConfig{SplitPolicy::Maximal});
}

}  // namespace bp::algorithms
