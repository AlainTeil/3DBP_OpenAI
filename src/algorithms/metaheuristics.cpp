#include "common.hpp"

#include <algorithm>
#include <cmath>
#include <random>
#include <unordered_map>
#include <unordered_set>

#include "bp/algorithms/algorithms.hpp"

namespace bp::algorithms {
namespace {

bool better(const PackingResult &a, const PackingResult &b) {
    const auto rank = [](PackingStatus status) {
        switch (status) {
        case PackingStatus::Feasible:
            return 3;
        case PackingStatus::Partial:
            return 2;
        case PackingStatus::Infeasible:
            return 1;
        case PackingStatus::Invalid:
            return 0;
        }
        return 0;
    };

    if (rank(a.status) != rank(b.status)) {
        return rank(a.status) > rank(b.status);
    }
    if (a.placements.size() != b.placements.size()) {
        return a.placements.size() > b.placements.size();
    }
    if (a.objective.bins_used != b.objective.bins_used) {
        return a.objective.bins_used < b.objective.bins_used;
    }
    return a.objective.leftover_volume < b.objective.leftover_volume;
}

PackingResult try_order(const Instance &instance, const std::vector<Box> &order, GreedyConfig cfg) {
    return greedy_pack(instance, order, cfg);
}

std::vector<Box> order_crossover(const std::vector<Box> &first, const std::vector<Box> &second, std::mt19937 &rng) {
    if (first.size() != second.size() || first.size() < 2) {
        return first;
    }

    std::uniform_int_distribution<std::size_t> dist(0, first.size() - 1);
    std::size_t left = dist(rng);
    std::size_t right = dist(rng);
    if (left > right) {
        std::swap(left, right);
    }

    std::vector<Box> child(first.size());
    std::vector<bool> filled(first.size(), false);
    std::unordered_set<std::string> used;
    for (std::size_t index = left; index <= right; ++index) {
        child[index] = first[index];
        filled[index] = true;
        used.insert(first[index].id);
    }

    std::size_t write = (right + 1) % first.size();
    for (std::size_t offset = 0; offset < second.size(); ++offset) {
        const auto &box = second[(right + 1 + offset) % second.size()];
        if (used.contains(box.id)) {
            continue;
        }
        while (filled[write]) {
            write = (write + 1) % first.size();
        }
        child[write] = box;
        filled[write] = true;
        used.insert(box.id);
    }

    return child;
}

} // namespace

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
PackingResult meta_ga(const Instance &instance, unsigned int seed, int iterations) {
    std::mt19937 rng(seed ? seed : std::random_device{}());
    const std::size_t pop_size = 12;
    const int iteration_count = std::max(1, iterations);
    std::vector<std::vector<Box>> population;
    population.reserve(pop_size);
    for (std::size_t i = 0; i < pop_size; ++i) {
        auto order = instance.boxes;
        std::shuffle(order.begin(), order.end(), rng);
        population.push_back(order);
    }

    PackingResult best;
    for (int iter = 0; iter < iteration_count; ++iter) {
        for (const auto &order : population) {
            auto res = try_order(instance, order, GreedyConfig{SplitPolicy::Maximal});
            if (iter == 0 || better(res, best)) {
                best = res;
            }
        }

        for (std::size_t i = 0; i + 1 < pop_size; i += 2) {
            const auto &first = population[i];
            const auto &second = population[i + 1];
            if (first.empty()) {
                continue;
            }
            auto child_a = order_crossover(first, second, rng);
            auto child_b = order_crossover(second, first, rng);
            population[i] = std::move(child_a);
            population[i + 1] = std::move(child_b);
        }

        for (auto &order : population) {
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

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
PackingResult meta_grasp(const Instance &instance, unsigned int seed, int iterations) {
    std::mt19937 rng(seed ? seed : std::random_device{}());
    PackingResult best;
    const int iteration_count = std::max(1, iterations);
    for (int iter = 0; iter < iteration_count; ++iter) {
        std::vector<Box> candidate = instance.boxes;
        std::unordered_map<std::string, long long> scores;
        for (const auto &box : candidate) {
            scores.emplace(box.id, volume(box.size) + static_cast<long long>(rng() % 5));
        }
        std::stable_sort(candidate.begin(), candidate.end(), [&](const Box &first, const Box &second) {
            return scores.at(first.id) > scores.at(second.id);
        });
        auto res = try_order(instance, candidate, GreedyConfig{SplitPolicy::Maximal});
        if (iter == 0 || better(res, best)) {
            best = res;
        }
    }
    return best;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
PackingResult meta_sa(const Instance &instance, unsigned int seed, int iterations) {
    std::mt19937 rng(seed ? seed : std::random_device{}());
    std::vector<Box> order = instance.boxes;
    PackingResult current = try_order(instance, order, GreedyConfig{SplitPolicy::Maximal});
    PackingResult best = current;

    double temperature = 1.0;
    const double cooling = 0.99;

    const int iteration_count = std::max(1, iterations);
    for (int iter = 0; iter < iteration_count; ++iter) {
        auto candidate_order = order;
        if (order.size() >= 2) {
            std::uniform_int_distribution<std::size_t> dist(0, order.size() - 1);
            auto i1 = dist(rng);
            auto i2 = dist(rng);
            std::swap(candidate_order[i1], candidate_order[i2]);
        }
        auto candidate = try_order(instance, candidate_order, GreedyConfig{SplitPolicy::Maximal});

        auto score = [&](const PackingResult &r) {
            return static_cast<double>(r.objective.bins_used) * 1e6 + static_cast<double>(r.objective.leftover_volume);
        };
        double delta = score(candidate) - score(current);
        if (delta < 0 || std::exp(-delta / (temperature + 1e-9)) > std::generate_canonical<double, 10>(rng)) {
            order = std::move(candidate_order);
            current = candidate;
            if (better(current, best)) {
                best = current;
            }
        }
        temperature *= cooling;
    }
    return best;
}

} // namespace bp::algorithms
