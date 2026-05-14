#include "bp/model.hpp"

#include <algorithm>
#include <stdexcept>

namespace bp {

std::vector<Vec3> orientations_for(const Box &box) {
    const auto &s = box.size;
    std::vector<Vec3> result;
    if (box.this_side_up) {
        result.push_back(s);
        result.push_back(Vec3{s.l, s.w, s.h});
        result.erase(std::unique(result.begin(), result.end(),
                                 [](const Vec3 &a, const Vec3 &b) { return a.w == b.w && a.l == b.l && a.h == b.h; }),
                     result.end());
        return result;
    }
    result.push_back(s);
    result.push_back(Vec3{s.w, s.h, s.l});
    result.push_back(Vec3{s.l, s.w, s.h});
    result.push_back(Vec3{s.l, s.h, s.w});
    result.push_back(Vec3{s.h, s.w, s.l});
    result.push_back(Vec3{s.h, s.l, s.w});
    result.erase(std::unique(result.begin(), result.end(),
                             [](const Vec3 &a, const Vec3 &b) { return a.w == b.w && a.l == b.l && a.h == b.h; }),
                 result.end());
    return result;
}

bool fits(const Vec3 &item, const Space &space) {
    return item.w <= space.size.w && item.l <= space.size.l && item.h <= space.size.h;
}

bool overlaps(const Placement &a, const Placement &b) {
    if (a.bin_id != b.bin_id) {
        return false;
    }
    const bool x_overlap =
        a.position.w < b.position.w + b.orientation.w && b.position.w < a.position.w + a.orientation.w;
    const bool y_overlap =
        a.position.l < b.position.l + b.orientation.l && b.position.l < a.position.l + a.orientation.l;
    const bool z_overlap =
        a.position.h < b.position.h + b.orientation.h && b.position.h < a.position.h + a.orientation.h;
    return x_overlap && y_overlap && z_overlap;
}

long long volume(const Vec3 &v) {
    return static_cast<long long>(v.w) * static_cast<long long>(v.l) * static_cast<long long>(v.h);
}

std::string to_string(PackingStatus status) {
    switch (status) {
    case PackingStatus::Feasible:
        return "feasible";
    case PackingStatus::Partial:
        return "partial";
    case PackingStatus::Infeasible:
        return "infeasible";
    case PackingStatus::Invalid:
        return "invalid";
    }
    return "invalid";
}

PackingStatus packing_status_from_string(const std::string &value) {
    if (value == "feasible") {
        return PackingStatus::Feasible;
    }
    if (value == "partial") {
        return PackingStatus::Partial;
    }
    if (value == "infeasible") {
        return PackingStatus::Infeasible;
    }
    if (value == "invalid") {
        return PackingStatus::Invalid;
    }
    throw std::runtime_error("Unknown packing status: " + value);
}

} // namespace bp
