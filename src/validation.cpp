#include "bp/validation.hpp"

#include <algorithm>
#include <array>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace bp {
namespace {

bool positive(const Vec3 &value) { return value.w > 0 && value.l > 0 && value.h > 0; }

bool same_multiset(Vec3 a, Vec3 b) {
    std::array<int, 3> lhs{a.w, a.l, a.h};
    std::array<int, 3> rhs{b.w, b.l, b.h};
    std::sort(lhs.begin(), lhs.end());
    std::sort(rhs.begin(), rhs.end());
    return lhs == rhs;
}

bool inside(const Placement &placement, const Bin &bin) {
    return placement.position.w >= 0 && placement.position.l >= 0 && placement.position.h >= 0 &&
           placement.orientation.w > 0 && placement.orientation.l > 0 && placement.orientation.h > 0 &&
           placement.position.w + placement.orientation.w <= bin.size.w &&
           placement.position.l + placement.orientation.l <= bin.size.l &&
           placement.position.h + placement.orientation.h <= bin.size.h;
}

bool footprint_overlaps(const Placement &base, const Placement &candidate) {
    const bool x_overlap = base.position.w < candidate.position.w + candidate.orientation.w &&
                           candidate.position.w < base.position.w + base.orientation.w;
    const bool y_overlap = base.position.l < candidate.position.l + candidate.orientation.l &&
                           candidate.position.l < base.position.l + base.orientation.l;
    return x_overlap && y_overlap;
}

bool violates_no_stack(const Placement &base, const Box &base_box, const Placement &candidate) {
    if (!base_box.no_stack || base.bin_id != candidate.bin_id || !footprint_overlaps(base, candidate)) {
        return false;
    }
    return candidate.position.h >= base.position.h + base.orientation.h;
}

void add_error(ValidationReport &report, std::string error) {
    report.valid = false;
    report.errors.push_back(std::move(error));
}

} // namespace

ValidationReport validate_instance(const Instance &instance) {
    ValidationReport report;
    std::unordered_set<std::string> bin_ids;
    std::unordered_set<std::string> box_ids;

    for (const auto &bin : instance.bins) {
        if (bin.id.empty()) {
            add_error(report, "bin id must not be empty");
        }
        if (!bin_ids.insert(bin.id).second) {
            add_error(report, "duplicate bin id: " + bin.id);
        }
        if (!positive(bin.size)) {
            add_error(report, "bin dimensions must be positive: " + bin.id);
        }
    }

    for (const auto &box : instance.boxes) {
        if (box.id.empty()) {
            add_error(report, "box id must not be empty");
        }
        if (!box_ids.insert(box.id).second) {
            add_error(report, "duplicate box id: " + box.id);
        }
        if (!positive(box.size)) {
            add_error(report, "box dimensions must be positive: " + box.id);
        }
    }

    return report;
}

std::vector<std::string> unplaced_box_ids(const Instance &instance, const PackingResult &result) {
    std::unordered_set<std::string> placed;
    for (const auto &placement : result.placements) {
        placed.insert(placement.box_id);
    }

    std::vector<std::string> unplaced;
    for (const auto &box : instance.boxes) {
        if (!placed.contains(box.id)) {
            unplaced.push_back(box.id);
        }
    }
    return unplaced;
}

bool can_place_with(const Placement &candidate, const Box &box, const std::vector<Placement> &existing,
                    const Instance &instance) {
    if (std::any_of(existing.begin(), existing.end(),
                    [&](const Placement &placed) { return overlaps(candidate, placed); })) {
        return false;
    }

    std::unordered_map<std::string, const Box *> boxes_by_id;
    for (const auto &existing_box : instance.boxes) {
        boxes_by_id.emplace(existing_box.id, &existing_box);
    }

    for (const auto &placed : existing) {
        const auto base_it = boxes_by_id.find(placed.box_id);
        if (base_it != boxes_by_id.end() && violates_no_stack(placed, *base_it->second, candidate)) {
            return false;
        }
        if (box.no_stack && violates_no_stack(candidate, box, placed)) {
            return false;
        }
    }

    return true;
}

ValidationReport validate_result(const Instance &instance, const PackingResult &result) {
    ValidationReport report = validate_instance(instance);
    if (!report.valid) {
        return report;
    }

    std::unordered_map<std::string, const Bin *> bins_by_id;
    for (const auto &bin : instance.bins) {
        bins_by_id.emplace(bin.id, &bin);
    }

    std::unordered_map<std::string, const Box *> boxes_by_id;
    for (const auto &box : instance.boxes) {
        boxes_by_id.emplace(box.id, &box);
    }

    std::unordered_set<std::string> placed_box_ids;
    for (const auto &placement : result.placements) {
        const auto box_it = boxes_by_id.find(placement.box_id);
        if (box_it == boxes_by_id.end()) {
            add_error(report, "placement references unknown box: " + placement.box_id);
            continue;
        }
        const auto bin_it = bins_by_id.find(placement.bin_id);
        if (bin_it == bins_by_id.end()) {
            add_error(report, "placement references unknown bin: " + placement.bin_id);
            continue;
        }
        if (!placed_box_ids.insert(placement.box_id).second) {
            add_error(report, "box placed more than once: " + placement.box_id);
        }
        if (!inside(placement, *bin_it->second)) {
            add_error(report, "placement is outside bin bounds: " + placement.box_id);
        }
        if (!same_multiset(box_it->second->size, placement.orientation)) {
            add_error(report, "placement uses an illegal orientation: " + placement.box_id);
        }
        if (box_it->second->this_side_up && placement.orientation.h != box_it->second->size.h) {
            add_error(report, "this_side_up box changed height axis: " + placement.box_id);
        }
    }

    for (std::size_t left = 0; left < result.placements.size(); ++left) {
        for (std::size_t right = left + 1; right < result.placements.size(); ++right) {
            const auto &a = result.placements[left];
            const auto &b = result.placements[right];
            if (overlaps(a, b)) {
                add_error(report, "placements overlap: " + a.box_id + " and " + b.box_id);
            }
        }
    }

    for (const auto &base : result.placements) {
        const auto base_it = boxes_by_id.find(base.box_id);
        if (base_it == boxes_by_id.end()) {
            continue;
        }
        for (const auto &candidate : result.placements) {
            if (base.box_id != candidate.box_id && violates_no_stack(base, *base_it->second, candidate)) {
                add_error(report, "no_stack box has another box above its footprint: " + base.box_id);
            }
        }
    }

    const auto unplaced = unplaced_box_ids(instance, result);
    if (result.status == PackingStatus::Feasible || result.feasible) {
        for (const auto &box_id : unplaced) {
            add_error(report, "feasible result is missing box: " + box_id);
        }
    }

    return report;
}

} // namespace bp
