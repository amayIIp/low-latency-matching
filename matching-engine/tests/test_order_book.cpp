// Include the GoogleTest framework to support defining test cases and assertions
#include <gtest/gtest.h>
// Include the order book system implementation to run our tests against it
#include "lob/order_book.hpp"
// Include the standard random library to generate mock orders for differential testing
#include <random>
// Include vector to manage sequential collections of mock orders
#include <vector>

// Define a test case to verify behavior on an empty order book
TEST(OrderBookTests, EmptyBookFirstOrderRests) {
    // Construct an empty order book instance
    lob::OrderBook book;
    
    // Construct a Buy order with ID=1, Side=Buy, Price=100, Qty=50, and Timestamp=1
    lob::Order order(1, lob::Side::Buy, 100, 50, 1);
    
    // Add the order and collect returned trades
    auto trades = book.addOrder(order);
    
    // Assert that no trades were executed since the book was empty
    EXPECT_TRUE(trades.empty());
    
    // Assert that the book is no longer empty since the order should be resting
    EXPECT_FALSE(book.empty());
    
    // Assert that there is exactly 1 price level in bids_ map
    EXPECT_EQ(book.bids().size(), 1);
}

// Define a test case to verify exact price match behavior where both sides are fully filled
TEST(OrderBookTests, ExactPriceMatchFullFillBothSides) {
    // Construct an empty order book
    lob::OrderBook book;
    
    // Add a resting Buy order (ID=1, Side=Buy, Price=100, Qty=10, Timestamp=1)
    book.addOrder(lob::Order(1, lob::Side::Buy, 100, 10, 1));
    
    // Add an incoming Sell order matching exactly (ID=2, Side=Sell, Price=100, Qty=10, Timestamp=2)
    auto trades = book.addOrder(lob::Order(2, lob::Side::Sell, 100, 10, 2));
    
    // Assert that exactly 1 trade was generated
    ASSERT_EQ(trades.size(), 1);
    
    // Verify trade execution details: resting order (1) is maker, incoming order (2) is taker
    EXPECT_EQ(trades[0].maker_id, 1);
    EXPECT_EQ(trades[0].taker_id, 2);
    EXPECT_EQ(trades[0].price, 100);
    EXPECT_EQ(trades[0].qty, 10);
    
    // Assert that both orders were fully filled and the book is now empty
    EXPECT_TRUE(book.empty());
}

// Define a test case to verify partial fills when the incoming order is larger than the resting order
TEST(OrderBookTests, PartialFillIncomingLargerThanResting) {
    // Construct an empty order book
    lob::OrderBook book;
    
    // Add a resting Buy order (ID=1, Side=Buy, Price=100, Qty=10, Timestamp=1)
    book.addOrder(lob::Order(1, lob::Side::Buy, 100, 10, 1));
    
    // Add a larger incoming Sell order (ID=2, Side=Sell, Price=100, Qty=15, Timestamp=2)
    auto trades = book.addOrder(lob::Order(2, lob::Side::Sell, 100, 15, 2));
    
    // Assert that 1 trade execution was generated
    ASSERT_EQ(trades.size(), 1);
    
    // Verify that the trade filled the resting order's full size (10 units)
    EXPECT_EQ(trades[0].maker_id, 1);
    EXPECT_EQ(trades[0].taker_id, 2);
    EXPECT_EQ(trades[0].price, 100);
    EXPECT_EQ(trades[0].qty, 10);
    
    // Verify that the order book is not empty because the incoming Sell order has 5 units remaining
    EXPECT_FALSE(book.empty());
    
    // Verify that the bids side is now completely empty
    EXPECT_TRUE(book.bids().empty());
    
    // Verify that the asks side contains the remaining 5 units of the Sell order at price 100
    ASSERT_EQ(book.asks().size(), 1);
    EXPECT_EQ(book.asks().at(100).front().qty, 5);
}

