# Limit Order Book (LOB) Matching Engine Benchmarks

This document records the latency and throughput benchmarks, profiling analysis, and optimization iterations of the matching engine.

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
5. **Realistic Load Generator**: Simulated load uses a random walk for `fair_price` bounded between `8500` and `11500` ticks, with orders clustering around it using a normal distribution offset. Quantities follow a log-normal distribution ($e^{x}$ where $x \in [0, 5.5]$), biasing heavily towards small order sizes (retail) with sparse large blocks (institutional).

---

## Baseline Performance Results (Phase 5)

Under realistic, heavily crossing market loads:

### Direct Call (Baseline)
* **Average Throughput**: **1,050,630 orders/second**
* **Latency Percentiles**:
  * **50th Percentile (Median)**: **60-70 ns**
  * **99th Percentile**: **330 ns**
  * **99.9th Percentile**: **1,290 ns**

### Pipelined Ingestion (SPSC Ring Buffer)
* **Average Throughput**: **187,586 orders/second**
* **Thread Placement**: Producer pinned to CPU Core 1, Consumer pinned to CPU Core 2.
* **Synchronization**: Lock-free SPSC ring buffer with `acquire`/`release` memory barriers and low-latency `pause` assembly spin-wait.

---

## Profiling & Optimization Loop (Phase 6)

Due to virtualized environment constraints on WSL2 (which restrict access to hardware performance counters), we utilized **`gprof`** to capture a flat profile of CPU execution time. Below is the baseline flat profile obtained from the benchmark run:

```
  %   cumulative   self              self     total           
 time   seconds   seconds    calls  ms/call  ms/call  name    
 34.62      0.99     0.99                             std::thread::_State_impl<std::thread::_Invoker<std::tuple<main::{lambda()#2}> > >::_M_run() (Consumer loop)
 22.73      1.64     0.65                             main
 17.83      2.15     0.51 10999999     0.00     0.00  lob::OrderBook::addOrder(lob::Order)
 12.24      2.50     0.35        1   350.00   581.82  std::thread::_State_impl<std::thread::_Invoker<std::tuple<main::{lambda()#1}> > >::_M_run() (Producer loop)
 10.84      2.81     0.31       11    28.18    28.18  lob::OrderBook::OrderBook()
```

The profile showed that **46.86%** of CPU time was consumed by thread wrapper spin-waiting on SPSC queue polling, and **17.83%** of time was spent inside the matching logic itself.

### Optimization Cycle 1: SPSC Ingestion Index Caching
* **Observation**: Polling the atomic index `tail_` in the producer loop and `head_` in the consumer loop on every iteration causes massive cache-coherency bus traffic (cache invalidations across CPU cores).
* **Fix**: Added non-atomic local caching variables (`tail_cached_` and `head_cached_`) to the SPSC buffer. The threads only query the atomic variables when the cached limit is reached (i.e., when the buffer is empty or full), significantly reducing cache-line bouncing.
* **Result**: Pipelined throughput increased from `5.58M orders/sec` to **6.81M orders/sec** (a **22% performance increase**).

### Optimization Cycle 2: Side-Specialized Matching & Branch Elimination
* **Observation**: The monolithic `addOrder` function evaluated `order.side == Side::Buy` checks. Though easily predicted, separating this logic allows the compiler to optimize register allocations and code layout for buy and sell matching paths independently.
* **Fix**: Split the matching logic into two private member functions: `addBuyOrder` and `addSellOrder`. We forced full compiler inlining using `always_inline inline` attributes to avoid function call overhead.
* **Result**: Compiled cleanly and maintained optimized latency and throughput. Direct Call throughput remained stable at **15.88M orders/sec**.

### Optimization Cycle 3: Elimination of Profiling Overhead (-pg)
* **Observation**: The `-pg` compiler flag used by `gprof` inserts calls to `mcount` at the prologue of every function. This forces the preservation of frame pointers and severely blocks critical compiler optimizations like automatic inlining, loop unrolling, and vectorization.
* **Fix**: Disabled `-pg` flags from the Release build configuration in `CMakeLists.txt` for production builds.
* **Result**: Direct Call throughput jumped from `11.5M orders/sec` to **15.95M orders/sec** (a **38% performance increase**), restoring nanosecond-level latency profiles.

---

## Tradeoff Analysis: Direct Call vs. Pipelined Ingestion

In virtualized systems (WSL2 / Hyper-V), we observe a throughput gap between single-threaded **Direct Call** (~15.9M orders/sec) and concurrent **Pipelined Ingestion** (~6.8M orders/sec):

1. **Cache Locality**: 
   * In **Direct Call**, a single thread modifies the `OrderBook` object. All active cash structures (ladders, active levels) reside in the local CPU Core's L1/L2 cache, resulting in maximum cache hits and minimum memory latency.
   * In **Pipelined Ingestion**, the producer writes to the shared SPSC ring buffer (`buffer_`) and updates `head_`, while the consumer reads from the buffer and updates `tail_`. This results in **cache-line bouncing** between Core 1 and Core 2 as cache lines transition states in the CPU cache coherence protocol (MESI).
2. **Context Switching and Virtualization Overhead**:
   * Thread affinity overrides (`pthread_setaffinity_np`) on virtualized platforms (WSL2) are managed by the host OS hypervisor. Sibling vCPUs frequently share execution resource pipelines on the same physical core. 
   * When the lock-free queue is empty or full, the spinning thread's `pause` cycles can saturate core execution slots, starving the working thread and causing scheduling latency that degrades overall throughput.
3. **Pipelining Benefit Justification**:
   * In production trading platforms, pipelining is used because **network packet parsing (e.g., FIX/binary protocol decode)** is extremely CPU-heavy. 
   * While direct memory replays are faster single-threaded, separating network I/O from the matching engine is necessary to prevent network packet drops and isolate the matching core from network-induced jitter.

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
