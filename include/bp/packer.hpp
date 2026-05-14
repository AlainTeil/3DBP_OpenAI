#pragma once

#include <cstdint>
#include <string>

#include "bp/model.hpp"

namespace bp {

/// Algorithms exposed by the public pack() entry point.
enum class AlgorithmId : std::uint8_t {
    /// Greedy first-fit using boxes sorted by descending volume.
    FFD,
    /// Greedy first-fit using boxes sorted by descending height, then volume.
    NFDH,
    /// Greedy first-fit using boxes sorted by descending height, then base area.
    Layered,
    /// Greedy first-fit using disjoint guillotine-style residual spaces.
    Guillotine,
    /// Greedy first-fit using boxes sorted by largest side, then volume.
    MaximalSpace,
    /// Permutation genetic search over greedy packing orderings.
    MetaGA,
    /// Randomized greedy ordering search.
    MetaGRASP,
    /// Simulated annealing over greedy packing orderings.
    MetaSA,
    /// Greedy first-fit in input stream order with no reordering.
    OnlineFFD
};

/// Tuning parameters for packing.
struct PackOptions {
    /// Algorithm to run.
    AlgorithmId algorithm{AlgorithmId::FFD};
    /// RNG seed for stochastic algorithms.
    unsigned int seed{0};
    /// Iteration budget where relevant.
    int iterations{200};
};

/// Run a packing algorithm on a validated instance.
///
/// Throws std::runtime_error when the instance is structurally invalid. Returned results are
/// validated before being handed back; invalid algorithm output is marked PackingStatus::Invalid.
PackingResult pack(const Instance &instance, const PackOptions &options);

} // namespace bp
