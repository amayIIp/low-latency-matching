# Declare the unit test executable compiled from the test_order_book.cpp and test_spsc_ring_buffer.cpp files
add_executable(test_order_book test_order_book.cpp test_spsc_ring_buffer.cpp)

# Link the test executable to our main LOB library and GoogleTest's gtest_main target which provides the main() entry point
target_link_libraries(test_order_book PRIVATE lob_lib gtest_main)

# Add the test target to CMake's CTest suite so it runs automatically when 'ctest' is invoked
add_test(NAME OrderBookTests COMMAND test_order_book)