// Define a test case to verify partial fills when the resting order is larger than the incoming order
TEST(OrderBookTests, PartialFillRestingLargerThanIncoming) {
    // Construct an empty order book
    lob::OrderBook book;
    
    // Add a resting Buy order (ID=1, Side=Buy, Price=100, Qty=15, Timestamp=1)
    book.addOrder(lob::Order(1, lob::Side::Buy, 100, 15, 1));
    
    // Add a smaller incoming Sell order (ID=2, Side=Sell, Price=100, Qty=10, Timestamp=2)
    auto trades = book.addOrder(lob::Order(2, lob::Side::Sell, 100, 10, 2));
    
    // Assert that 1 trade execution was generated
    ASSERT_EQ(trades.size(), 1);
    
    // Verify that the trade filled the incoming order's size (10 units)
    EXPECT_EQ(trades[0].maker_id, 1);
    EXPECT_EQ(trades[0].taker_id, 2);
    EXPECT_EQ(trades[0].price, 100);
    EXPECT_EQ(trades[0].qty, 10);
    
    // Verify that the book is not empty because the resting Buy order still has 5 units remaining
    EXPECT_FALSE(book.empty());
    
    // Verify that the asks side is completely empty
    EXPECT_TRUE(book.asks().empty());
    
    // Verify that the bids side contains the remaining 5 units of the Buy order at price 100
    ASSERT_EQ(book.bids().size(), 1);
    EXPECT_EQ(book.bids().at(100).front().qty, 5);
}

// Define a test case to verify an incoming order crossing multiple price levels in one transaction
TEST(OrderBookTests, OrderCrossesMultiplePriceLevels) {
    // Construct an empty order book
    lob::OrderBook book;
    
    // Rest two Sell orders at different price levels: ID=1 at price 101, ID=2 at price 102
    book.addOrder(lob::Order(1, lob::Side::Sell, 101, 5, 1));
    book.addOrder(lob::Order(2, lob::Side::Sell, 102, 5, 2));
    
    // Add an incoming Buy order covering both levels plus extra (ID=3, Side=Buy, Price=103, Qty=12, Timestamp=3)
    auto trades = book.addOrder(lob::Order(3, lob::Side::Buy, 103, 12, 3));
    
    // Assert that exactly 2 trade executions occurred
    ASSERT_EQ(trades.size(), 2);
    
    // Verify the first trade matched the lowest Ask (maker 1 at price 101 for 5 units)
    EXPECT_EQ(trades[0].maker_id, 1);
    EXPECT_EQ(trades[0].taker_id, 3);
    EXPECT_EQ(trades[0].price, 101);
    EXPECT_EQ(trades[0].qty, 5);
    
    // Verify the second trade matched the next lowest Ask (maker 2 at price 102 for 5 units)
    EXPECT_EQ(trades[1].maker_id, 2);
    EXPECT_EQ(trades[1].taker_id, 3);
    EXPECT_EQ(trades[1].price, 102);
    EXPECT_EQ(trades[1].qty, 5);
    
    // Verify the remaining 2 units of the incoming Buy order rest at price 103 on the bids side
    EXPECT_FALSE(book.empty());
    EXPECT_TRUE(book.asks().empty());
    ASSERT_EQ(book.bids().size(), 1);
    EXPECT_EQ(book.bids().at(103).front().qty, 2);
}

// Define a test case to verify that cancelling a resting order works correctly
TEST(OrderBookTests, CancelRestingOrder) {
    // Construct an empty order book
    lob::OrderBook book;
    
    // Add a resting Sell order
    book.addOrder(lob::Order(1, lob::Side::Sell, 105, 10, 1));
    
    // Cancel the order using its ID
    bool success = book.cancelOrder(1);
    
    // Assert that cancelOrder returned true, indicating success
    EXPECT_TRUE(success);
    
    // Assert that the order book is now completely empty
    EXPECT_TRUE(book.empty());
}

// Define a test case to verify that cancelling a non-existent order fails gracefully
TEST(OrderBookTests, CancelNonExistentOrder) {
    // Construct an empty order book
    lob::OrderBook book;
    
    // Attempt to cancel an order ID that was never added
    bool success = book.cancelOrder(999);
    
    // Assert that cancelOrder returned false, showing it did not find anything to cancel
    EXPECT_FALSE(success);
}

