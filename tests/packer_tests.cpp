#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "bp/instance_io.hpp"
#include "bp/packer.hpp"

using namespace bp;

TEST(Packer, SimpleFFDFeasible) {
    Instance inst;
    inst.bins.push_back(Bin{.id = "bin-0", .size = Vec3{10, 10, 10}});
    inst.boxes.push_back(Box{.id = "a", .size = Vec3{5, 5, 5}});
    inst.boxes.push_back(Box{.id = "b", .size = Vec3{5, 5, 5}});
    auto res = pack(inst, PackOptions{AlgorithmId::FFD, 0u, 50});
    EXPECT_TRUE(res.feasible);
    EXPECT_EQ(res.objective.bins_used, 1);
}

TEST(Packer, HonorsNoStack) {
    Instance inst;
    inst.bins.push_back(Bin{.id = "bin-0", .size = Vec3{10, 10, 10}});
    inst.boxes.push_back(Box{.id = "base", .size = Vec3{5, 10, 5}, .this_side_up = false, .no_stack = true});
    inst.boxes.push_back(Box{.id = "top", .size = Vec3{5, 5, 2}});
    auto res = pack(inst, PackOptions{AlgorithmId::FFD, 0u, 50});
    ASSERT_TRUE(res.feasible);
    // ensure no placement above z=5 for base
    int top_z = 0;
    for (const auto& p : res.placements) {
        if (p.box_id == "top") {
            top_z = p.position.h;
        }
    }
    EXPECT_EQ(top_z, 0);
}
