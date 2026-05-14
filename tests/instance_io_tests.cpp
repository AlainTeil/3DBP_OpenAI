#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "bp/instance_io.hpp"

using namespace bp;

namespace {

std::filesystem::path write_temp_json(const std::string &name, const std::string &contents) {
    const auto path = std::filesystem::temp_directory_path() / name;
    std::ofstream out(path);
    out << contents;
    return path;
}

} // namespace

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
    res.status = PackingStatus::Feasible;
    res.feasible = true;
    res.objective = Objective{.bins_used = 1, .leftover_volume = 100};
    res.unplaced_box_ids = {};
    res.placements.push_back(
        Placement{.box_id = "box-a", .bin_id = "bin-a", .position = Vec3{1, 2, 3}, .orientation = Vec3{4, 5, 6}});

    const auto tmp = std::filesystem::temp_directory_path() / "res.json";
    save_result(res, tmp.string());

    auto loaded = load_result(tmp.string());
    EXPECT_TRUE(loaded.feasible);
    EXPECT_EQ(loaded.status, PackingStatus::Feasible);
    EXPECT_EQ(loaded.objective.bins_used, 1);
    ASSERT_EQ(loaded.placements.size(), 1u);
    EXPECT_EQ(loaded.placements[0].position.h, 3);
    EXPECT_EQ(loaded.placements[0].orientation.l, 5);
}

TEST(InstanceIO, RejectsInvalidInstanceDimensions) {
    const auto tmp = std::filesystem::temp_directory_path() / "invalid_inst.json";
    std::ofstream out(tmp);
    out << R"({"bins":[{"id":"bin-a","size":{"w":0,"l":10,"h":10}}],"boxes":[]})";
    out.close();

    EXPECT_THROW((void)load_instance(tmp.string()), std::runtime_error);
}

TEST(InstanceIO, RejectsMalformedAndInvalidInstanceJson) {
    const std::vector<std::pair<std::string, std::string>> cases = {
        {"missing_bins.json", R"({"boxes":[]})"},
        {"missing_size_component.json", R"({"bins":[{"id":"bin-a","size":{"w":1,"l":2}}],"boxes":[]})"},
        {"duplicate_ids.json",
         R"({"bins":[{"id":"bin-a","size":{"w":5,"l":5,"h":5}},{"id":"bin-a","size":{"w":5,"l":5,"h":5}}],"boxes":[]})"},
        {"negative_box_dimension.json",
         R"({"bins":[{"id":"bin-a","size":{"w":5,"l":5,"h":5}}],"boxes":[{"id":"box-a","size":{"w":1,"l":-1,"h":1}}]})"},
        {"not_json.json", R"({"bins":[)"},
    };

    for (const auto &[name, contents] : cases) {
        SCOPED_TRACE(name);
        const auto path = write_temp_json(name, contents);
        EXPECT_THROW((void)load_instance(path.string()), std::exception);
    }
}

TEST(InstanceIO, LoadsLegacyResultWithoutStatus) {
    const auto path = write_temp_json("legacy_result.json", R"({
        "feasible": false,
        "objective": {"bins_used": 0, "leftover_volume": 0},
        "placements": []
    })");

    const auto result = load_result(path.string());
    EXPECT_EQ(result.status, PackingStatus::Infeasible);
    EXPECT_FALSE(result.feasible);
    EXPECT_TRUE(result.placements.empty());
}

TEST(InstanceIO, StatusFieldOverridesLegacyFeasibleShortcut) {
    const auto path = write_temp_json("status_overrides_feasible.json", R"({
        "status": "partial",
        "feasible": true,
        "objective": {"bins_used": 1, "leftover_volume": 10},
        "unplaced_box_ids": ["box-b"],
        "placements": [
            {
                "box_id": "box-a",
                "bin_id": "bin-a",
                "position": {"x": 0, "y": 0, "z": 0},
                "orientation": {"w": 1, "l": 1, "h": 1}
            }
        ]
    })");

    const auto result = load_result(path.string());
    EXPECT_EQ(result.status, PackingStatus::Partial);
    EXPECT_FALSE(result.feasible);
    EXPECT_THAT(result.unplaced_box_ids, ::testing::ElementsAre("box-b"));
}

TEST(InstanceIO, RejectsUnknownResultStatus) {
    const auto path = write_temp_json("unknown_status_result.json", R"({
        "status": "mostly-fine",
        "objective": {"bins_used": 0, "leftover_volume": 0},
        "placements": []
    })");

    EXPECT_THROW((void)load_result(path.string()), std::runtime_error);
}