// Define a test case to verify FIFO / time-priority execution rules
TEST(OrderBookTests, VerifyTimePriorityFIFO) {
    // Construct an empty order book
    lob::OrderBook book;
    
    // Add two Buy orders at the exact same price (100) but different timestamps
    // First order (ID=1, Qty=10, Timestamp=1)
    book.addOrder(lob::Order(1, lob::Side::Buy, 100, 10, 1));
    // Second order (ID=2, Qty=10, Timestamp=2)
    book.addOrder(lob::Order(2, lob::Side::Buy, 100, 10, 2));
    
    // Add an incoming Sell order of quantity 15 at price 100 (ID=3, Timestamp=3)
    auto trades = book.addOrder(lob::Order(3, lob::Side::Sell, 100, 15, 3));
    
    // Assert that exactly 2 trade executions occurred
    ASSERT_EQ(trades.size(), 2);
    
    // Verify that the first trade matched the earliest order (ID 1) completely
    EXPECT_EQ(trades[0].maker_id, 1);
    EXPECT_EQ(trades[0].taker_id, 3);
    EXPECT_EQ(trades[0].price, 100);
    EXPECT_EQ(trades[0].qty, 10);
    
    // Verify that the second trade matched the next order in priority (ID 2) for the remaining 5 units
    EXPECT_EQ(trades[1].maker_id, 2);
    EXPECT_EQ(trades[1].taker_id, 3);
    EXPECT_EQ(trades[1].price, 100);
    EXPECT_EQ(trades[1].qty, 5);
    
    // Verify that the first Buy order (ID 1) is completely gone and the second (ID 2) has 5 units remaining
    EXPECT_FALSE(book.empty());
    ASSERT_EQ(book.bids().at(100).size(), 1);
    EXPECT_EQ(book.bids().at(100).front().id, 2);
    EXPECT_EQ(book.bids().at(100).front().qty, 5);
}

// Helper function to generate a sequence of N randomized orders with a deterministic seed
std::vector<lob::Order> generateRandomOrders(uint32_t seed, size_t num_orders) {
    // Initialize standard Merseene Twister RNG engine with our seed
    std::mt19937 rng(seed);
    
    // Define a distribution to select Buy (0) or Sell (1) with equal probability
    std::uniform_int_distribution<int> side_dist(0, 1);
    
    // Define a realistic price distribution between 9000 and 11000 ticks
    std::uniform_int_distribution<lob::Price> price_dist(9000, 11000);
    
    // Define a quantity distribution between 1 and 1000 lots
    std::uniform_int_distribution<lob::Qty> qty_dist(1, 1000);

    // Vector to store our mock orders
    std::vector<lob::Order> orders;
    orders.reserve(num_orders);
    
    // Set initial sequence timestamp
    uint64_t timestamp = 0;
    
    // Loop to construct N randomized orders
    for (size_t index = 0; index < num_orders; ++index) {
        // Sample a side from distribution
        lob::Side side = (side_dist(rng) == 0) ? lob::Side::Buy : lob::Side::Sell;
        
        // Construct and insert the order into the vector
        orders.emplace_back(index + 1, side, price_dist(rng), qty_dist(rng), ++timestamp);
    }
    
    // Return the generated list of orders
    return orders;
}

// Define the differential test case executing 1,000,000 randomized orders against two engines
TEST(DifferentialTest, RandomSequenceMatchesReference) {
    // Explicitly define our RNG seed for reproducibility
    uint32_t seed = 987654;
    
    // Create reference engine (acts as ground truth)
    lob::OrderBook reference_engine;
    
    // Create engine under test (to verify against reference engine)
    lob::OrderBook engine_under_test;

    // Number of random orders to run in our test
    const size_t num_orders = 1000000;
    
    // Generate the deterministic order sequence
    auto orders = generateRandomOrders(seed, num_orders);
    
    // Log the configuration details
    std::cout << "Starting differential test with seed: " << seed << " and " << num_orders << " orders." << std::endl;

    // Loop through each generated order and process them in both engines
    for (const auto& order : orders) {
        // Add the order to the reference engine
        auto trades_ref = reference_engine.addOrder(order);
        
        // Add the order to the engine under test
        auto trades_test = engine_under_test.addOrder(order);
        
        // Assert that the trades generated match exactly in quantity, price, and participants
        if (trades_ref != trades_test) {
            // Log details and fail the test if there is a mismatch
            FAIL() << "Trades mismatch with seed " << seed << " on order ID: " << order.id;
        }
    }
    
    // Assert that the final state of the bids map is identical in both books
    if (reference_engine.bids() != engine_under_test.bids()) {
        FAIL() << "Bids map state mismatch with seed " << seed;
    }
    
    // Assert that the final state of the asks map is identical in both books
    if (reference_engine.asks() != engine_under_test.asks()) {
        FAIL() << "Asks map state mismatch with seed " << seed;
    }
}
