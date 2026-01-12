#include "bp/instance_io.hpp"

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

namespace bp {
namespace {

nlohmann::json to_json(const Instance& instance) {
    nlohmann::json j;
    j["bins"] = nlohmann::json::array();
    for (const auto& b : instance.bins) {
        j["bins"].push_back({{"id", b.id}, {"size", {{"w", b.size.w}, {"l", b.size.l}, {"h", b.size.h}}}});
    }
    j["boxes"] = nlohmann::json::array();
    for (const auto& b : instance.boxes) {
        j["boxes"].push_back({{"id", b.id},
                               {"size", {{"w", b.size.w}, {"l", b.size.l}, {"h", b.size.h}}},
                               {"this_side_up", b.this_side_up},
                               {"no_stack", b.no_stack}});
    }
    return j;
}

Instance from_json_instance(const nlohmann::json& j) {
    Instance inst;
    if (!j.contains("bins") || !j.contains("boxes")) {
        throw std::runtime_error("Invalid instance JSON: missing bins or boxes");
    }
    for (const auto& bin_j : j.at("bins")) {
        Bin b;
        b.id = bin_j.at("id").get<std::string>();
        const auto& s = bin_j.at("size");
        b.size = Vec3{s.at("w").get<int>(), s.at("l").get<int>(), s.at("h").get<int>()};
        inst.bins.push_back(b);
    }
    for (const auto& box_j : j.at("boxes")) {
        Box b;
        b.id = box_j.at("id").get<std::string>();
        const auto& s = box_j.at("size");
        b.size = Vec3{s.at("w").get<int>(), s.at("l").get<int>(), s.at("h").get<int>()};
        b.this_side_up = box_j.value("this_side_up", false);
        b.no_stack = box_j.value("no_stack", false);
        inst.boxes.push_back(b);
    }
    return inst;
}

nlohmann::json to_json(const PackingResult& result) {
    nlohmann::json j;
    j["feasible"] = result.feasible;
    j["objective"] = {{"bins_used", result.objective.bins_used},
                      {"leftover_volume", result.objective.leftover_volume}};
    j["stats"] = {{"boxes_total", result.stats.boxes_total},
                   {"boxes_placed", result.stats.boxes_placed},
                   {"volume_boxes", result.stats.volume_boxes},
                   {"volume_packed", result.stats.volume_packed},
                   {"volume_bins_used", result.stats.volume_bins_used},
                   {"fill_ratio", result.stats.fill_ratio}};
    j["metadata"] = {{"algorithm", result.metadata.algorithm},
                      {"seed", result.metadata.seed},
                      {"iterations", result.metadata.iterations},
                      {"instance_file", result.metadata.instance_file},
                      {"timestamp", result.metadata.timestamp}};
    j["placements"] = nlohmann::json::array();
    for (const auto& p : result.placements) {
        j["placements"].push_back({{"box_id", p.box_id},
                                    {"bin_id", p.bin_id},
                                    {"position", {{"x", p.position.w}, {"y", p.position.l}, {"z", p.position.h}}},
                                    {"orientation", {{"w", p.orientation.w}, {"l", p.orientation.l}, {"h", p.orientation.h}}}});
    }
    return j;
}

PackingResult from_json_result(const nlohmann::json& j) {
    PackingResult r;
    r.feasible = j.at("feasible").get<bool>();
    const auto& obj = j.at("objective");
    r.objective.bins_used = obj.at("bins_used").get<int>();
    r.objective.leftover_volume = obj.at("leftover_volume").get<long long>();
    if (j.contains("stats")) {
        const auto& s = j.at("stats");
        r.stats.boxes_total = s.value("boxes_total", 0);
        r.stats.boxes_placed = s.value("boxes_placed", 0);
        r.stats.volume_boxes = s.value("volume_boxes", 0LL);
        r.stats.volume_packed = s.value("volume_packed", 0LL);
        r.stats.volume_bins_used = s.value("volume_bins_used", 0LL);
        r.stats.fill_ratio = s.value("fill_ratio", 0.0);
    }
    if (j.contains("metadata")) {
        const auto& m = j.at("metadata");
        r.metadata.algorithm = m.value("algorithm", "");
        r.metadata.seed = m.value("seed", 0u);
        r.metadata.iterations = m.value("iterations", 0);
        r.metadata.instance_file = m.value("instance_file", "");
        r.metadata.timestamp = m.value("timestamp", "");
    }
    if (j.contains("placements")) {
        for (const auto& p : j.at("placements")) {
            Placement pl;
            pl.box_id = p.at("box_id").get<std::string>();
            pl.bin_id = p.at("bin_id").get<std::string>();
            const auto& pos = p.at("position");
            pl.position = Vec3{pos.at("x").get<int>(), pos.at("y").get<int>(), pos.at("z").get<int>()};
            const auto& o = p.at("orientation");
            pl.orientation = Vec3{o.at("w").get<int>(), o.at("l").get<int>(), o.at("h").get<int>()};
            r.placements.push_back(pl);
        }
    }
    return r;
}

}  // namespace

Instance load_instance(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Failed to open instance file: " + path);
    }
    nlohmann::json j;
    in >> j;
    return from_json_instance(j);
}

PackingResult load_result(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Failed to open result file: " + path);
    }
    nlohmann::json j;
    in >> j;
    return from_json_result(j);
}

void save_instance(const Instance& instance, const std::string& path) {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("Failed to open output file: " + path);
    }
    out << to_json(instance).dump(2);
}

void save_result(const PackingResult& result, const std::string& path) {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("Failed to open output file: " + path);
    }
    out << to_json(result).dump(2);
}

}  // namespace bp
