#pragma once

#include <string>
#include <utility>
#include <vector>

namespace bp {

/// Axis-aligned integer dimensions.
struct Vec3 {
    /// Width component.
    int w{0};
    /// Length component.
    int l{0};
    /// Height component.
    int h{0};
};

inline bool operator==(const Vec3& a, const Vec3& b) {
    return a.w == b.w && a.l == b.l && a.h == b.h;
}

/// A single box to be packed.
struct Box {
    /// Unique box identifier.
    std::string id;
    /// Original box dimensions.
    Vec3 size;
    /// If true, box cannot be rotated vertically.
    bool this_side_up{false};
    /// If true, other boxes cannot be stacked on top.
    bool no_stack{false};
};

/// A container that holds boxes.
struct Bin {
    /// Unique bin identifier.
    std::string id;
    /// Inner usable dimensions.
    Vec3 size;
};

/// Position and orientation of a placed box.
struct Placement {
    /// Identifier of the placed box.
    std::string box_id;
    /// Identifier of the target bin.
    std::string bin_id;
    /// Anchor position; x=w-axis, y=l-axis, z=height.
    Vec3 position;
    /// Resulting oriented size after rotation.
    Vec3 orientation;
};

/// Packing instance describing bins and boxes.
struct Instance {
    /// Available bins.
    std::vector<Bin> bins;
    /// Boxes to pack.
    std::vector<Box> boxes;
};

/// Objective values for a packing run.
struct Objective {
    /// Number of bins that ended up used.
    int bins_used{0};
    /// Unused volume across used bins.
    long long leftover_volume{0};
};

/// Aggregate statistics of the packing.
struct PackingStats {
    /// Total boxes in the instance.
    int boxes_total{0};
    /// Boxes successfully placed.
    int boxes_placed{0};
    /// Sum of all box volumes.
    long long volume_boxes{0};
    /// Packed volume of placed boxes.
    long long volume_packed{0};
    /// Volume of bins that were used.
    long long volume_bins_used{0};
    /// Ratio of packed volume to used bin volume.
    double fill_ratio{0.0};
};

/// Metadata describing how the packing run was produced.
struct RunMetadata {
    /// Algorithm label used for the run.
    std::string algorithm;
    /// Random seed value.
    unsigned int seed{0};
    /// Number of iterations, if applicable.
    int iterations{0};
    /// Input instance filename, if provided.
    std::string instance_file;
    /// Timestamp for the run.
    std::string timestamp;
};

/// Full result of a packing run.
struct PackingResult {
    /// True when a valid packing was found.
    bool feasible{false};
    /// Placements for all packed boxes.
    std::vector<Placement> placements;
    /// Objective outcomes.
    Objective objective;
    /// Packing statistics.
    PackingStats stats;
    /// How the run was produced.
    RunMetadata metadata;
};

/// Axis-aligned free space within a bin.
struct Space {
    /// Origin corner of the space.
    Vec3 origin;
    /// Dimensions of the free space.
    Vec3 size;
    /// Whether stacking is allowed in this space.
    bool allow_stack{true};
};

std::vector<Vec3> orientations_for(const Box& box);

bool fits(const Vec3& item, const Space& space);

bool overlaps(const Placement& a, const Placement& b);

long long volume(const Vec3& v);

}  // namespace bp
