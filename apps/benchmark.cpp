#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <fmt/core.h>

#include "bp/instance_io.hpp"
#include "bp/packer.hpp"
#include "bp/validation.hpp"

namespace {

#ifndef THREEDBP_SOURCE_DIR
#define THREEDBP_SOURCE_DIR ""
#endif

struct AlgorithmSpec {
    bp::AlgorithmId id;
    const char *name;
};

struct Args {
    std::filesystem::path corpus_dir{"benchmarks/corpus"};
    std::vector<std::filesystem::path> inputs;
    std::filesystem::path output;
    unsigned int seed{0};
    int iterations{50};
};

constexpr std::array<AlgorithmSpec, 9> algorithms{{
    {bp::AlgorithmId::FFD, "ffd"},
    {bp::AlgorithmId::NFDH, "nfdh"},
    {bp::AlgorithmId::Layered, "layer"},
    {bp::AlgorithmId::Guillotine, "guillotine"},
    {bp::AlgorithmId::MaximalSpace, "maxspace"},
    {bp::AlgorithmId::MetaGA, "meta-ga"},
    {bp::AlgorithmId::MetaGRASP, "meta-grasp"},
    {bp::AlgorithmId::MetaSA, "meta-sa"},
    {bp::AlgorithmId::OnlineFFD, "online-ffd"},
}};

void usage() {
    std::cerr << "Usage: benchmark [--corpus dir] [--input file.json ...] [--seed N] [--iterations N] "
                 "[--output report.csv]\n";
}

Args parse_args(int argc, char **argv) {
    Args args;
    for (int index = 1; index < argc; ++index) {
        const std::string arg = argv[index];
        if (arg == "--corpus" && index + 1 < argc) {
            args.corpus_dir = argv[++index];
        } else if (arg == "--input" && index + 1 < argc) {
            args.inputs.emplace_back(argv[++index]);
        } else if (arg == "--seed" && index + 1 < argc) {
            args.seed = static_cast<unsigned int>(std::stoul(argv[++index]));
        } else if (arg == "--iterations" && index + 1 < argc) {
            args.iterations = std::stoi(argv[++index]);
        } else if (arg == "--output" && index + 1 < argc) {
            args.output = argv[++index];
        } else if (arg == "--help") {
            usage();
            std::exit(0);
        } else {
            usage();
            throw std::runtime_error("Unknown or incomplete argument: " + arg);
        }
    }
    if (args.iterations < 0) {
        throw std::runtime_error("Iterations must be non-negative");
    }
    return args;
}

std::vector<std::filesystem::path> collect_inputs(const Args &args) {
    if (!args.inputs.empty()) {
        return args.inputs;
    }

    auto corpus_dir = args.corpus_dir;
    if (!std::filesystem::is_directory(corpus_dir) && corpus_dir.is_relative()) {
        const auto source_relative = std::filesystem::path(THREEDBP_SOURCE_DIR) / corpus_dir;
        if (!std::filesystem::path(THREEDBP_SOURCE_DIR).empty() && std::filesystem::is_directory(source_relative)) {
            corpus_dir = source_relative;
        }
    }

    if (!std::filesystem::is_directory(corpus_dir)) {
        throw std::runtime_error("Benchmark corpus directory not found: " + args.corpus_dir.string());
    }

    std::vector<std::filesystem::path> inputs;
    for (const auto &entry : std::filesystem::directory_iterator(corpus_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            inputs.push_back(entry.path());
        }
    }
    std::sort(inputs.begin(), inputs.end());
    if (inputs.empty()) {
        throw std::runtime_error("Benchmark corpus directory contains no JSON instances: " + corpus_dir.string());
    }
    return inputs;
}

std::ostream &open_output(const std::filesystem::path &path, std::ofstream &file) {
    if (path.empty()) {
        return std::cout;
    }
    file.open(path);
    if (!file) {
        throw std::runtime_error("Failed to open benchmark output file: " + path.string());
    }
    return file;
}

} // namespace

int main(int argc, char **argv) {
    try {
        const auto args = parse_args(argc, argv);
        const auto inputs = collect_inputs(args);

        std::ofstream output_file;
        auto &out = open_output(args.output, output_file);
        out << "instance,algorithm,status,elapsed_ms,bins_used,boxes_total,boxes_placed,boxes_unplaced,fill_ratio,"
               "valid\n";

        for (const auto &input : inputs) {
            const auto instance = bp::load_instance(input.string());
            for (const auto &algorithm : algorithms) {
                bp::PackOptions options;
                options.algorithm = algorithm.id;
                options.seed = args.seed;
                options.iterations = args.iterations;

                const auto start = std::chrono::steady_clock::now();
                const auto result = bp::pack(instance, options);
                const auto stop = std::chrono::steady_clock::now();
                const std::chrono::duration<double, std::milli> elapsed = stop - start;
                const auto validation = bp::validate_result(instance, result);

                out << input.filename().string() << ',' << algorithm.name << ',' << bp::to_string(result.status) << ',';
                out << fmt::format("{:.3f}", elapsed.count()) << ',' << result.objective.bins_used << ',';
                out << result.stats.boxes_total << ',' << result.stats.boxes_placed << ','
                    << result.stats.boxes_unplaced << ',';
                out << fmt::format("{:.6f}", result.stats.fill_ratio) << ',' << (validation.valid ? "true" : "false")
                    << '\n';
            }
        }

        return 0;
    } catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }
}
