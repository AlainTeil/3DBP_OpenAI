#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "bp/packer.hpp"

using namespace bp;

TEST(Integration, ThisSideUpRespected) {
    Instance inst;
    inst.bins.push_back(Bin{.id = "bin-0", .size = Vec3{10, 10, 10}});
    inst.boxes.push_back(Box{.id = "upright", .size = Vec3{4, 2, 6}, .this_side_up = true, .no_stack = false});
    inst.boxes.push_back(Box{.id = "filler", .size = Vec3{2, 2, 2}});

    auto res = pack(inst, PackOptions{AlgorithmId::FFD, 123u, 20});
    ASSERT_TRUE(res.feasible);
    const auto it = std::find_if(res.placements.begin(), res.placements.end(), [](const Placement& p) {
        return p.box_id == "upright";
    });
    ASSERT_NE(it, res.placements.end());
    EXPECT_EQ(it->orientation.h, 6);
}

TEST(Integration, GuillotinePacksTwoSlabs) {
    Instance inst;
    inst.bins.push_back(Bin{.id = "bin-0", .size = Vec3{10, 10, 10}});
    inst.boxes.push_back(Box{.id = "a", .size = Vec3{10, 10, 5}});
    inst.boxes.push_back(Box{.id = "b", .size = Vec3{10, 10, 5}});

    auto res = pack(inst, PackOptions{AlgorithmId::Guillotine, 0u, 10});
    ASSERT_TRUE(res.feasible);
    EXPECT_EQ(res.objective.bins_used, 1);
    EXPECT_EQ(res.objective.leftover_volume, 0);
}

TEST(Integration, MetaHeuristicFindsFeasible) {
    Instance inst;
    inst.bins.push_back(Bin{.id = "bin-0", .size = Vec3{8, 8, 8}});
    inst.boxes.push_back(Box{.id = "a", .size = Vec3{4, 4, 4}});
    inst.boxes.push_back(Box{.id = "b", .size = Vec3{4, 4, 4}});
    inst.boxes.push_back(Box{.id = "c", .size = Vec3{4, 4, 4}});

    auto res = pack(inst, PackOptions{AlgorithmId::MetaGA, 42u, 8});
    EXPECT_TRUE(res.feasible);
    EXPECT_EQ(res.objective.bins_used, 1);
}
