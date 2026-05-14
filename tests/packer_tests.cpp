#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include "bp/instance_io.hpp"
#include "bp/packer.hpp"

#include "test_helpers.hpp"

using namespace bp;

TEST(Packer, SimpleFFDFeasible) {
    Instance inst;
    inst.bins.push_back(Bin{.id = "bin-0", .size = Vec3{10, 10, 10}});
    inst.boxes.push_back(Box{.id = "a", .size = Vec3{5, 5, 5}});
    inst.boxes.push_back(Box{.id = "b", .size = Vec3{5, 5, 5}});
    auto res = pack(inst, PackOptions{AlgorithmId::FFD, 0u, 50});
    EXPECT_EQ(res.status, PackingStatus::Feasible);
    EXPECT_TRUE(res.feasible);
    EXPECT_EQ(res.objective.bins_used, 1);
    expect_valid_packing(inst, res);
}

TEST(Packer, HonorsNoStack) {
    Instance inst;
    inst.bins.push_back(Bin{.id = "bin-0", .size = Vec3{10, 10, 10}});
    inst.boxes.push_back(Box{.id = "base", .size = Vec3{5, 10, 5}, .this_side_up = false, .no_stack = true});
    inst.boxes.push_back(Box{.id = "top", .size = Vec3{5, 5, 2}});
    auto res = pack(inst, PackOptions{AlgorithmId::FFD, 0u, 50});
    ASSERT_TRUE(res.feasible);
    expect_valid_packing(inst, res);
    // ensure no placement above z=5 for base
    const auto top_it = std::find_if(res.placements.begin(), res.placements.end(),
                                     [](const Placement &placement) { return placement.box_id == "top"; });
    ASSERT_NE(top_it, res.placements.end());
    EXPECT_EQ(top_it->position.h, 0);
}

TEST(Packer, AllAlgorithmsReturnValidPackingOnAuditSeed) {
    const std::vector<AlgorithmId> algorithms = {
        AlgorithmId::FFD,        AlgorithmId::NFDH,         AlgorithmId::Layered,
        AlgorithmId::Guillotine, AlgorithmId::MaximalSpace, AlgorithmId::MetaGA,
        AlgorithmId::MetaGRASP,  AlgorithmId::MetaSA,       AlgorithmId::OnlineFFD};

    Instance inst;
    inst.bins.push_back(Bin{.id = "bin-0", .size = Vec3{20, 20, 20}});
    inst.bins.push_back(Bin{.id = "bin-1", .size = Vec3{20, 20, 20}});
    inst.boxes = {
        Box{.id = "box-0", .size = Vec3{5, 5, 2}},  Box{.id = "box-1", .size = Vec3{5, 6, 5}},
        Box{.id = "box-2", .size = Vec3{3, 6, 3}},  Box{.id = "box-3", .size = Vec3{6, 7, 6}},
        Box{.id = "box-4", .size = Vec3{4, 2, 7}},  Box{.id = "box-5", .size = Vec3{6, 2, 4}},
        Box{.id = "box-6", .size = Vec3{6, 7, 4}},  Box{.id = "box-7", .size = Vec3{2, 4, 5}},
        Box{.id = "box-8", .size = Vec3{6, 2, 2}},  Box{.id = "box-9", .size = Vec3{7, 7, 2}},
        Box{.id = "box-10", .size = Vec3{5, 7, 4}}, Box{.id = "box-11", .size = Vec3{7, 3, 6}},
        Box{.id = "box-12", .size = Vec3{6, 5, 2}}, Box{.id = "box-13", .size = Vec3{6, 5, 4}},
        Box{.id = "box-14", .size = Vec3{6, 3, 6}}, Box{.id = "box-15", .size = Vec3{5, 4, 6}},
        Box{.id = "box-16", .size = Vec3{5, 6, 4}}, Box{.id = "box-17", .size = Vec3{3, 3, 7}},
    };
    inst.boxes[4].no_stack = true;
    inst.boxes[7].this_side_up = true;

    for (const auto algorithm : algorithms) {
        auto result = pack(inst, PackOptions{algorithm, 0u, 25});
        expect_valid_packing(inst, result);
    }
}

TEST(Packer, InvalidInstanceThrowsWithContext) {
    Instance inst;
    inst.bins.push_back(Bin{.id = "bin-0", .size = Vec3{0, 10, 10}});
    inst.boxes.push_back(Box{.id = "box-0", .size = Vec3{1, 1, 1}});

    EXPECT_THROW((void)pack(inst, PackOptions{AlgorithmId::FFD, 0u, 1}), std::runtime_error);
}
