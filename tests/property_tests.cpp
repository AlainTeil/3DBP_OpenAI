#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <random>
#include <string>
#include <vector>

#include "bp/packer.hpp"
#include "bp/validation.hpp"

#include "test_helpers.hpp"

using namespace bp;

namespace {

Instance random_instance(std::mt19937 &rng, int case_index) {
    std::uniform_int_distribution<int> box_count_dist(0, 14);
    std::uniform_int_distribution<int> small_dim_dist(1, 7);
    std::uniform_int_distribution<int> wide_dim_dist(1, 12);
    std::bernoulli_distribution this_side_up_dist(0.25);
    std::bernoulli_distribution no_stack_dist(0.20);
    std::bernoulli_distribution wide_box_dist(0.20);

    Instance instance;
    instance.bins.push_back(Bin{.id = "bin-a-" + std::to_string(case_index), .size = Vec3{10, 10, 10}});
    instance.bins.push_back(Bin{.id = "bin-b-" + std::to_string(case_index), .size = Vec3{8, 12, 9}});

    const int box_count = box_count_dist(rng);
    for (int box_index = 0; box_index < box_count; ++box_index) {
        auto &dim_dist = wide_box_dist(rng) ? wide_dim_dist : small_dim_dist;
        instance.boxes.push_back(Box{.id = "box-" + std::to_string(case_index) + "-" + std::to_string(box_index),
                                     .size = Vec3{dim_dist(rng), dim_dist(rng), dim_dist(rng)},
                                     .this_side_up = this_side_up_dist(rng),
                                     .no_stack = no_stack_dist(rng)});
    }
    return instance;
}

void expect_status_matches_placements(const Instance &instance, const PackingResult &result) {
    const auto expected_unplaced = unplaced_box_ids(instance, result);
    EXPECT_EQ(result.unplaced_box_ids, expected_unplaced);
    EXPECT_EQ(result.feasible, result.status == PackingStatus::Feasible);
    EXPECT_EQ(result.stats.boxes_total, static_cast<int>(instance.boxes.size()));
    EXPECT_EQ(result.stats.boxes_placed, static_cast<int>(result.placements.size()));
    EXPECT_EQ(result.stats.boxes_unplaced, static_cast<int>(expected_unplaced.size()));

    if (expected_unplaced.empty()) {
        EXPECT_EQ(result.status, PackingStatus::Feasible);
    } else if (result.placements.empty()) {
        EXPECT_EQ(result.status, PackingStatus::Infeasible);
    } else {
        EXPECT_EQ(result.status, PackingStatus::Partial);
    }
}

} // namespace

TEST(PropertyPacking, RandomSmallInstancesAlwaysProduceValidatedResults) {
    const std::vector<AlgorithmId> algorithms = {
        AlgorithmId::FFD,        AlgorithmId::NFDH,         AlgorithmId::Layered,
        AlgorithmId::Guillotine, AlgorithmId::MaximalSpace, AlgorithmId::MetaGA,
        AlgorithmId::MetaGRASP,  AlgorithmId::MetaSA,       AlgorithmId::OnlineFFD};

    std::mt19937 rng(20260513u);
    for (int case_index = 0; case_index < 24; ++case_index) {
        const auto instance = random_instance(rng, case_index);
        ASSERT_TRUE(validate_instance(instance).valid);

        for (const auto algorithm : algorithms) {
            SCOPED_TRACE(::testing::Message() << "case=" << case_index << " algorithm=" << static_cast<int>(algorithm));
            const auto seed = static_cast<unsigned int>(1000 + case_index);
            const auto result = pack(instance, PackOptions{algorithm, seed, 8});
            expect_valid_packing(instance, result);
            expect_status_matches_placements(instance, result);
        }
    }
}

TEST(PropertyPacking, RandomNoStackHeavyInstancesStayValid) {
    std::mt19937 rng(424242u);
    std::uniform_int_distribution<int> dim_dist(2, 5);

    for (int case_index = 0; case_index < 16; ++case_index) {
        Instance instance;
        instance.bins.push_back(Bin{.id = "bin-0", .size = Vec3{12, 12, 8}});
        for (int box_index = 0; box_index < 10; ++box_index) {
            instance.boxes.push_back(Box{.id = "box-" + std::to_string(case_index) + "-" + std::to_string(box_index),
                                         .size = Vec3{dim_dist(rng), dim_dist(rng), dim_dist(rng)},
                                         .this_side_up = box_index % 3 == 0,
                                         .no_stack = box_index % 2 == 0});
        }

        SCOPED_TRACE(::testing::Message() << "case=" << case_index);
        const auto result =
            pack(instance, PackOptions{AlgorithmId::Guillotine, static_cast<unsigned int>(case_index), 8});
        expect_valid_packing(instance, result);
        expect_status_matches_placements(instance, result);
    }
}