#include <algorithm>
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include <fmt/core.h>

#include "bp/instance_io.hpp"
#include "bp/packer.hpp"

namespace {

struct AlgorithmSpec {
    const char *name;
    bp::AlgorithmId id;
    const char *description;
};

constexpr std::array<AlgorithmSpec, 9> algorithms{{
    {"ffd", bp::AlgorithmId::FFD, "offline deterministic greedy first-fit, descending volume"},
    {"nfdh", bp::AlgorithmId::NFDH, "offline deterministic greedy first-fit, descending height then volume"},
    {"layer", bp::AlgorithmId::Layered, "offline deterministic greedy first-fit, descending height then base area"},
    {"guillotine", bp::AlgorithmId::Guillotine, "offline deterministic greedy packer with guillotine residual spaces"},
    {"maxspace", bp::AlgorithmId::MaximalSpace, "offline deterministic greedy first-fit, largest side then volume"},
    {"meta-ga", bp::AlgorithmId::MetaGA, "offline stochastic genetic search over greedy orderings"},
    {"meta-grasp", bp::AlgorithmId::MetaGRASP, "offline stochastic randomized greedy ordering search"},
    {"meta-sa", bp::AlgorithmId::MetaSA, "offline stochastic simulated annealing over greedy orderings"},
    {"online-ffd", bp::AlgorithmId::OnlineFFD, "online deterministic greedy first-fit in input order"},
}};

bp::AlgorithmId parse_algo(const std::string &name) {
    const auto it = std::find_if(algorithms.begin(), algorithms.end(),
                                 [&name](const AlgorithmSpec &algorithm) { return name == algorithm.name; });
    if (it != algorithms.end()) {
        return it->id;
    }
    throw std::runtime_error("Unknown algorithm: " + name);
}

void list_algorithms() {
    fmt::print("Available algorithms:\n");
    for (const auto &algorithm : algorithms) {
        fmt::print("  {:<11} {}\n", algorithm.name, algorithm.description);
    }
}

void usage() {
    std::cerr << "Usage: pack --input file.json --algo <name> [--seed N] [--iterations N] [--output out.json]\n"
              << "             [--verbose 0|1|2] [--csv placements.csv]\n"
              << "       pack --list-algorithms\n";
}

} // namespace

int main(int argc, char **argv) {
    std::string input;
    std::string output;
    std::string algo_name;
    unsigned int seed = 0;
    int iterations = 200;
    int verbose = 0;
    std::string csv_path;
    bool should_list_algorithms = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--input" && i + 1 < argc) {
            input = argv[++i];
        } else if (arg == "--output" && i + 1 < argc) {
            output = argv[++i];
        } else if (arg == "--algo" && i + 1 < argc) {
            algo_name = argv[++i];
        } else if (arg == "--seed" && i + 1 < argc) {
            seed = static_cast<unsigned int>(std::stoul(argv[++i]));
        } else if (arg == "--iterations" && i + 1 < argc) {
            iterations = std::stoi(argv[++i]);
        } else if (arg == "--verbose" && i + 1 < argc) {
            verbose = std::stoi(argv[++i]);
        } else if (arg == "--csv" && i + 1 < argc) {
            csv_path = argv[++i];
        } else if (arg == "--list-algorithms") {
            should_list_algorithms = true;
        } else if (arg == "--help") {
            usage();
            return 0;
        }
    }

    if (should_list_algorithms) {
        list_algorithms();
        return 0;
    }

    if (input.empty() || algo_name.empty()) {
        usage();
        return 1;
    }

    try {
        auto instance = bp::load_instance(input);
        bp::PackOptions opts;
        opts.algorithm = parse_algo(algo_name);
        opts.seed = seed;
        opts.iterations = iterations;
        auto result = bp::pack(instance, opts);
        result.metadata.instance_file = input;

        if (!output.empty()) {
            bp::save_result(result, output);
        } else {
            fmt::print("{}\n", bp::to_string(result.status));
            fmt::print("bins_used: {}\n", result.objective.bins_used);
            fmt::print("leftover_volume: {}\n", result.objective.leftover_volume);
            if (!result.unplaced_box_ids.empty()) {
                fmt::print("unplaced_boxes: {}\n", result.unplaced_box_ids.size());
            }
        }

        if (!csv_path.empty()) {
            std::ofstream csv(csv_path);
            if (!csv) {
                throw std::runtime_error("Failed to open CSV output file: " + csv_path);
            }
            csv << "bin_id,box_id,x,y,z,w,l,h\n";
            for (const auto &p : result.placements) {
                csv << p.bin_id << ',' << p.box_id << ',' << p.position.w << ',' << p.position.l << ',' << p.position.h
                    << ',' << p.orientation.w << ',' << p.orientation.l << ',' << p.orientation.h << "\n";
            }
        }

        if (verbose > 0) {
            fmt::print("boxes: placed {} / {}, unplaced {}\n", result.stats.boxes_placed, result.stats.boxes_total,
                       result.stats.boxes_unplaced);
            fmt::print("volume: packed {} / {} (fill {:.2f}%)\n", result.stats.volume_packed,
                       result.stats.volume_bins_used, result.stats.fill_ratio * 100.0);
            fmt::print("seed: {}  iterations: {}  algo: {}\n", result.metadata.seed, result.metadata.iterations,
                       result.metadata.algorithm);
            if (!result.metadata.timestamp.empty()) {
                fmt::print("timestamp: {}\n", result.metadata.timestamp);
            }
            for (const auto &error : result.validation_errors) {
                fmt::print("validation: {}\n", error);
            }
        }

        if (verbose > 1) {
            std::map<std::string, std::vector<bp::Placement>> by_bin;
            for (const auto &p : result.placements) {
                by_bin[p.bin_id].push_back(p);
            }
            for (auto &[bin_id, vec] : by_bin) {
                std::sort(vec.begin(), vec.end(), [](const bp::Placement &a, const bp::Placement &b) {
                    if (a.position.h != b.position.h) {
                        return a.position.h < b.position.h;
                    }
                    if (a.position.l != b.position.l) {
                        return a.position.l < b.position.l;
                    }
                    return a.position.w < b.position.w;
                });
                fmt::print("Bin {}:\n", bin_id);
                for (const auto &p : vec) {
                    fmt::print("  {} at ({},{},{}) size ({},{},{})\n", p.box_id, p.position.w, p.position.l,
                               p.position.h, p.orientation.w, p.orientation.l, p.orientation.h);
                }
            }
        }
        return result.status == bp::PackingStatus::Feasible ? 0 : 2;
    } catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
}
