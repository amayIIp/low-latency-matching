# Low-Latency Limit Order Book (LOB) Matching Engine

A C++20 low-latency Limit Order Book (LOB) matching engine designed for high-frequency trading (HFT) performance.

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
