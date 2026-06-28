# Create an executable target named 'test_order_book' from the test source file
add_executable(test_order_book test_order_book.cpp)

# Link the test executable to our main library 'lob_lib' so it can access the OrderBook class and test its functionality
target_link_libraries(test_order_book PRIVATE lob_lib)

# Specify the include directories for the test executable, pulling in headers from the parent directory's include folder
target_include_directories(test_order_book PRIVATE ../include)

# Register the compiled test executable with CMake's CTest utility so it runs during test execution
add_test(NAME OrderBookTests COMMAND test_order_book)
