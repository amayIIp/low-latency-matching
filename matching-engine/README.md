# Low-Latency Limit Order Book (LOB) Matching Engine

**Status: Phase 3 complete**

A C++20 low-latency Limit Order Book (LOB) matching engine designed for high-frequency trading (HFT) performance. This project implements an ultra-fast order book that supports add, cancel, and modify operations with nanosecond-level execution latency. The system features a modular architecture, comprehensive unit tests using GoogleTest, and memory/thread safety sanitizers to ensure maximum reliability and correctness under extreme load. 

In Phase 3, the engine was rewritten for high-performance zero-allocation execution and O(1) time complexity across all hot-path operations (insert, cancel, match).

## Design Tradeoffs & Assumptions

### 1. Bounded Price Range (Array-indexed Ladder)
To replace the slow, tree-chasing `std::map` (O(log N)) with O(1) direct array indexing, we assume a realistic bounded price range for a trading session. The engine is configured with:
* `MIN_PRICE = 8000` ticks
* `MAX_PRICE = 12000` ticks

**Tradeoff**: Orders outside this range are rejected. However, this is a standard industry practice where price bands (e.g., LULD - Limit Up/Limit Down) are enforced at the exchange level. This array-indexed structure is extremely cache-friendly, avoiding tree pointer chasing and keeping contiguous memory structures.

### 2. Pre-allocated Object Pool
To avoid expensive OS runtime heap allocations (`new`/`malloc`), we pre-allocate a pool of `1,100,000` order structures in contiguous memory at startup. Acquire and release operations simply manipulate pointers on an intrusive free list, executing in constant time O(1).

### 3. Intrusive Doubly-Linked Lists
Each price level utilizes an intrusive doubly-linked list inside the pooled `Order` structures. This eliminates the node allocation overhead of `std::list` and enables O(1) order cancellations by unlinking orders directly from their active positions in the queue without scans.

### 4. Zero-Allocation Proof
We override global `operator new` and `operator delete`. When the `PROVE_ZERO_ALLOC` flag is enabled, calling `new` after the initialization phase triggers an immediate crash. The benchmark successfully processes 1,000,000 orders without a single runtime allocation.

## Project Structure

* **`include/lob/`**: Contains library headers (`types.hpp`, `order.hpp`, `order_book.hpp`).
* **`src/`**: Contains the matching engine core implementation (`order_book.cpp`) and application entry point (`main.cpp`).
* **`tests/`**: Contains unit tests verifying correctness.
* **`bench/`**: Contains high-performance throughput benchmarks.
* **`docs/`**: Holds markdown documents describing architecture and benchmark results.

## Building the Project

Since this repository is located on a Windows-mounted filesystem (`/mnt/d/...` under WSL), running CMake configurations directly within the mounted directory can result in `Operation not permitted` errors. To avoid this, configure and build the project in a native Linux directory (such as `/tmp/matching-engine-build` or `~/matching-engine-build`).

### Build Steps

Run the following commands in your shell to build:

```bash
# Create a build directory in the native Linux /tmp folder to bypass Windows mount issues
mkdir -p /tmp/matching-engine-build

# Navigate into the native build directory
cd /tmp/matching-engine-build

# Run CMake, pointing it to the matching-engine source directory on the mounted drive
/mnt/d/Low\ Latency\ Matching\ Engine/tools/cmake-3.29.3-linux-x86_64/bin/cmake "/mnt/d/Low Latency Matching Engine/matching-engine"

# Run the generated makefiles to compile all libraries and binaries
make
```

### Running the Executables

From the build directory (`/tmp/matching-engine-build`):

```bash
# Run the main matching engine demonstration executable
./matching_engine

# Run the unit tests to verify the order book functions correctly
./tests/test_order_book

# Run the benchmark executable to measure performance and throughput
./bench_matching_engine
```
