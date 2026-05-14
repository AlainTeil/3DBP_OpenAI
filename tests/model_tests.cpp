#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "bp/model.hpp"

using namespace bp;

TEST(Model, OrientationRespectsThisSideUp) {
    Box b{.id = "b1", .size = Vec3{2, 3, 4}, .this_side_up = true, .no_stack = false};
    auto o = orientations_for(b);
    EXPECT_THAT(o, ::testing::Contains(Vec3{2, 3, 4}));
    EXPECT_THAT(o, ::testing::Contains(Vec3{3, 2, 4}));
    for (const auto &v : o) {
        EXPECT_EQ(v.h, 4);
    }
}

TEST(Model, Volume) { EXPECT_EQ(volume(Vec3{2, 3, 4}), 24); }

TEST(Model, PackingStatusRoundTrip) {
    EXPECT_EQ(packing_status_from_string(to_string(PackingStatus::Feasible)), PackingStatus::Feasible);
    EXPECT_EQ(packing_status_from_string(to_string(PackingStatus::Partial)), PackingStatus::Partial);
    EXPECT_EQ(packing_status_from_string(to_string(PackingStatus::Infeasible)), PackingStatus::Infeasible);
    EXPECT_EQ(packing_status_from_string(to_string(PackingStatus::Invalid)), PackingStatus::Invalid);
}
