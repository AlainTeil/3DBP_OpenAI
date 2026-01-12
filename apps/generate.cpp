#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "bp/instance_io.hpp"

namespace {

struct Args {
    int boxes{0};
    int bins{1};
    int bin_w{0};
    int bin_l{0};
    int bin_h{0};
    double rate_up{0.0};
    double rate_nostack{0.0};
    unsigned int seed{0};
    std::string output{"instance.json"};
};

bool parse_dims(const std::string& s, int& w, int& l, int& h) {
    auto first_x = s.find('x');
    auto second_x = s.find('x', first_x + 1);
    if (first_x == std::string::npos || second_x == std::string::npos) {
        return false;
    }
    try {
        w = std::stoi(s.substr(0, first_x));
        l = std::stoi(s.substr(first_x + 1, second_x - first_x - 1));
        h = std::stoi(s.substr(second_x + 1));
        return true;
    } catch (...) {
        return false;
    }
}

void usage() {
    std::cerr << "Usage: generate --boxes N --bins M --bin-size WxLxH [--up p] [--nostack p] [--seed N] [--output file]\n";
}

Args parse(int argc, char** argv) {
    Args a;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--boxes" && i + 1 < argc) {
            a.boxes = std::stoi(argv[++i]);
        } else if (arg == "--bins" && i + 1 < argc) {
            a.bins = std::stoi(argv[++i]);
        } else if (arg == "--bin-size" && i + 1 < argc) {
            if (!parse_dims(argv[++i], a.bin_w, a.bin_l, a.bin_h)) {
                throw std::runtime_error("Invalid bin size format. Expected WxLxH");
            }
        } else if (arg == "--up" && i + 1 < argc) {
            a.rate_up = std::stod(argv[++i]);
        } else if (arg == "--nostack" && i + 1 < argc) {
            a.rate_nostack = std::stod(argv[++i]);
        } else if (arg == "--seed" && i + 1 < argc) {
            a.seed = static_cast<unsigned int>(std::stoul(argv[++i]));
        } else if (arg == "--output" && i + 1 < argc) {
            a.output = argv[++i];
        } else if (arg == "--help") {
            usage();
            std::exit(0);
        }
    }
    if (a.boxes <= 0 || a.bins <= 0 || a.bin_w <= 0 || a.bin_l <= 0 || a.bin_h <= 0) {
        usage();
        throw std::runtime_error("Boxes, bins, and bin size must be provided and positive");
    }
    return a;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const auto args = parse(argc, argv);
        std::mt19937 rng(args.seed ? args.seed : std::random_device{}());
        std::uniform_real_distribution<double> prob(0.0, 1.0);

        bp::Instance inst;
        for (int b = 0; b < args.bins; ++b) {
            inst.bins.push_back(bp::Bin{.id = "bin-" + std::to_string(b),
                                       .size = bp::Vec3{args.bin_w, args.bin_l, args.bin_h}});
        }

        std::uniform_int_distribution<int> wdist(args.bin_w / 6, args.bin_w / 2);
        std::uniform_int_distribution<int> ldist(args.bin_l / 6, args.bin_l / 2);
        std::uniform_int_distribution<int> hdist(args.bin_h / 6, args.bin_h / 2);

        for (int i = 0; i < args.boxes; ++i) {
            bp::Box box;
            box.id = "box-" + std::to_string(i);
            box.size = bp::Vec3{wdist(rng), ldist(rng), hdist(rng)};
            box.this_side_up = prob(rng) < args.rate_up;
            box.no_stack = prob(rng) < args.rate_nostack;
            inst.boxes.push_back(box);
        }

        bp::save_instance(inst, args.output);
        std::cout << "Wrote instance to " << args.output << "\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
}
