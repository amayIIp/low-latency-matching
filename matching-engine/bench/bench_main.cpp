// Include the standard input/output library to print benchmark statistics to the console
#include <iostream>
// Include the chrono library to access high-resolution clocks for measuring elapsed time in nanoseconds/microseconds
#include <chrono>
// Include the vector library to construct lists of mock orders for testing
#include <vector>
// Include the order book header file to run performance tests against the matching engine
#include "lob/order_book.hpp"

// Main entry point for the benchmarking executable
int main() {
    // Print a message indicating the benchmark is starting
    std::cout << "Starting matching-engine benchmark..." << std::endl;

    // Define the total number of orders we will create and process in this test run
    const size_t num_orders = 1000000; // 1 million orders

    // Create an instance of the OrderBook to test
    lob::OrderBook book;

    // Pre-allocate a vector to store our test orders to avoid measuring memory allocation during the actual run
    std::vector<lob::Order> test_orders;
    // Request the vector reserve enough memory upfront for our 1 million orders to prevent runtime resizing
    test_orders.reserve(num_orders);

    // Loop from 0 up to num_orders - 1 to populate our test order list
    for (size_t index = 0; index < num_orders; ++index) {
        // Alternately create buy and sell orders using the modulo operator (even indices are Buy, odd are Sell)
        lob::Side side = (index % 2 == 0) ? lob::Side::Buy : lob::Side::Sell;
        
        // Define a base price and adjust it based on index to simulate various order book price levels
        lob::Price price = 100 + (index % 10);
        
        // Add the newly constructed order object to our vector (ID is index + 1, quantity is fixed at 10)
        test_orders.emplace_back(index + 1, price, 10, side);
    }

    // Print a status update indicating order generation is finished and timing is about to start
    std::cout << "Generated " << num_orders << " mock orders. Starting measurement..." << std::endl;

    // Record the current high-resolution clock timestamp before starting the benchmark loop
    auto start_time = std::chrono::high_resolution_clock::now();

    // Loop through each order in our pre-allocated vector
    for (const auto& order : test_orders) {
        // Insert the order into the book and run the skeleton matching code
        book.add_order(order);
    }

    // Record the high-resolution clock timestamp immediately after completing the benchmark loop
    auto end_time = std::chrono::high_resolution_clock::now();

    // Calculate the total duration in microseconds by subtracting the start time from the end time
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

    // Convert the duration to seconds as a floating-point number for easier human reading
    double duration_seconds = static_cast<double>(duration) / 1000000.0;

    // Calculate the throughput: the number of orders processed divided by the time in seconds
    double throughput = static_cast<double>(num_orders) / duration_seconds;

    // Print the elapsed time and calculated throughput to the console
    std::cout << "Processed " << num_orders << " orders in " << duration_seconds << " seconds." << std::endl;
    std::cout << "Throughput: " << throughput << " orders per second." << std::endl;

    // Return 0 to indicate the benchmark finished successfully
    return 0;
}
