#pragma once

#include <string>

#include "bp/model.hpp"

namespace bp {

enum class AlgorithmId {
    FFD,
    NFDH,
    Layered,
    Guillotine,
    MaximalSpace,
    MetaGA,
    MetaGRASP,
    MetaSA,
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

PackingResult pack(const Instance& instance, const PackOptions& options);

}  // namespace bp
