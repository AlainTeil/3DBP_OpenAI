#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#include "bp/packer.hpp"
#include "bp/validation.hpp"

#include "test_helpers.hpp"

using namespace bp;

namespace {

bool footprint_overlaps(const Placement &left, const Placement &right) {
    const bool width_overlap = left.position.w < right.position.w + right.orientation.w &&
                               right.position.w < left.position.w + left.orientation.w;
    const bool length_overlap = left.position.l < right.position.l + right.orientation.l &&
                                right.position.l < left.position.l + left.orientation.l;
    return width_overlap && length_overlap;
}

} // namespace

TEST(AdversarialPacking, NoStackBaseKeepsOtherBoxesOffItsFootprint) {
    const std::vector<AlgorithmId> algorithms = {AlgorithmId::FFD,          AlgorithmId::NFDH,
                                                 AlgorithmId::Layered,      AlgorithmId::Guillotine,
                                                 AlgorithmId::MaximalSpace, AlgorithmId::OnlineFFD};

    Instance instance;
    instance.bins.push_back(Bin{.id = "bin-0", .size = Vec3{10, 10, 8}});
    instance.boxes.push_back(Box{.id = "base", .size = Vec3{5, 5, 4}, .no_stack = true});
    instance.boxes.push_back(Box{.id = "neighbor", .size = Vec3{5, 5, 4}});
    instance.boxes.push_back(Box{.id = "filler", .size = Vec3{5, 5, 2}});

    for (const auto algorithm : algorithms) {
        SCOPED_TRACE(::testing::Message() << "algorithm=" << static_cast<int>(algorithm));
        const auto result = pack(instance, PackOptions{algorithm, 17u, 12});
        EXPECT_EQ(result.status, PackingStatus::Feasible);
        expect_valid_packing(instance, result);

        const auto base_it = std::find_if(result.placements.begin(), result.placements.end(),
                                          [](const Placement &placement) { return placement.box_id == "base"; });
        ASSERT_NE(base_it, result.placements.end());
        for (const auto &placement : result.placements) {
            if (placement.box_id == "base" || placement.bin_id != base_it->bin_id) {
                continue;
            }
            EXPECT_FALSE(footprint_overlaps(*base_it, placement) &&
                         placement.position.h >= base_it->position.h + base_it->orientation.h)
                << placement.box_id;
        }
    }
}

TEST(AdversarialPacking, ThisSideUpPreventsOnlyFittingVerticalRotation) {
    Instance instance;
    instance.bins.push_back(Bin{.id = "bin-0", .size = Vec3{6, 4, 3}});
    instance.boxes.push_back(Box{.id = "upright", .size = Vec3{3, 6, 4}, .this_side_up = true});

    const auto result = pack(instance, PackOptions{AlgorithmId::FFD, 0u, 5});
    EXPECT_EQ(result.status, PackingStatus::Infeasible);
    EXPECT_FALSE(result.feasible);
    EXPECT_TRUE(result.placements.empty());
    EXPECT_THAT(result.unplaced_box_ids, ::testing::ElementsAre("upright"));
    expect_valid_packing(instance, result);
}

TEST(AdversarialPacking, DistinguishesPartialFromInfeasible) {
    Instance instance;
    instance.bins.push_back(Bin{.id = "bin-0", .size = Vec3{5, 5, 5}});
    instance.boxes.push_back(Box{.id = "fits", .size = Vec3{5, 5, 5}});
    instance.boxes.push_back(Box{.id = "too-big", .size = Vec3{6, 4, 4}});

    const auto result = pack(instance, PackOptions{AlgorithmId::FFD, 0u, 5});
    EXPECT_EQ(result.status, PackingStatus::Partial);
    EXPECT_FALSE(result.feasible);
    EXPECT_THAT(result.unplaced_box_ids, ::testing::ElementsAre("too-big"));
    ASSERT_EQ(result.placements.size(), 1u);
    EXPECT_EQ(result.placements.front().box_id, "fits");
    expect_valid_packing(instance, result);
}

TEST(ValidationRegression, DetectsIndependentPlacementInvariantFailures) {
    Instance instance;
    instance.bins.push_back(Bin{.id = "bin-0", .size = Vec3{10, 10, 10}});
    instance.boxes.push_back(Box{.id = "base", .size = Vec3{5, 5, 2}, .no_stack = true});
    instance.boxes.push_back(Box{.id = "top", .size = Vec3{5, 5, 2}});
    instance.boxes.push_back(Box{.id = "upright", .size = Vec3{2, 3, 4}, .this_side_up = true});
    instance.boxes.push_back(Box{.id = "outside", .size = Vec3{2, 2, 2}});

    PackingResult result;
    result.status = PackingStatus::Feasible;
    result.feasible = true;
    result.placements = {
        Placement{.box_id = "base", .bin_id = "bin-0", .position = Vec3{0, 0, 0}, .orientation = Vec3{5, 5, 2}},
        Placement{.box_id = "top", .bin_id = "bin-0", .position = Vec3{0, 0, 2}, .orientation = Vec3{5, 5, 2}},
        Placement{.box_id = "top", .bin_id = "bin-0", .position = Vec3{5, 0, 0}, .orientation = Vec3{5, 5, 2}},
        Placement{.box_id = "upright", .bin_id = "bin-0", .position = Vec3{5, 5, 0}, .orientation = Vec3{2, 4, 3}},
        Placement{.box_id = "outside", .bin_id = "bin-0", .position = Vec3{9, 9, 9}, .orientation = Vec3{2, 2, 2}},
        Placement{.box_id = "ghost", .bin_id = "bin-0", .position = Vec3{0, 5, 0}, .orientation = Vec3{1, 1, 1}},
    };

    const auto report = validate_result(instance, result);
    EXPECT_FALSE(report.valid);
    EXPECT_THAT(report.errors, ::testing::Contains(::testing::HasSubstr("box placed more than once: top")));
    EXPECT_THAT(report.errors,
                ::testing::Contains(::testing::HasSubstr("this_side_up box changed height axis: upright")));
    EXPECT_THAT(report.errors, ::testing::Contains(::testing::HasSubstr("placement is outside bin bounds: outside")));
    EXPECT_THAT(report.errors, ::testing::Contains(::testing::HasSubstr("placement references unknown box: ghost")));
    EXPECT_THAT(report.errors,
                ::testing::Contains(::testing::HasSubstr("no_stack box has another box above its footprint: base")));
}

TEST(ValidationRegression, CanPlaceRejectsNoStackViolationsInBothDirections) {
    Instance instance;
    instance.bins.push_back(Bin{.id = "bin-0", .size = Vec3{10, 10, 10}});
    const Box base{.id = "base", .size = Vec3{5, 5, 2}, .no_stack = true};
    const Box top{.id = "top", .size = Vec3{5, 5, 2}};
    instance.boxes.push_back(base);
    instance.boxes.push_back(top);

    const Placement base_placement{
        .box_id = "base", .bin_id = "bin-0", .position = Vec3{0, 0, 0}, .orientation = Vec3{5, 5, 2}};
    const Placement top_placement{
        .box_id = "top", .bin_id = "bin-0", .position = Vec3{0, 0, 2}, .orientation = Vec3{5, 5, 2}};

    EXPECT_FALSE(can_place_with(top_placement, top, {base_placement}, instance));
    EXPECT_FALSE(can_place_with(base_placement, base, {top_placement}, instance));
}