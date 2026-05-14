#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "bp/instance_io.hpp"
#include "bp/validation.hpp"

namespace {

#ifndef THREEDBP_GENERATE_EXE
#define THREEDBP_GENERATE_EXE "three_dbp_generate"
#endif

#ifndef THREEDBP_PACK_EXE
#define THREEDBP_PACK_EXE "three_dbp_pack"
#endif

struct CommandResult {
    int exit_code{0};
    std::string stdout_text;
    std::string stderr_text;
};

std::string read_text(const std::filesystem::path &path) {
    std::ifstream input(path);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

void write_text(const std::filesystem::path &path, const std::string &text) {
    std::ofstream output(path);
    output << text;
}

std::string shell_quote(const std::filesystem::path &path) {
    std::string quoted{"'"};
    for (const char ch : path.string()) {
        if (ch == '\'') {
            quoted += "'\\''";
        } else {
            quoted += ch;
        }
    }
    quoted += "'";
    return quoted;
}

std::filesystem::path test_dir(const std::string &name) {
    const auto dir = std::filesystem::temp_directory_path() / ("three_dbp_cli_golden_" + name);
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    return dir;
}

CommandResult run_command(const std::string &command, const std::filesystem::path &dir) {
    const auto stdout_path = dir / "stdout.txt";
    const auto stderr_path = dir / "stderr.txt";
    const std::string redirected = command + " > " + shell_quote(stdout_path) + " 2> " + shell_quote(stderr_path);
    const int raw_status = std::system(redirected.c_str());

    CommandResult result;
    if (raw_status == -1) {
        result.exit_code = -1;
    } else {
        result.exit_code = WEXITSTATUS(raw_status);
    }
    result.stdout_text = read_text(stdout_path);
    result.stderr_text = read_text(stderr_path);
    return result;
}

std::string command_with_args(const char *exe, const std::string &args) { return shell_quote(exe) + " " + args; }

} // namespace

TEST(CliGolden, GenerateWritesExpectedMessageAndLoadableInstance) {
    const auto dir = test_dir("generate");
    const auto instance_path = dir / "generated.json";

    const auto result = run_command(
        command_with_args(THREEDBP_GENERATE_EXE, "--boxes 4 --bins 2 --bin-size 12x12x12 --up 1 --nostack 1 "
                                                 "--seed 123 --output " +
                                                     shell_quote(instance_path)),
        dir);

    EXPECT_EQ(result.exit_code, 0) << result.stderr_text;
    EXPECT_EQ(result.stdout_text, "Wrote instance to " + instance_path.string() + "\n");
    EXPECT_TRUE(result.stderr_text.empty());

    const auto instance = bp::load_instance(instance_path.string());
    ASSERT_EQ(instance.bins.size(), 2u);
    ASSERT_EQ(instance.boxes.size(), 4u);
    EXPECT_EQ(instance.bins[0].id, "bin-0");
    EXPECT_EQ(instance.bins[0].size, (bp::Vec3{12, 12, 12}));
    for (const auto &box : instance.boxes) {
        EXPECT_TRUE(box.this_side_up);
        EXPECT_TRUE(box.no_stack);
    }
}

TEST(CliGolden, PackStdoutSummaryForFeasibleInstance) {
    const auto dir = test_dir("pack_stdout");
    const auto instance_path = dir / "instance.json";
    write_text(instance_path, R"({
        "bins": [{"id": "bin-0", "size": {"w": 10, "l": 10, "h": 10}}],
        "boxes": [
            {"id": "bottom", "size": {"w": 10, "l": 10, "h": 5}},
            {"id": "top", "size": {"w": 10, "l": 10, "h": 5}}
        ]
    })");

    const auto result = run_command(command_with_args(THREEDBP_PACK_EXE, "--input " + shell_quote(instance_path) +
                                                                             " --algo ffd --seed 7 --iterations 3"),
                                    dir);

    EXPECT_EQ(result.exit_code, 0) << result.stderr_text;
    EXPECT_EQ(result.stdout_text, "feasible\nbins_used: 1\nleftover_volume: 0\n");
    EXPECT_TRUE(result.stderr_text.empty());
}

