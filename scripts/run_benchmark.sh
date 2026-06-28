#!/bin/bash
# Tell the shell to exit immediately if any command fails (returns a non-zero exit status)
set -e

# Print status message indicating optimized build is starting
echo "Starting Clean Release Build (optimized for maximum throughput)..."

# Define the path of the native build directory on the WSL2 filesystem
BUILD_DIR="/tmp/matching-engine-build"

# Remove any existing build directory to guarantee a clean configure/build cycle
rm -rf "$BUILD_DIR"
# Create the build directory
mkdir -p "$BUILD_DIR"

# Resolve the absolute path of the matching-engine source directory
SOURCE_DIR="$(pwd)/matching-engine"
# Resolve the absolute path of our local CMake binary
CMAKE_BIN="$(pwd)/tools/cmake-3.29.3-linux-x86_64/bin/cmake"

# Change directory into the build directory
cd "$BUILD_DIR"

# Run our local CMake to configure the project in Release mode
"$CMAKE_BIN" -DCMAKE_BUILD_TYPE=Release "$SOURCE_DIR"

# Compile all targets using make, running on all available CPU cores
make -j$(nproc)

# Print status message indicating benchmark execution is starting
echo "Running the performance benchmark framework..."

# Execute the compiled benchmark binary to run Direct Call and Pipelined throughput tests
./bench_matching_engine

# Print success completion message
echo "SUCCESS: Benchmark completed successfully. Latency report saved to matching-engine/docs/latency_report.csv!"
