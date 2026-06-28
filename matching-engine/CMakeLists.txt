# Specify the minimum version of CMake required to build this project (we require 3.20 or newer)
cmake_minimum_required(VERSION 3.20)

# Define the project name, version, and set the language to C++
project(MatchingEngine VERSION 1.0.0 LANGUAGES CXX)

# Configure the C++ standard to be C++20 for modern features like concepts and standard formatting support
set(CMAKE_CXX_STANDARD 20)
# Force CMake to require C++20 standard; if the compiler doesn't support it, generation will fail
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Disable compiler-specific extensions (like gnu++20) to ensure the code remains standard-compliant and portable
set(CMAKE_CXX_EXTENSIONS OFF)

# Add a library target named 'lob_lib' compiled from the order book source code file
add_library(lob_lib src/order_book.cpp)

# Specify the directories where the compiler should search for header files when compiling the 'lob_lib' library
target_include_directories(lob_lib PUBLIC include)

# Add the main executable target named 'matching_engine' compiled from the main.cpp source file
add_executable(matching_engine src/main.cpp)
# Link the 'matching_engine' executable to our library 'lob_lib' so it can call OrderBook methods
target_link_libraries(matching_engine PRIVATE lob_lib)

# Add the benchmark executable target named 'bench_matching_engine' compiled from the benchmark source file
add_executable(bench_matching_engine bench/bench_main.cpp)
# Link the benchmark executable to our 'lob_lib' library so it can run performance checks on the OrderBook
target_link_libraries(bench_matching_engine PRIVATE lob_lib)

# Enable testing support in CMake so we can register and run our test suite using 'ctest'
enable_testing()
# Process the CMakeLists.txt in the tests/ directory to configure build targets for unit tests
add_subdirectory(tests)
