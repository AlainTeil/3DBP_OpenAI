#pragma once

#include <string>
#include <vector>

#include "bp/model.hpp"

namespace bp {

/// Result of validating an instance or packing result.
struct ValidationReport {
    /// True when no errors were found.
    bool valid{true};
    /// Human-readable validation errors.
    std::vector<std::string> errors;
};

/// Validate that bin and box identifiers are non-empty and unique and that all dimensions are positive.
ValidationReport validate_instance(const Instance &instance);

/// Validate placements against an instance and the packing invariants.
///
/// Checks known IDs, duplicate box placements, bounds, legal rotations, this_side_up, overlap,
/// no_stack, and feasible-result completeness.
ValidationReport validate_result(const Instance &instance, const PackingResult &result);

/// Return the instance box identifiers that do not appear in result placements.
std::vector<std::string> unplaced_box_ids(const Instance &instance, const PackingResult &result);

/// Return whether candidate can be added to existing placements without overlap or no_stack violations.
bool can_place_with(const Placement &candidate, const Box &box, const std::vector<Placement> &existing,
                    const Instance &instance);

} // namespace bp