TEST(CliGolden, PackWritesResultJsonAndCsvWithStableFields) {
    const auto dir = test_dir("pack_files");
    const auto instance_path = dir / "instance.json";
    const auto result_path = dir / "result.json";
    const auto csv_path = dir / "placements.csv";
    write_text(instance_path, R"({
        "bins": [{"id": "bin-0", "size": {"w": 10, "l": 10, "h": 10}}],
        "boxes": [
            {"id": "bottom", "size": {"w": 10, "l": 10, "h": 5}},
            {"id": "top", "size": {"w": 10, "l": 10, "h": 5}}
        ]
    })");

    const auto result = run_command(command_with_args(THREEDBP_PACK_EXE, "--input " + shell_quote(instance_path) +
                                                                             " --algo ffd --seed 7 --iterations 3 "
                                                                             "--output " +
                                                                             shell_quote(result_path) + " --csv " +
                                                                             shell_quote(csv_path)),
                                    dir);

    EXPECT_EQ(result.exit_code, 0) << result.stderr_text;
    EXPECT_TRUE(result.stdout_text.empty());
    EXPECT_TRUE(result.stderr_text.empty());

    const auto instance = bp::load_instance(instance_path.string());
    const auto packing = bp::load_result(result_path.string());
    EXPECT_EQ(packing.status, bp::PackingStatus::Feasible);
    EXPECT_TRUE(packing.feasible);
    EXPECT_EQ(packing.objective.bins_used, 1);
    EXPECT_EQ(packing.objective.leftover_volume, 0);
    EXPECT_EQ(packing.stats.boxes_total, 2);
    EXPECT_EQ(packing.stats.boxes_placed, 2);
    EXPECT_EQ(packing.stats.boxes_unplaced, 0);
    EXPECT_EQ(packing.metadata.algorithm, "ffd");
    EXPECT_EQ(packing.metadata.seed, 7u);
    EXPECT_EQ(packing.metadata.iterations, 3);
    EXPECT_EQ(packing.metadata.instance_file, instance_path.string());
    EXPECT_TRUE(packing.unplaced_box_ids.empty());
    EXPECT_TRUE(packing.validation_errors.empty());
    EXPECT_TRUE(bp::validate_result(instance, packing).valid);

    EXPECT_EQ(read_text(csv_path), "bin_id,box_id,x,y,z,w,l,h\n"
                                   "bin-0,bottom,0,0,0,10,10,5\n"
                                   "bin-0,top,0,0,5,10,10,5\n");
}

TEST(CliGolden, PackReturnsPartialExitCodeAndReportsUnplacedCount) {
    const auto dir = test_dir("pack_partial");
    const auto instance_path = dir / "instance.json";
    write_text(instance_path, R"({
        "bins": [{"id": "bin-0", "size": {"w": 5, "l": 5, "h": 5}}],
        "boxes": [
            {"id": "fits", "size": {"w": 5, "l": 5, "h": 5}},
            {"id": "too-big", "size": {"w": 6, "l": 4, "h": 4}}
        ]
    })");

    const auto result = run_command(command_with_args(THREEDBP_PACK_EXE, "--input " + shell_quote(instance_path) +
                                                                             " --algo ffd --seed 0 --iterations 1"),
                                    dir);

    EXPECT_EQ(result.exit_code, 2) << result.stderr_text;
    EXPECT_EQ(result.stdout_text, "partial\nbins_used: 1\nleftover_volume: 0\nunplaced_boxes: 1\n");
    EXPECT_TRUE(result.stderr_text.empty());
}

TEST(CliGolden, InvalidCliInputReturnsErrorExitCode) {
    const auto generate_dir = test_dir("generate_error");
    const auto generate_result = run_command(
        command_with_args(THREEDBP_GENERATE_EXE, "--boxes 1 --bins 1 --bin-size 10x10x10 --up 1.5"), generate_dir);
    EXPECT_EQ(generate_result.exit_code, 1);
    EXPECT_THAT(generate_result.stderr_text, ::testing::HasSubstr("Probabilities must be in the [0, 1] range"));

    const auto pack_dir = test_dir("pack_error");
    const auto instance_path = pack_dir / "instance.json";
    write_text(instance_path, R"({"bins": [], "boxes": []})");
    const auto pack_result = run_command(
        command_with_args(THREEDBP_PACK_EXE, "--input " + shell_quote(instance_path) + " --algo unknown"), pack_dir);
    EXPECT_EQ(pack_result.exit_code, 1);
    EXPECT_THAT(pack_result.stderr_text, ::testing::HasSubstr("Unknown algorithm: unknown"));
}