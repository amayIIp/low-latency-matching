
#include <gtest/gtest.h>

#include "lob/order_book.hpp"

#include "lob/reference_order_book.hpp"

#include <random>

#include <vector>

TEST(OrderBookTests, EmptyBookFirstOrderRests) {

    lob::OrderBook book;

    lob::Order order(1, lob::Side::Buy, 10000, 50, 1);

    auto trades = book.addOrder(order);

    EXPECT_TRUE(trades.empty());

    EXPECT_FALSE(book.empty());

    EXPECT_EQ(book.bids().size(), 1);
}

TEST(OrderBookTests, ExactPriceMatchFullFillBothSides) {

    lob::OrderBook book;

    book.addOrder(lob::Order(1, lob::Side::Buy, 10000, 10, 1));

    auto trades = book.addOrder(lob::Order(2, lob::Side::Sell, 10000, 10, 2));

    ASSERT_EQ(trades.size(), 1);

    EXPECT_EQ(trades[0].maker_id, 1);
    EXPECT_EQ(trades[0].taker_id, 2);
    EXPECT_EQ(trades[0].price, 10000);
    EXPECT_EQ(trades[0].qty, 10);

    EXPECT_TRUE(book.empty());
}

TEST(OrderBookTests, PartialFillIncomingLargerThanResting) {

    lob::OrderBook book;

    book.addOrder(lob::Order(1, lob::Side::Buy, 10000, 10, 1));

    auto trades = book.addOrder(lob::Order(2, lob::Side::Sell, 10000, 15, 2));

    ASSERT_EQ(trades.size(), 1);

    EXPECT_EQ(trades[0].maker_id, 1);
    EXPECT_EQ(trades[0].taker_id, 2);
    EXPECT_EQ(trades[0].price, 10000);
    EXPECT_EQ(trades[0].qty, 10);

    EXPECT_FALSE(book.empty());

    EXPECT_TRUE(book.bids().empty());

    ASSERT_EQ(book.asks().size(), 1);
    EXPECT_EQ(book.asks().at(10000).front().qty, 5);
}

TEST(OrderBookTests, PartialFillRestingLargerThanIncoming) {

    lob::OrderBook book;

    book.addOrder(lob::Order(1, lob::Side::Buy, 10000, 15, 1));

    auto trades = book.addOrder(lob::Order(2, lob::Side::Sell, 10000, 10, 2));

    ASSERT_EQ(trades.size(), 1);

    EXPECT_EQ(trades[0].maker_id, 1);
    EXPECT_EQ(trades[0].taker_id, 2);
    EXPECT_EQ(trades[0].price, 10000);
    EXPECT_EQ(trades[0].qty, 10);

    EXPECT_FALSE(book.empty());

    EXPECT_TRUE(book.asks().empty());

    ASSERT_EQ(book.bids().size(), 1);
    EXPECT_EQ(book.bids().at(10000).front().qty, 5);
}

TEST(OrderBookTests, OrderCrossesMultiplePriceLevels) {

    lob::OrderBook book;

    book.addOrder(lob::Order(1, lob::Side::Sell, 10001, 5, 1));
    book.addOrder(lob::Order(2, lob::Side::Sell, 10002, 5, 2));

    auto trades = book.addOrder(lob::Order(3, lob::Side::Buy, 10003, 12, 3));

    ASSERT_EQ(trades.size(), 2);

    EXPECT_EQ(trades[0].maker_id, 1);
    EXPECT_EQ(trades[0].taker_id, 3);
    EXPECT_EQ(trades[0].price, 10001);
    EXPECT_EQ(trades[0].qty, 5);

    EXPECT_EQ(trades[1].maker_id, 2);
    EXPECT_EQ(trades[1].taker_id, 3);
    EXPECT_EQ(trades[1].price, 10002);
    EXPECT_EQ(trades[1].qty, 5);

    EXPECT_FALSE(book.empty());
    EXPECT_TRUE(book.asks().empty());
    ASSERT_EQ(book.bids().size(), 1);
    EXPECT_EQ(book.bids().at(10003).front().qty, 2);
}

TEST(OrderBookTests, CancelRestingOrder) {

    lob::OrderBook book;

    book.addOrder(lob::Order(1, lob::Side::Sell, 10005, 10, 1));

    bool success = book.cancelOrder(1);

    EXPECT_TRUE(success);

    EXPECT_TRUE(book.empty());
}

TEST(OrderBookTests, CancelNonExistentOrder) {

    lob::OrderBook book;

    bool success = book.cancelOrder(999);

    EXPECT_FALSE(success);
}

TEST(OrderBookTests, VerifyTimePriorityFIFO) {

    lob::OrderBook book;

    book.addOrder(lob::Order(1, lob::Side::Buy, 10000, 10, 1));

    book.addOrder(lob::Order(2, lob::Side::Buy, 10000, 10, 2));

    auto trades = book.addOrder(lob::Order(3, lob::Side::Sell, 10000, 15, 3));

    ASSERT_EQ(trades.size(), 2);

    EXPECT_EQ(trades[0].maker_id, 1);
    EXPECT_EQ(trades[0].taker_id, 3);
    EXPECT_EQ(trades[0].price, 10000);
    EXPECT_EQ(trades[0].qty, 10);

    EXPECT_EQ(trades[1].maker_id, 2);
    EXPECT_EQ(trades[1].taker_id, 3);
    EXPECT_EQ(trades[1].price, 10000);
    EXPECT_EQ(trades[1].qty, 5);

    EXPECT_FALSE(book.empty());
    ASSERT_EQ(book.bids().at(10000).size(), 1);
    EXPECT_EQ(book.bids().at(10000).front().id, 2);
    EXPECT_EQ(book.bids().at(10000).front().qty, 5);
}

std::vector<lob::Order> generateRandomOrders(uint32_t seed, size_t num_orders) {

    std::mt19937 rng(seed);

    std::uniform_int_distribution<int> side_dist(0, 1);

    std::uniform_int_distribution<lob::Price> price_dist(9000, 11000);

    std::uniform_int_distribution<lob::Qty> qty_dist(1, 1000);

    std::vector<lob::Order> orders;
    orders.reserve(num_orders);

    uint64_t timestamp = 0;

    for (size_t index = 0; index < num_orders; ++index) {

        lob::Side side = (side_dist(rng) == 0) ? lob::Side::Buy : lob::Side::Sell;

        orders.emplace_back(index + 1, side, price_dist(rng), qty_dist(rng), ++timestamp);
    }

    return orders;
}

TEST(DifferentialTest, RandomSequenceMatchesReference) {

    uint32_t seed = 987654;

    lob::ReferenceOrderBook reference_engine;

    lob::OrderBook engine_under_test;

    const size_t num_orders = 1000000;

    auto orders = generateRandomOrders(seed, num_orders);

    std::cout << "Starting differential test with seed: " << seed << " and " << num_orders << " orders." << std::endl;

    for (const auto& order : orders) {

        auto trades_ref = reference_engine.addOrder(order);

        auto trades_test = engine_under_test.addOrder(order);

        if (trades_ref != trades_test) {

            FAIL() << "Trades mismatch with seed " << seed << " on order ID: " << order.id;
        }
    }

    auto ref_bids = reference_engine.bids();
    auto test_bids = engine_under_test.bids();
    auto ref_asks = reference_engine.asks();
    auto test_asks = engine_under_test.asks();

    if (ref_bids != test_bids) {
        FAIL() << "Bids map state mismatch with seed " << seed;
    }

    if (ref_asks != test_asks) {
        FAIL() << "Asks map state mismatch with seed " << seed;
    }
}
