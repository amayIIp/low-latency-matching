// Include the GoogleTest framework headers to use test macros like TEST, EXPECT_EQ, and ASSERT_TRUE
#include <gtest/gtest.h>
// Include the order book header file so we can run tests against our matching engine
#include "lob/order_book.hpp"

// Define a simple dummy test to verify that the test runner, compiler, and build system are fully functional
TEST(OrderBookTests, DummyTest) {
    // Assert that the sum of 1 and 1 equals 2 using GoogleTest's EXPECT_EQ macro
    EXPECT_EQ(1 + 1, 2);
}

// Define a test case to verify that adding an order to the book works correctly without matches
TEST(OrderBookTests, TestAddOrder) {
    // Create an instance of the OrderBook to test
    lob::OrderBook book;

    // Construct a sample buy order with ID 10, Side Buy, Price 500, quantity 20, and timestamp 1
    lob::Order order(10, lob::Side::Buy, 500, 20, 1);

    // Add the order to the book and capture any returned trade executions
    auto trades = book.addOrder(order);

    // Verify that the trades list is empty since there are no matching ask orders in the book
    EXPECT_TRUE(trades.empty());

    // Verify that the book is not empty now that an order has been successfully added
    EXPECT_FALSE(book.empty());
}

// Define a test case to verify that cancelling an order works correctly
TEST(OrderBookTests, TestCancelOrder) {
    // Create an instance of the OrderBook to test
    lob::OrderBook book;

    // Construct a sample sell order with ID 11, Side Sell, Price 510, quantity 15, and timestamp 2
    lob::Order order(11, lob::Side::Sell, 510, 15, 2);

    // Add the order to the book
    book.addOrder(order);

    // Cancel the order using its ID (11) and store the status result
    bool cancel_result = book.cancelOrder(11);

    // Verify that the cancellation succeeded (returned true)
    EXPECT_TRUE(cancel_result);

    // Verify that the order book is empty now that the order has been removed
    EXPECT_TRUE(book.empty());

    // Attempt to cancel the same order again to verify it handles duplicate cancels safely
    bool cancel_again_result = book.cancelOrder(11);

    // Verify that the second cancellation attempt failed (returned false)
    EXPECT_FALSE(cancel_again_result);
}
