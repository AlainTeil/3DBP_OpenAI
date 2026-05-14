#include "common.hpp"

#include <algorithm>
#include <iterator>
#include <numeric>
#include <unordered_set>

#include "bp/validation.hpp"

namespace bp::algorithms {
namespace {

Space make_root_space(const Bin &bin) { return Space{Vec3{0, 0, 0}, bin.size, true}; }

void prune(std::vector<Space> &spaces) {
    spaces.erase(std::remove_if(spaces.begin(), spaces.end(),
                                [](const Space &s) { return s.size.w <= 0 || s.size.l <= 0 || s.size.h <= 0; }),
                 spaces.end());
    for (std::size_t i = 0; i < spaces.size(); ++i) {
        for (std::size_t j = 0; j < spaces.size();) {
            if (i == j) {
                ++j;
                continue;
            }
            const auto &a = spaces[i];
            const auto &b = spaces[j];
            const bool contained = a.origin.w <= b.origin.w && a.origin.l <= b.origin.l && a.origin.h <= b.origin.h &&
                                   a.origin.w + a.size.w >= b.origin.w + b.size.w &&
                                   a.origin.l + a.size.l >= b.origin.l + b.size.l &&
                                   a.origin.h + a.size.h >= b.origin.h + b.size.h;
            if (contained) {
                spaces.erase(spaces.begin() + static_cast<long>(j));
            } else {
                ++j;
            }
        }
    }
}

bool place_in_space(const Box &box, const Bin &bin, const Space &space, SplitPolicy policy,
                    const std::vector<Placement> &existing, const Instance &instance, Placement &out,
                    std::vector<Space> &new_spaces) {
    auto orientations = orientations_for(box);
    std::sort(orientations.begin(), orientations.end(), [&](const Vec3 &a, const Vec3 &b) {
        const int rem_ha = space.size.h - a.h;
        const int rem_hb = space.size.h - b.h;
        if (space.allow_stack && rem_ha != rem_hb) {
            return rem_ha > rem_hb; // prefer leaving more headroom
        }
        return volume(a) > volume(b);
    });

    for (const auto &orientation : orientations) {
        if (!fits(orientation, space)) {
            continue;
        }
        out.box_id = box.id;
        out.bin_id = bin.id;
        out.position = space.origin;
        out.orientation = orientation;

        if (!can_place_with(out, box, existing, instance)) {
            continue;
        }

        new_spaces.clear();

        if (policy == SplitPolicy::Guillotine) {
            const int rem_w = space.size.w - orientation.w;
            const int rem_l = space.size.l - orientation.l;
            const int rem_h = space.size.h - orientation.h;

            if (rem_w > 0) {
                new_spaces.push_back(Space{Vec3{space.origin.w + orientation.w, space.origin.l, space.origin.h},
                                           Vec3{rem_w, orientation.l, orientation.h}, space.allow_stack});
            }
            if (rem_l > 0) {
                new_spaces.push_back(Space{Vec3{space.origin.w, space.origin.l + orientation.l, space.origin.h},
                                           Vec3{space.size.w, rem_l, orientation.h}, space.allow_stack});
            }
            if (space.allow_stack && !box.no_stack && rem_h > 0) {
                new_spaces.push_back(Space{Vec3{space.origin.w, space.origin.l, space.origin.h + orientation.h},
                                           Vec3{space.size.w, space.size.l, space.size.h - orientation.h},
                                           space.allow_stack});
            }
        } else {
            // Maximal split
            new_spaces.push_back(Space{Vec3{space.origin.w + orientation.w, space.origin.l, space.origin.h},
                                       Vec3{space.size.w - orientation.w, orientation.l, orientation.h},
                                       space.allow_stack});
            new_spaces.push_back(Space{Vec3{space.origin.w, space.origin.l + orientation.l, space.origin.h},
                                       Vec3{space.size.w, space.size.l - orientation.l, orientation.h},
                                       space.allow_stack});
            if (space.allow_stack && !box.no_stack) {
                new_spaces.push_back(Space{Vec3{space.origin.w, space.origin.l, space.origin.h + orientation.h},
                                           Vec3{space.size.w, space.size.l, space.size.h - orientation.h},
                                           space.allow_stack});
            }
        }

        prune(new_spaces);
        return true;
    }
    return false;
}

void summarize_result(const Instance &instance, PackingResult &result) {
    std::unordered_set<std::string> used_bins;
    long long packed_volume = 0;

    for (const auto &placement : result.placements) {
        used_bins.insert(placement.bin_id);
        packed_volume += volume(placement.orientation);
    }

    const long long used_bin_volume =
        std::accumulate(instance.bins.begin(), instance.bins.end(), 0LL, [&](long long total, const Bin &bin) {
            return used_bins.contains(bin.id) ? total + volume(bin.size) : total;
        });

    result.unplaced_box_ids = unplaced_box_ids(instance, result);
    result.objective.bins_used = static_cast<int>(used_bins.size());
    result.objective.leftover_volume = used_bin_volume - packed_volume;

    if (result.unplaced_box_ids.empty()) {
        result.status = PackingStatus::Feasible;
        result.feasible = true;
    } else if (result.placements.empty()) {
        result.status = PackingStatus::Infeasible;
        result.feasible = false;
    } else {
        result.status = PackingStatus::Partial;
        result.feasible = false;
    }
}

} // namespace

std::vector<Box> sort_by_volume_desc(const std::vector<Box> &boxes) {
    std::vector<Box> sorted = boxes;
    std::sort(sorted.begin(), sorted.end(), [](const Box &a, const Box &b) { return volume(a.size) > volume(b.size); });
    return sorted;
}

PackingResult greedy_pack(const Instance &instance, const std::vector<Box> &ordered_boxes, GreedyConfig config) {
    PackingResult result;
    if (instance.bins.empty()) {
        summarize_result(instance, result);
        return result;
    }

    std::vector<std::vector<Space>> bin_spaces;
    bin_spaces.reserve(instance.bins.size());
    std::transform(instance.bins.begin(), instance.bins.end(), std::back_inserter(bin_spaces),
                   [](const Bin &bin) { return std::vector<Space>{make_root_space(bin)}; });

    std::vector<Placement> placements;
    placements.reserve(ordered_boxes.size());

    std::size_t placed_count = 0;
    for (const auto &box : ordered_boxes) {
        bool placed = false;
        for (std::size_t b = 0; b < instance.bins.size(); ++b) {
            auto &spaces = bin_spaces[b];
            Placement candidate;
            std::vector<Space> new_spaces;
            for (std::size_t i = 0; i < spaces.size(); ++i) {
                if (place_in_space(box, instance.bins[b], spaces[i], config.policy, placements, instance, candidate,
                                   new_spaces)) {
                    spaces.erase(spaces.begin() + static_cast<long>(i));
                    spaces.insert(spaces.end(), new_spaces.begin(), new_spaces.end());
                    prune(spaces);
                    placements.push_back(candidate);
                    placed = true;
                    break;
                }
            }
            if (placed) {
                break;
            }
        }
        if (!placed) {
            result.placements = placements;
            summarize_result(instance, result);
            return result;
        }
        ++placed_count;
    }

    (void)placed_count;
    result.placements = placements;
    summarize_result(instance, result);
    return result;
}

} // namespace bp::algorithms
