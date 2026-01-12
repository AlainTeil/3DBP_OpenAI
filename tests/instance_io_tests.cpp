#include <filesystem>
#include <fstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "bp/instance_io.hpp"

using namespace bp;

TEST(InstanceIO, RoundTripInstance) {
    Instance inst;
    inst.bins.push_back(Bin{.id = "bin-a", .size = Vec3{10, 12, 14}});
    inst.boxes.push_back(Box{.id = "box-a", .size = Vec3{2, 3, 4}, .this_side_up = true, .no_stack = false});
    inst.boxes.push_back(Box{.id = "box-b", .size = Vec3{5, 5, 5}, .this_side_up = false, .no_stack = true});

    const auto tmp = std::filesystem::temp_directory_path() / "inst.json";
    save_instance(inst, tmp.string());

    auto loaded = load_instance(tmp.string());
    ASSERT_EQ(loaded.bins.size(), 1u);
    ASSERT_EQ(loaded.boxes.size(), 2u);
    EXPECT_EQ(loaded.bins[0].id, "bin-a");
    EXPECT_EQ(loaded.bins[0].size.w, 10);
    EXPECT_TRUE(loaded.boxes[0].this_side_up);
    EXPECT_TRUE(loaded.boxes[1].no_stack);
}

TEST(InstanceIO, RoundTripResult) {
    PackingResult res;
    res.feasible = true;
    res.objective = Objective{.bins_used = 1, .leftover_volume = 100};
    res.placements.push_back(Placement{.box_id = "box-a",
                                       .bin_id = "bin-a",
                                       .position = Vec3{1, 2, 3},
                                       .orientation = Vec3{4, 5, 6}});

    const auto tmp = std::filesystem::temp_directory_path() / "res.json";
    save_result(res, tmp.string());

    auto loaded = load_result(tmp.string());
    EXPECT_TRUE(loaded.feasible);
    EXPECT_EQ(loaded.objective.bins_used, 1);
    ASSERT_EQ(loaded.placements.size(), 1u);
    EXPECT_EQ(loaded.placements[0].position.h, 3);
    EXPECT_EQ(loaded.placements[0].orientation.l, 5);
}
