#pragma once

#include "bp/model.hpp"

namespace bp::algorithms {

PackingResult first_fit_decreasing(const Instance &instance, unsigned int seed);

PackingResult next_fit_decreasing_height(const Instance &instance, unsigned int seed);

PackingResult guillotine_ffd(const Instance &instance, unsigned int seed);

PackingResult layered_maximal_space(const Instance &instance, unsigned int seed);

PackingResult maximal_space_ffd(const Instance &instance, unsigned int seed);

PackingResult meta_ga(const Instance &instance, unsigned int seed, int iterations);

PackingResult meta_grasp(const Instance &instance, unsigned int seed, int iterations);

PackingResult meta_sa(const Instance &instance, unsigned int seed, int iterations);

PackingResult online_first_fit(const Instance &instance, unsigned int seed);

} // namespace bp::algorithms
