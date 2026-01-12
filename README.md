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

## Tests
```bash
ctest --test-dir build --output-on-failure
```

## CLI Usage
Two executables are produced in `build/`:
- `three_dbp_generate` – random instance generator
- `three_dbp_pack` – run a packing algorithm on a JSON instance

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
- `--up p` probability a box is "this side up"
- `--nostack p` probability a box is "do not stack"
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
- `--iterations` is used by metaheuristics (GA/GRASP/SA).
- If `--output` is omitted, a summary is printed to stdout.
- `--verbose 1` adds stats/metadata; `--verbose 2` also prints per-bin layout.
- `--csv` writes placements as `bin_id,box_id,x,y,z,w,l,h`.

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
  "feasible": true,
  "objective": {"bins_used": 1, "leftover_volume": 1234},
  "stats": {"boxes_total": 1, "boxes_placed": 1, "volume_boxes": 6000, "volume_packed": 6000,
             "volume_bins_used": 1000000, "fill_ratio": 0.006},
  "metadata": {"algorithm": "ffd", "seed": 42, "iterations": 200, "instance_file": "instance.json",
                "timestamp": "2025-01-06T12:00:00Z"},
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

## Algorithms
- Offline heuristics: First-Fit Decreasing (FFD), Next-Fit Decreasing Height (NFDH), layered maximal space, guillotine FFD, maximal-space FFD.
- Metaheuristics: GA, GRASP, simulated annealing (permutation-based search over orderings).
- Online: 3D First-Fit (stream order, no reordering).

## Constraints and assumptions
- Integer dimensions only; 90-degree rotations. `this_side_up` fixes the height axis; `no_stack` forbids placing boxes above that box.
- Objective: minimize bins used, then leftover volume within used bins.

## Strengths
- Multiple heuristics (offline, online, meta) under one interface and CLI.
- Dependency-light: uses fmt, nlohmann/json, GTest/GMock via FetchContent.
- JSON I/O for easy integration; simple CLI for generation and packing.
- Tests covering core geometry, I/O, edge cases, and pipeline flows.

## Weaknesses / limitations
- Constructive heuristics; no exact/ILP/CP solver for optimality.
- No advanced stability/physics or complex stacking rules beyond `this_side_up` and `no_stack`.
- Metaheuristics are simple (small populations, basic operators); solution quality may lag state-of-the-art for large/irregular instances.
- No GPU acceleration or parallel search; performance tuned for small/medium instances.

## Formatting and analysis
- clang-format: see .clang-format
- clang-tidy: see .clang-tidy
- Doxygen: `cmake --build build --target docs` (if Doxygen is available)
