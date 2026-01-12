#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include <fmt/core.h>

#include "bp/instance_io.hpp"
#include "bp/packer.hpp"

namespace {

bp::AlgorithmId parse_algo(const std::string& name) {
    static const std::unordered_map<std::string, bp::AlgorithmId> map = {
        {"ffd", bp::AlgorithmId::FFD},
        {"nfdh", bp::AlgorithmId::NFDH},
        {"layer", bp::AlgorithmId::Layered},
        {"guillotine", bp::AlgorithmId::Guillotine},
        {"maxspace", bp::AlgorithmId::MaximalSpace},
        {"meta-ga", bp::AlgorithmId::MetaGA},
        {"meta-grasp", bp::AlgorithmId::MetaGRASP},
        {"meta-sa", bp::AlgorithmId::MetaSA},
        {"online-ffd", bp::AlgorithmId::OnlineFFD},
    };
    auto it = map.find(name);
    if (it == map.end()) {
        throw std::runtime_error("Unknown algorithm: " + name);
    }
    return it->second;
}

void usage() {
    std::cerr << "Usage: pack --input file.json --algo <name> [--seed N] [--iterations N] [--output out.json]\n"
              << "             [--verbose 0|1|2] [--csv placements.csv]\n";
}

}  // namespace

int main(int argc, char** argv) {
    std::string input;
    std::string output;
    std::string algo_name;
    unsigned int seed = 0;
    int iterations = 200;
    int verbose = 0;
    std::string csv_path;

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
        } else if (arg == "--help") {
            usage();
            return 0;
        }
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
            fmt::print("{}\n", result.feasible ? "feasible" : "infeasible");
            fmt::print("bins_used: {}\n", result.objective.bins_used);
            fmt::print("leftover_volume: {}\n", result.objective.leftover_volume);
        }

        if (!csv_path.empty()) {
            std::ofstream csv(csv_path);
            csv << "bin_id,box_id,x,y,z,w,l,h\n";
            for (const auto& p : result.placements) {
                csv << p.bin_id << ',' << p.box_id << ',' << p.position.w << ',' << p.position.l << ',' << p.position.h << ','
                    << p.orientation.w << ',' << p.orientation.l << ',' << p.orientation.h << "\n";
            }
        }

        if (verbose > 0) {
            fmt::print("boxes: placed {} / {}\n", result.stats.boxes_placed, result.stats.boxes_total);
            fmt::print("volume: packed {} / {} (fill {:.2f}%)\n", result.stats.volume_packed, result.stats.volume_bins_used,
                       result.stats.fill_ratio * 100.0);
            fmt::print("seed: {}  iterations: {}  algo: {}\n", result.metadata.seed, result.metadata.iterations,
                       result.metadata.algorithm);
            if (!result.metadata.timestamp.empty()) {
                fmt::print("timestamp: {}\n", result.metadata.timestamp);
            }
        }

        if (verbose > 1) {
            std::map<std::string, std::vector<bp::Placement>> by_bin;
            for (const auto& p : result.placements) {
                by_bin[p.bin_id].push_back(p);
            }
            for (auto& [bin_id, vec] : by_bin) {
                std::sort(vec.begin(), vec.end(), [](const bp::Placement& a, const bp::Placement& b) {
                    if (a.position.h != b.position.h) return a.position.h < b.position.h;
                    if (a.position.l != b.position.l) return a.position.l < b.position.l;
                    return a.position.w < b.position.w;
                });
                fmt::print("Bin {}:\n", bin_id);
                for (const auto& p : vec) {
                    fmt::print("  {} at ({},{},{}) size ({},{},{})\n", p.box_id, p.position.w, p.position.l, p.position.h,
                               p.orientation.w, p.orientation.l, p.orientation.h);
                }
            }
        }
        return result.feasible ? 0 : 2;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
}
