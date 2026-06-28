# Limit Order Book (LOB) Matching Engine Benchmarks

This document records the latency and throughput baseline benchmarks of the matching engine.

## System Configuration & Environment

* **Date**: 2026-06-28
* **Hardware**:
  * **CPU**: 13th Gen Intel(R) Core(TM) i5-13450HX
  * **Cores**: 8 Cores, 16 Threads
  * **Architecture**: x86_64
* **OS / Kernel**: Ubuntu Linux on WSL2 (Kernel version: `6.18.33.2-microsoft-standard-WSL2`)
* **Compiler**: GNU C++ Compiler version `15.2.0`
* **Compilation Flags**: `-std=c++20 -O3 -march=native -DNDEBUG -DPROVE_ZERO_ALLOC`

---

## Benchmarking Methodology

1. **Warmup Phase**: The first 10,000 orders are processed through the engine to warm CPU L1/L2 caches, configure the branch predictor, and populate the object pools. Warmup samples are excluded from the latency reports.
2. **Measurement Phase**: Latency is measured per-order using `std::chrono::high_resolution_clock` inside the hot-path timing loop.
3. **Zero-Allocation Enforcement**: A global `operator new`/`operator delete` check is active (`PROVE_ZERO_ALLOC`). Any dynamic memory allocation during the timed loop results in an immediate crash.
4. **Throughput Run Consistency**: Wall-clock duration is measured over 5 consecutive runs of 1,000,000 orders each to verify consistency and guard against CPU throttle noise.

---

## Baseline Performance Results (Phase 4)

### Throughput (1,000,000 Orders)
* **Average Throughput**: **14,804,200 orders/second**
* **Minimum Throughput**: 11,876,300 orders/second
* **Maximum Throughput**: 16,887,000 orders/second

### Latency Percentiles
* **50th Percentile (Median)**: **60 ns**
* **99th Percentile**: **330 ns**
* **99.9th Percentile**: **1,293 ns**

*Detailed histogram outputs are saved in `docs/latency_report.csv`.*

---

## Reproduction Commands

Run the following commands in the WSL2 shell to reproduce the benchmark results:

```bash
# Navigate to the native build directory
cd /tmp/matching-engine-build

# Clean and re-configure CMake in Release mode
../../mnt/d/\Low\ Latency\ Matching\ Engine/tools/cmake-3.29.3-linux-x86_64/bin/cmake -DCMAKE_BUILD_TYPE=Release "/mnt/d/Low Latency Matching Engine/matching-engine"

# Compile the target binaries
make -j$(nproc)

# Run the benchmark utility
./bench_matching_engine
```
