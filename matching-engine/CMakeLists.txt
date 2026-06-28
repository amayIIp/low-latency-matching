# Specify the minimum CMake version required for configuring this project (3.20+)
cmake_minimum_required(VERSION 3.20)

# Set the project name, its version, and declare C++ as the programming language used
project(MatchingEngine VERSION 1.0.0 LANGUAGES CXX)

# Force the C++ standard to C++20 for all targets in this project
set(CMAKE_CXX_STANDARD 20)
# Make C++20 support mandatory so the configuration stops if the compiler is too old
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Disable compiler-specific extensions (like GNU extensions) to maintain standard compliance
set(CMAKE_CXX_EXTENSIONS OFF)

# Check if a build type is specified; if not, default to 'Release' for optimal performance
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  # Set the default build type to Release in the CMake cache
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

# Define compile options and definitions depending on the selected build type
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  # Log that the Release configuration is active
  message(STATUS "Configuring in Release mode: optimized for maximum throughput")
  # Add compile flags for Release mode: O3 optimizations, native CPU features, and disable assertions
  add_compile_options(-O3 -march=native)
  # Define NDEBUG globally to turn off assert statements for speed
  add_compile_definitions(NDEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
  # Log that the Debug configuration with sanitizers is active
  message(STATUS "Configuring in Debug mode: compiling with Address and Undefined Behavior sanitizers")
  # Add compile flags for Debug: include debug info, disable optimizations, enable ASan and UBSan, keep frame pointers
  add_compile_options(-g -O0 -fsanitize=address,undefined -fno-omit-frame-pointer)
  # Link the sanitizer libraries to ensure run-time check crashes are caught
  add_link_options(-fsanitize=address,undefined)
elseif(CMAKE_BUILD_TYPE STREQUAL "TSan")
  # Log that the ThreadSanitizer configuration is active
  message(STATUS "Configuring in TSan mode: compiling with Thread Sanitizer to detect race conditions")
  # Add compile flags for TSan: include debug info, minor optimizations (O1 required by TSan), enable thread sanitizer, keep frame pointers
  add_compile_options(-g -O1 -fsanitize=thread -fno-omit-frame-pointer)
  # Link the ThreadSanitizer libraries for running tests under TSan
  add_link_options(-fsanitize=thread)
endif()

# Include the FetchContent module to download dependencies at configure-time
include(FetchContent)

# Declare GoogleTest dependency details: download URL pointing to version 1.14.0 release
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)

# Enable testing support so standard test runner (ctest) is aware of target executables
enable_testing()

# Make the declared googletest dependency available to compile and link against
FetchContent_MakeAvailable(googletest)

# Define our static library target 'lob_lib' containing the implementation of our limit order book
add_library(lob_lib src/order_book.cpp)

# Specify the include directories for the 'lob_lib' target. PUBLIC makes it accessible to libraries linking to it
target_include_directories(lob_lib PUBLIC include)

# Add the main application executable compiling main.cpp
add_executable(matching_engine src/main.cpp)
# Link the main executable with our limit order book library
target_link_libraries(matching_engine PRIVATE lob_lib)

# Add the benchmark executable compiling bench_main.cpp
add_executable(bench_matching_engine bench/bench_main.cpp)
# Link the benchmark executable with our limit order book library
target_link_libraries(bench_matching_engine PRIVATE lob_lib)

# Process the CMakeLists.txt within the tests subdirectory to configure our testing suite
add_subdirectory(tests)
