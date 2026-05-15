# 3D Bin Packing (C++20)

A C++20 library and CLI tools for 3D bin packing with multiple heuristics (offline and online), JSON I/O, and tests.

## Requirements
- CMake >= 3.20
- C++20 compiler (GCC or Clang)
- Git (for FetchContent dependencies)

## Build
```bash
cmake -S . -B build -DTHREEDBP_BUILD_TESTS=ON
cmake --build build
```

Preset-based builds are also available:
```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug

cmake --preset release
cmake --build --preset release
ctest --preset release
```

Sanitizer builds use the `asan-ubsan` preset:
```bash
cmake --preset asan-ubsan
cmake --build --preset asan-ubsan
ctest --preset asan-ubsan
```

## Tests
For preset builds, run the matching CTest preset:
```bash
ctest --preset debug
ctest --preset release
ctest --preset asan-ubsan
```

For the manual `cmake -S . -B build` layout, run:
```bash
ctest --test-dir build --output-on-failure
```

The test suite covers:
- Core model geometry and status conversion.
- Instance/result JSON round trips, malformed input rejection, legacy result compatibility, and unknown status rejection.
- Packer behavior across all algorithms, including adversarial `no_stack`, `this_side_up`, partial, and infeasible cases.
- Validation regression cases for duplicate placements, unknown IDs, out-of-bounds placements, illegal rotations, overlap, and stacking violations.
- Deterministic randomized property-style checks that generated packings remain valid and that status, stats, placements, and unplaced IDs stay consistent.
- CLI golden-output checks for stdout summaries, generated files, CSV output, benchmark reports, and exit codes.
- Installed-package smoke testing verifies the exported CMake package from an external consumer project.

## CLI Usage
Three executables are produced in the selected CMake build directory:
- `three_dbp_generate` – random instance generator
- `three_dbp_pack` – run a packing algorithm on a JSON instance
- `three_dbp_benchmark` – run all algorithms over a fixed benchmark corpus and emit a CSV report

### Generate instances
```bash
./three_dbp_generate \
  --boxes 50 --bins 3 --bin-size 100x100x100 \
  --up 0.2 --nostack 0.1 --seed 123 --output instance.json
```
Options:
- `--boxes N` number of boxes
- `--bins M` number of bins
- `--bin-size WxLxH` bin dimensions (integers)
- `--up p` probability a box is "this side up"; must be in `[0, 1]`
- `--nostack p` probability a box is "do not stack"; must be in `[0, 1]`
- `--seed N` RNG seed (0 => nondeterministic)
- `--output file` output JSON path

### Pack instances
```bash
./three_dbp_pack \
  --input instance.json \
  --algo ffd|nfdh|layer|guillotine|maxspace|meta-ga|meta-grasp|meta-sa|online-ffd \
  --seed 42 --iterations 200 --output result.json \
  --verbose 0|1|2 --csv placements.csv
```
Notes:
- Use `--list-algorithms` to print the supported algorithm names and a short description.
- `--iterations` is used by metaheuristics (GA/GRASP/SA).
- If `--output` is omitted, a summary is printed to stdout.
- `--verbose 1` adds stats/metadata; `--verbose 2` also prints per-bin layout.
- `--csv` writes placements as `bin_id,box_id,x,y,z,w,l,h`.
- Exit code is `0` for `feasible`, `2` for `partial`, `infeasible`, or `invalid`, and `1` for CLI/input/runtime errors.

### Benchmark corpus
```bash
./build/debug/three_dbp_benchmark --corpus benchmarks/corpus --seed 42 --iterations 50 --output benchmark.csv

cd build/asan-ubsan
./three_dbp_benchmark --corpus benchmarks/corpus --seed 42 --iterations 50 --output benchmark.csv
```
The benchmark runner loads each corpus instance, runs every algorithm, and writes report-only metrics:
`instance,algorithm,status,elapsed_ms,bins_used,boxes_total,boxes_placed,boxes_unplaced,fill_ratio,valid`.
The checked-in corpus currently includes dense layering, mixed constraint, and partial-pressure cases. Build-tree benchmark executables also resolve the checked-in corpus from the source root, so `--corpus benchmarks/corpus` works from the repository root or from a preset build directory such as `build/debug` or `build/asan-ubsan`. Benchmark results are intended for comparison and trend tracking; CI does not fail on runtime or packing-quality thresholds.

## JSON Format
Instance:
```json
{
  "bins": [
    {"id": "bin-0", "size": {"w": 100, "l": 100, "h": 100}}
  ],
  "boxes": [
    {"id": "box-0", "size": {"w": 10, "l": 20, "h": 30}, "this_side_up": false, "no_stack": true}
  ]
}
```
Result:
```json
{
  "status": "feasible",
  "feasible": true,
  "objective": {"bins_used": 1, "leftover_volume": 994000},
  "stats": {"boxes_total": 1, "boxes_placed": 1, "boxes_unplaced": 0,
             "volume_boxes": 6000, "volume_packed": 6000,
             "volume_bins_used": 1000000, "fill_ratio": 0.006},
  "metadata": {"algorithm": "ffd", "seed": 42, "iterations": 200, "instance_file": "instance.json",
                "timestamp": "2026-05-13T12:00:00Z"},
  "unplaced_box_ids": [],
  "validation_errors": [],
  "placements": [
    {
      "box_id": "box-0",
      "bin_id": "bin-0",
      "position": {"x": 0, "y": 0, "z": 0},
      "orientation": {"w": 10, "l": 20, "h": 30}
    }
  ]
}
```

