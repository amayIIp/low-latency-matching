#!/bin/bash
# Tell the shell to exit immediately if any command fails (returns a non-zero exit status)
set -e

# Print a status message indicating the build process is starting
echo "Starting Clean Debug Build with Address and Undefined Behavior Sanitizers..."

# Define the path of the native build directory on the WSL2 filesystem
BUILD_DIR="/tmp/matching-engine-build"

# Remove any existing build directory to guarantee a completely clean configure/build cycle
rm -rf "$BUILD_DIR"
# Create the build directory
mkdir -p "$BUILD_DIR"

# Resolve the absolute path of the matching-engine source directory
SOURCE_DIR="$(pwd)/matching-engine"
# Resolve the absolute path of our local CMake binary
CMAKE_BIN="$(pwd)/tools/cmake-3.29.3-linux-x86_64/bin/cmake"

# Change directory into the build directory
cd "$BUILD_DIR"

# Run our local CMake to configure the project in Debug mode (enabling ASan/UBSan)
"$CMAKE_BIN" -DCMAKE_BUILD_TYPE=Debug "$SOURCE_DIR"

# Compile all targets using make, running on all available CPU cores to speed it up
make -j$(nproc)

# Resolve the absolute path of our local CTest binary
CTEST_BIN="$(dirname "$CMAKE_BIN")/ctest"

# Run the compiled test suite using CTest, printing failed logs verbosely
"$CTEST_BIN" --output-on-failure

# Print success completion message
echo "SUCCESS: Debug build and tests passed cleanly under memory sanitizers!"
