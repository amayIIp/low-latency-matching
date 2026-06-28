// Include the C standard assert library to perform runtime validation checks
#include <cassert>
// Include the standard input/output stream library to log test execution messages
#include <iostream>
// Include the order book header file to test the functionality of our LOB implementation
#include "lob/order_book.hpp"

// Define a test function to verify adding an order behaves as expected
void test_add_order() {
    // Log the start of this specific unit test
    std::cout << "Running test_add_order..." << std::endl;

    // Create an instance of the OrderBook to run our tests against
    lob::OrderBook book;

    // Construct a test buy order (ID 10, Price 500, Qty 20, Side Buy)
    lob::Order order(10, 500, 20, lob::Side::Buy);

    // Add the order to the book and capture the returned trades
    auto trades = book.add_order(order);

    // Assert that no trades were generated because there are no matching ask orders in the book
    assert(trades.empty());

    // Assert that the order book is not empty now that an order has been added
    assert(!book.empty());

    // Log successful completion of this test
    std::cout << "test_add_order PASSED" << std::endl;
}

// Define a test function to verify cancelling an order works correctly
void test_cancel_order() {
    // Log the start of this specific unit test
    std::cout << "Running test_cancel_order..." << std::endl;

    // Create an instance of the OrderBook to run our tests against
    lob::OrderBook book;

    // Construct a test sell order (ID 11, Price 510, Qty 15, Side Sell)
    lob::Order order(11, 510, 15, lob::Side::Sell);

    // Add the order to the book
    book.add_order(order);

    // Attempt to cancel the order using its ID (11) and store the boolean result
    bool cancel_result = book.cancel_order(11);

    // Assert that the cancel function returned true, indicating the order was found and removed
    assert(cancel_result == true);

    // Assert that the order book is completely empty now that the only order has been cancelled
    assert(book.empty());

    // Attempt to cancel the same order again to verify it fails safely
    bool cancel_again_result = book.cancel_order(11);

    // Assert that the duplicate cancellation returned false since the order no longer exists
    assert(cancel_again_result == false);

    // Log successful completion of this test
    std::cout << "test_cancel_order PASSED" << std::endl;
}

// Main execution entry point for running our unit tests
int main() {
    // Log that the test suite is beginning
    std::cout << "Starting matching-engine unit tests..." << std::endl;

    // Call the add order test function
    test_add_order();

    // Call the cancel order test function
    test_cancel_order();

    // Log that all tests successfully passed
    std::cout << "All tests passed successfully!" << std::endl;

    // Return 0 to indicate the test program completed without failures
    return 0;
}