Result status values are:
- `feasible`: all boxes were placed and the result passed invariant validation.
- `partial`: at least one box was placed, but one or more boxes are listed in `unplaced_box_ids`.
- `infeasible`: no boxes could be placed.
- `invalid`: the algorithm produced placements that failed validation; `validation_errors` explains why.

The `feasible` boolean is retained as a compatibility shortcut for `status == "feasible"`; new integrations should prefer `status` and `unplaced_box_ids`.

## Algorithms
All algorithms use the same public `pack()` result contract: they validate the input instance, report `feasible`, `partial`, `infeasible`, or `invalid`, populate `unplaced_box_ids`, and run final result validation before returning.

| CLI name | Mode | Determinism | Ordering/search | Iterations used | Notes |
| --- | --- | --- | --- | --- | --- |
| `ffd` | Offline | Deterministic | Greedy first-fit, boxes sorted by descending volume | No | General-purpose baseline heuristic. |
| `nfdh` | Offline | Deterministic | Greedy first-fit, boxes sorted by descending height then volume | No | Height-ordered heuristic rather than a full canonical NFDH layer implementation. |
| `layer` | Offline | Deterministic | Greedy first-fit, boxes sorted by descending height then base area | No | Layered ordering heuristic over the shared free-space packer. |
| `guillotine` | Offline | Deterministic | Greedy first-fit with disjoint guillotine-style residual spaces | No | Fast constructive heuristic that favors simple residual-space splits. |
| `maxspace` | Offline | Deterministic | Greedy first-fit, boxes sorted by largest side then volume | No | Maximal-space-inspired ordering heuristic, not an exact maximal-empty-space solver. |
| `meta-ga` | Offline | Stochastic | Genetic permutation search using order crossover and mutation over the greedy packer | Yes | Uses `--seed` and `--iterations`; better search coverage costs more runtime. |
| `meta-grasp` | Offline | Stochastic | Randomized greedy ordering search over the greedy packer | Yes | Uses `--seed` and `--iterations`; useful for sampling alternatives to fixed orderings. |
| `meta-sa` | Offline | Stochastic | Simulated annealing over box orderings | Yes | Uses `--seed` and `--iterations`; accepts some worse intermediate orderings during search. |
| `online-ffd` | Online | Deterministic | Greedy first-fit in input stream order with no reordering | No | Preserves input order for stream-like scenarios; usually less flexible than offline sorting/search. |

## Constraints and assumptions
- Integer dimensions only; 90-degree rotations. `this_side_up` fixes the height axis; `no_stack` forbids placing boxes above that box.
- Objective: minimize bins used, then leftover volume within used bins.
- Loaded instances are validated: bin and box IDs must be non-empty and unique, and all dimensions must be positive.
- Every result returned by `pack()` is validated for known IDs, duplicate boxes, bounds, legal rotations, `this_side_up`, overlap, and `no_stack` violations.
- `pack()` throws `std::runtime_error` for invalid instances. Use `validate_instance()` first if callers need to collect validation errors without running an algorithm.

## Library validation API
The public validation helpers live in `bp/validation.hpp`:
- `validate_instance(instance)` checks identifiers and dimensions.
- `validate_result(instance, result)` checks a result against the instance and packing invariants.
- `unplaced_box_ids(instance, result)` derives missing box IDs from placements.
- `can_place_with(candidate, box, existing, instance)` is used by packers to reject candidate placements that overlap or violate `no_stack`.

## CMake package usage
After installation, consumers can use:
```cmake
find_package(three_dbp CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE three_dbp::three_dbp)
```

The repository includes an installed-package smoke test in `tests/package_smoke` that exercises this flow from outside the main build tree:
```bash
cmake --install build/debug --prefix build/debug/install
cmake -S tests/package_smoke -B build/package-smoke -DCMAKE_PREFIX_PATH="$PWD/build/debug/install"
cmake --build build/package-smoke
./build/package-smoke/three_dbp_package_smoke
```

## Quality checks
The GitHub Actions workflow runs:
- Debug configure/build/test through the `debug` preset.
- Installed-package smoke testing through `tests/package_smoke`.
- clang-format check over `include`, `src`, `apps`, and `tests`.
- cppcheck over project sources and tests.
- ASan/UBSan configure/build/test through the `asan-ubsan` preset.

## Strengths
- Multiple heuristics (offline, online, meta) under one interface and CLI.
- Dependency-light: uses fmt, nlohmann/json, GTest/GMock via FetchContent.
- JSON I/O for easy integration; simple CLI for generation and packing.
- Tests covering core geometry, I/O regressions, adversarial constraints, randomized packing invariants, edge cases, CLI golden outputs, benchmark reports, and pipeline flows.
- Runtime validation prevents invalid placements from being reported as feasible.

## Weaknesses / limitations
- Constructive heuristics; no exact/ILP/CP solver for optimality.
- No advanced stability/physics or complex stacking rules beyond `this_side_up` and `no_stack`.
- Metaheuristics are simple (small populations, basic operators); solution quality may lag state-of-the-art for large/irregular instances.
- No GPU acceleration or parallel search; performance tuned for small/medium instances.
- Several algorithm IDs are heuristic variants over a shared greedy free-space packer rather than full textbook implementations.

## Formatting and analysis
- clang-format: see .clang-format
- clang-tidy: see .clang-tidy
- Doxygen: configure a docs-enabled build, then run `cmake --build build --target docs` (if Doxygen is available). The provided presets set `THREEDBP_BUILD_DOCS=OFF`.
