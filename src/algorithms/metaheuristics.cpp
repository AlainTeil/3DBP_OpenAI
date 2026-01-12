#include "common.hpp"

#include <algorithm>
#include <cmath>
#include <random>

#include "bp/algorithms/algorithms.hpp"

namespace bp::algorithms {
namespace {

bool better(const PackingResult& a, const PackingResult& b) {
    if (a.feasible != b.feasible) {
        return a.feasible;
    }
    if (a.objective.bins_used != b.objective.bins_used) {
        return a.objective.bins_used < b.objective.bins_used;
    }
    return a.objective.leftover_volume < b.objective.leftover_volume;
}

PackingResult try_order(const Instance& instance, std::vector<Box> order, GreedyConfig cfg) {
    return greedy_pack(instance, order, cfg);
}

}  // namespace

PackingResult meta_ga(const Instance& instance, unsigned int seed, int iterations) {
    std::mt19937 rng(seed ? seed : std::random_device{}());
    const std::size_t pop_size = 12;
    std::vector<std::vector<Box>> population;
    population.reserve(pop_size);
    for (std::size_t i = 0; i < pop_size; ++i) {
        auto order = instance.boxes;
        std::shuffle(order.begin(), order.end(), rng);
        population.push_back(order);
    }

    PackingResult best;
    for (int iter = 0; iter < iterations; ++iter) {
        for (auto& order : population) {
            auto res = try_order(instance, order, GreedyConfig{SplitPolicy::Maximal});
            if (iter == 0 || better(res, best)) {
                best = res;
            }
        }
        // simple crossover: swap segments between pairs
        for (std::size_t i = 0; i + 1 < pop_size; i += 2) {
            auto& a = population[i];
            auto& b = population[i + 1];
            if (a.empty()) {
                continue;
            }
            std::uniform_int_distribution<std::size_t> dist(0, a.size() - 1);
            const auto cut = dist(rng);
            for (std::size_t j = 0; j <= cut && j < a.size() && j < b.size(); ++j) {
                std::swap(a[j], b[j]);
            }
        }
        // mutation
        for (auto& order : population) {
            if (order.size() < 2) {
                continue;
            }
            std::uniform_int_distribution<std::size_t> dist(0, order.size() - 1);
            const auto i1 = dist(rng);
            const auto i2 = dist(rng);
            std::swap(order[i1], order[i2]);
        }
    }
    return best;
}

PackingResult meta_grasp(const Instance& instance, unsigned int seed, int iterations) {
    std::mt19937 rng(seed ? seed : std::random_device{}());
    PackingResult best;
    for (int iter = 0; iter < iterations; ++iter) {
        std::vector<Box> candidate = instance.boxes;
        std::stable_sort(candidate.begin(), candidate.end(), [&](const Box& a, const Box& b) {
            const int jitter_a = static_cast<int>(rng() % 5);
            const int jitter_b = static_cast<int>(rng() % 5);
            const long long score_a = volume(a.size) + static_cast<long long>(jitter_a);
            const long long score_b = volume(b.size) + static_cast<long long>(jitter_b);
            return score_a > score_b;
        });
        auto res = try_order(instance, candidate, GreedyConfig{SplitPolicy::Maximal});
        if (iter == 0 || better(res, best)) {
            best = res;
        }
    }
    return best;
}

PackingResult meta_sa(const Instance& instance, unsigned int seed, int iterations) {
    std::mt19937 rng(seed ? seed : std::random_device{}());
    std::vector<Box> order = instance.boxes;
    PackingResult current = try_order(instance, order, GreedyConfig{SplitPolicy::Maximal});
    PackingResult best = current;

    double temperature = 1.0;
    const double cooling = 0.99;

    for (int iter = 0; iter < iterations; ++iter) {
        if (order.size() >= 2) {
            std::uniform_int_distribution<std::size_t> dist(0, order.size() - 1);
            auto i1 = dist(rng);
            auto i2 = dist(rng);
            std::swap(order[i1], order[i2]);
        }
        auto candidate = try_order(instance, order, GreedyConfig{SplitPolicy::Maximal});

        auto score = [&](const PackingResult& r) {
            return static_cast<double>(r.objective.bins_used) * 1e6 +
                   static_cast<double>(r.objective.leftover_volume);
        };
        double delta = score(candidate) - score(current);
        if (delta < 0 || std::exp(-delta / (temperature + 1e-9)) > std::generate_canonical<double, 10>(rng)) {
            current = candidate;
            if (better(current, best)) {
                best = current;
            }
        }
        temperature *= cooling;
    }
    return best;
}

}  // namespace bp::algorithms
