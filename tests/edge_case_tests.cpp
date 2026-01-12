#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "bp/packer.hpp"

using namespace bp;

TEST(EdgeCases, NoBinsInfeasible) {
    Instance inst;
    inst.boxes.push_back(Box{.id = "a", .size = Vec3{1, 1, 1}});
    auto res = pack(inst, PackOptions{AlgorithmId::FFD, 0u, 5});
    EXPECT_FALSE(res.feasible);
    EXPECT_TRUE(res.placements.empty());
}

TEST(EdgeCases, EmptyBoxesTriviallyFeasible) {
    Instance inst;
    inst.bins.push_back(Bin{.id = "bin-0", .size = Vec3{5, 5, 5}});
    auto res = pack(inst, PackOptions{AlgorithmId::FFD, 0u, 5});
    EXPECT_TRUE(res.feasible);
    EXPECT_EQ(res.objective.bins_used, 0);
    EXPECT_EQ(res.objective.leftover_volume, 0);
}

TEST(EdgeCases, BoxExactlyFitsBin) {
    Instance inst;
    inst.bins.push_back(Bin{.id = "bin-0", .size = Vec3{3, 4, 5}});
    inst.boxes.push_back(Box{.id = "box-0", .size = Vec3{3, 4, 5}});
    auto res = pack(inst, PackOptions{AlgorithmId::FFD, 0u, 5});
    ASSERT_TRUE(res.feasible);
    EXPECT_EQ(res.objective.bins_used, 1);
    EXPECT_EQ(res.objective.leftover_volume, 0);
}

TEST(EdgeCases, OversizeBoxInfeasible) {
    Instance inst;
    inst.bins.push_back(Bin{.id = "bin-0", .size = Vec3{2, 2, 2}});
    inst.boxes.push_back(Box{.id = "big", .size = Vec3{3, 2, 2}});
    auto res = pack(inst, PackOptions{AlgorithmId::FFD, 0u, 5});
    EXPECT_FALSE(res.feasible);
}

TEST(EdgeCases, OnlineFFDHandlesSingleTallBox) {
    Instance inst;
    inst.bins.push_back(Bin{.id = "bin-0", .size = Vec3{4, 4, 10}});
    inst.boxes.push_back(Box{.id = "tall", .size = Vec3{4, 4, 10}, .this_side_up = true});
    auto res = pack(inst, PackOptions{AlgorithmId::OnlineFFD, 0u, 0});
    EXPECT_TRUE(res.feasible);
    EXPECT_EQ(res.objective.bins_used, 1);
}
