#include <filesystem>
#include <random>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "bp/instance_io.hpp"
#include "bp/packer.hpp"

using namespace bp;

TEST(System, PipelineRoundTripJson) {
    Instance inst;
    inst.bins.push_back(Bin{.id = "bin-0", .size = Vec3{12, 12, 12}});
    inst.boxes.push_back(Box{.id = "base", .size = Vec3{6, 12, 6}, .no_stack = true});
    inst.boxes.push_back(Box{.id = "top", .size = Vec3{4, 4, 4}});

    const auto tmp_dir = std::filesystem::temp_directory_path();
    const auto inst_path = tmp_dir / "pipeline_inst.json";
    const auto res_path = tmp_dir / "pipeline_res.json";

    save_instance(inst, inst_path.string());
    auto loaded_inst = load_instance(inst_path.string());

    auto result = pack(loaded_inst, PackOptions{AlgorithmId::MaximalSpace, 7u, 10});
    save_result(result, res_path.string());
    auto loaded_res = load_result(res_path.string());

    EXPECT_EQ(loaded_res.feasible, result.feasible);
    ASSERT_EQ(loaded_res.placements.size(), result.placements.size());
    EXPECT_EQ(loaded_res.objective.bins_used, 1);
    // Ensure no stacking occurred above the no-stack base.
    const auto top_it = std::find_if(loaded_res.placements.begin(), loaded_res.placements.end(), [](const Placement& p) {
        return p.box_id == "top";
    });
    ASSERT_NE(top_it, loaded_res.placements.end());
    EXPECT_EQ(top_it->position.h, 0);
}

TEST(System, OnlineFFDHandlesStream) {
    Instance inst;
    inst.bins.push_back(Bin{.id = "bin-0", .size = Vec3{20, 20, 20}});

    std::mt19937 rng(123);
    std::uniform_int_distribution<int> dist(2, 6);
    for (int i = 0; i < 15; ++i) {
        inst.boxes.push_back(Box{.id = "box-" + std::to_string(i), .size = Vec3{dist(rng), dist(rng), dist(rng)}});
    }

    auto res = pack(inst, PackOptions{AlgorithmId::OnlineFFD, 123u, 0});
    EXPECT_TRUE(res.feasible);
    EXPECT_LE(res.objective.bins_used, 1);
}
