// Include the optimized order book header file.
#include "lob/order_book.hpp"
// Include algorithm for access to std::min.
#include <algorithm>

#ifdef PROVE_ZERO_ALLOC
// Include standard library utilities to support malloc/free and exit handling.
#include <cstdlib>
// Include standard stream to output diagnostic crash dumps.
#include <iostream>

namespace lob {
// Initialize the global atomic allocation permission flag to false (allocations allowed initially).
std::atomic<bool> allocations_forbidden{false};
}

// Global override of the C++ 'new' operator.
// If the allocation flag is set to true, any attempt to dynamically allocate memory will crash the engine.
void* operator new(size_t size) {
    // If dynamic allocations are forbidden...
    if (lob::allocations_forbidden.load(std::memory_order_relaxed)) {
        // Output a critical message to standard error.
        std::cerr << "CRITICAL ERROR: Dynamic memory allocation of " << size 
                  << " bytes attempted while allocations_forbidden is ACTIVE!\n";
        // Forcefully abort execution immediately to fail tests/runs.
        std::abort();
    }
    // Perform standard memory allocation via malloc.
    return std::malloc(size);
}

// Global override of the array 'new[]' operator.
void* operator new[](size_t size) {
    if (lob::allocations_forbidden.load(std::memory_order_relaxed)) {
        std::cerr << "CRITICAL ERROR: Dynamic memory allocation of " << size 
                  << " bytes attempted while allocations_forbidden is ACTIVE!\n";
        std::abort();
    }
    return std::malloc(size);
}

// Global override of the nothrow 'new' operator.
void* operator new(size_t size, const std::nothrow_t&) noexcept {
    if (lob::allocations_forbidden.load(std::memory_order_relaxed)) {
        std::cerr << "CRITICAL ERROR: Dynamic memory allocation of " << size 
                  << " bytes attempted while allocations_forbidden is ACTIVE!\n";
        std::abort();
    }
    return std::malloc(size);
}

// Global override of the nothrow array 'new[]' operator.
void* operator new[](size_t size, const std::nothrow_t&) noexcept {
    if (lob::allocations_forbidden.load(std::memory_order_relaxed)) {
        std::cerr << "CRITICAL ERROR: Dynamic memory allocation of " << size 
                  << " bytes attempted while allocations_forbidden is ACTIVE!\n";
        std::abort();
    }
    return std::malloc(size);
}

// Global override of the standard single-object 'delete' operator.
void operator delete(void* p) noexcept {
    // Free the memory block using free.
    std::free(p);
}

// Global override of the standard array 'delete[]' operator.
void operator delete[](void* p) noexcept {
    std::free(p);
}

// Global override of the sized single-object 'delete' operator.
void operator delete(void* p, size_t) noexcept {
    // Free the memory block using free.
    std::free(p);
}

// Global override of the sized array 'delete[]' operator.
void operator delete[](void* p, size_t) noexcept {
    std::free(p);
}

// Global override of the nothrow single-object 'delete' operator.
void operator delete(void* p, const std::nothrow_t&) noexcept {
    std::free(p);
}

// Global override of the nothrow array 'delete[]' operator.
void operator delete[](void* p, const std::nothrow_t&) noexcept {
    std::free(p);
}
#endif

// Place code inside the 'lob' namespace.
namespace lob {

// Implement the constructor for the optimized OrderBook.
OrderBook::OrderBook()
    // Initialize the order pool once with pre-allocated storage capacity.
    : pool_(POOL_CAPACITY),
      // Initialize vector size for bids array matching NUM_LEVELS slots.
      bids_levels_(NUM_LEVELS),
      // Initialize vector size for asks array matching NUM_LEVELS slots.
      asks_levels_(NUM_LEVELS),
      // Pre-allocate lookup array to accommodate order IDs up to 2,000,000 without runtime reallocation.
      order_index_(2000000, nullptr),
      // Set the initial best bid index to -1 (indicating empty bids book).
      best_bid_index_(-1),
      // Set the initial best ask index to NUM_LEVELS (indicating empty asks book).
      best_ask_index_(NUM_LEVELS) {
    // Body is empty since initialization lists set up all vectors and pool.
}

// Implement the destructor.
OrderBook::~OrderBook() {
    // No raw resources or pointers need manual release.
}

// Process and match an incoming order using side-specialized helper functions.
TradeVector OrderBook::addOrder(Order order) {
    // Check if the order price is outside our pre-allocated bounds.
    if (order.price < MIN_PRICE || order.price > MAX_PRICE) {
        // Return an empty trade list immediately.
        return TradeVector();
    }

    // Delegate matching logic depending on the order side.
    if (order.side == Side::Buy) {
        // Execute buy specialized match loop.
        return addBuyOrder(order);
    } else {
        // Execute sell specialized match loop.
        return addSellOrder(order);
    }
}

// Implement buy-specialized matching function to avoid side branching inside loop.
__attribute__((always_inline)) inline TradeVector OrderBook::addBuyOrder(Order order) {
    // Zero-allocation stack trade container.
    TradeVector trades;
    // Convert the price tick to a relative array index.
    int price_index = order.price - MIN_PRICE;

    // While the incoming Buy order has remaining quantity and the asks side is not empty...
    while (order.qty > 0 && best_ask_index_ < static_cast<int>(NUM_LEVELS)) {
        // Check if the incoming Buy price index is lower than the best Ask price index (no price cross).
        if (price_index < best_ask_index_) {
            // Stop matching; no further matches are possible.
            break;
        }

        // Get a reference to the active best ask price level in the asks array.
        PriceLevel& best_level = asks_levels_[best_ask_index_];
        // Start iterating from the head of the price queue.
        Order* resting = best_level.head;

        // While we still have incoming quantity and there are resting orders in this price level...
        while (order.qty > 0 && resting != nullptr) {
            // Determine the transaction fill quantity.
            Qty fill_qty = std::min(order.qty, resting->qty);

            // Add a new Trade execution record.
            trades.emplace_back(resting->id, order.id, best_ask_index_ + MIN_PRICE, fill_qty);

            // Reduce the remaining quantity of the incoming order.
            order.qty -= fill_qty;
            // Reduce the remaining quantity of the resting order.
            resting->qty -= fill_qty;

            // Cache a pointer to the next order in queue before any deletion.
            Order* next_resting = resting->next;

            // If the resting order has been fully filled (quantity drops to 0)...
            if (resting->qty == 0) {
                // Erase its record from the fast cancellation lookup index.
                if (resting->id < order_index_.size()) {
                    order_index_[resting->id] = nullptr;
                }
                // Remove it from the doubly-linked queue list.
                best_level.remove(resting);
                // Release the Order object block back to the pool.
                pool_.release(resting);
            }

            // Advance to the next order in the price level queue.
            resting = next_resting;
        }

        // If the best ask price level is now completely empty of orders...
        if (best_level.head == nullptr) {
            // Scan upwards to locate the next active ask price level.
            while (best_ask_index_ < static_cast<int>(NUM_LEVELS) && asks_levels_[best_ask_index_].head == nullptr) {
                // Increment the index.
                ++best_ask_index_;
            }
        }
    }

    // If the incoming order still has quantity remaining, add it as a resting bid.
    if (order.qty > 0) {
        // Acquire a clean Order object from our pool.
        Order* rested = pool_.acquire(order.id, Side::Buy, order.price, order.qty, order.timestamp);
        // Append the pooled order to the back of the queue at its price index level.
        bids_levels_[price_index].push_back(rested);

        // Check if the order ID exceeds the size of the lookup vector.
        if (order.id >= order_index_.size()) {
            // Resize the vector to double the needed size.
            order_index_.resize(order.id * 2, nullptr);
        }
        // Save the pointer to the resting order.
        order_index_[order.id] = rested;

        // Update the cached best bid index.
        if (price_index > best_bid_index_) {
            best_bid_index_ = price_index;
        }
    }

    // Return trades by value.
    return trades;
}

// Implement sell-specialized matching function to avoid side branching inside loop.
__attribute__((always_inline)) inline TradeVector OrderBook::addSellOrder(Order order) {
    // Zero-allocation stack trade container.
    TradeVector trades;
    // Convert the price tick to a relative array index.
    int price_index = order.price - MIN_PRICE;

    // While the incoming Sell order has remaining quantity and the bids side is not empty...
    while (order.qty > 0 && best_bid_index_ >= 0) {
        // Check if the incoming Sell price index is higher than the best Bid price index (no price cross).
        if (price_index > best_bid_index_) {
            // Stop matching; no further matches are possible.
            break;
        }

        // Get a reference to the active best bid price level in the bids array.
        PriceLevel& best_level = bids_levels_[best_bid_index_];
        // Start iterating from the head of the price queue.
        Order* resting = best_level.head;

        // While we still have incoming quantity and there are resting orders in this price level...
        while (order.qty > 0 && resting != nullptr) {
            // Determine the transaction fill quantity.
            Qty fill_qty = std::min(order.qty, resting->qty);

            // Add a new Trade execution record.
            trades.emplace_back(resting->id, order.id, best_bid_index_ + MIN_PRICE, fill_qty);

            // Reduce the remaining quantity of the incoming order.
            order.qty -= fill_qty;
            // Reduce the remaining quantity of the resting order.
            resting->qty -= fill_qty;

            // Cache a pointer to the next order in queue before any deletion.
            Order* next_resting = resting->next;

            // If the resting order has been fully filled (quantity drops to 0)...
            if (resting->qty == 0) {
                // Erase its record from the fast cancellation lookup index.
                if (resting->id < order_index_.size()) {
                    order_index_[resting->id] = nullptr;
                }
                // Remove it from the doubly-linked queue list.
                best_level.remove(resting);
                // Release the Order object block back to the pool.
                pool_.release(resting);
            }

            // Advance to the next order in the price level queue.
            resting = next_resting;
        }

        // If the best bid price level is now completely empty of orders...
        if (best_level.head == nullptr) {
            // Scan downwards to locate the next active bid price level.
            while (best_bid_index_ >= 0 && bids_levels_[best_bid_index_].head == nullptr) {
                // Decrement the index.
                --best_bid_index_;
            }
        }
    }

    // If the incoming order still has quantity remaining, add it as a resting ask.
    if (order.qty > 0) {
        // Acquire a clean Order object from our pool.
        Order* rested = pool_.acquire(order.id, Side::Sell, order.price, order.qty, order.timestamp);
        // Append the pooled order to the back of the queue at its price index level.
        asks_levels_[price_index].push_back(rested);

        // Check if the order ID exceeds the size of the lookup vector.
        if (order.id >= order_index_.size()) {
            // Resize the vector to double the needed size.
            order_index_.resize(order.id * 2, nullptr);
        }
        // Save the pointer to the resting order.
        order_index_[order.id] = rested;

        // Update the cached best ask index.
        if (price_index < best_ask_index_) {
            best_ask_index_ = price_index;
        }
    }

    // Return trades by value.
    return trades;
}

// Cancel an order in O(1) time using intrusive doubly-linked deletion.
bool OrderBook::cancelOrder(OrderId id) {
    // If the order ID is out of bounds for our lookup index...
    if (id >= order_index_.size()) {
        return false;
    }

    // Retrieve the pointer to the active pooled Order object.
    Order* o = order_index_[id];
    // If the order pointer is null (not active in the book)...
    if (o == nullptr) {
        // Return false.
        return false;
    }

    // Reset the lookup index slot for this ID.
    order_index_[id] = nullptr;

    // Get the relative price level index.
    int price_index = o->price - MIN_PRICE;

    // If the order was a Buy...
    if (o->side == Side::Buy) {
        // Retrieve a reference to the corresponding bids price level.
        PriceLevel& level = bids_levels_[price_index];
        // Remove the order from the level queue.
        level.remove(o);
        // If the level is now empty...
        if (level.head == nullptr) {
            // If the emptied level was the best bid, scan downwards to find the next active index.
            if (price_index == best_bid_index_) {
                while (best_bid_index_ >= 0 && bids_levels_[best_bid_index_].head == nullptr) {
                    --best_bid_index_;
                }
            }
        }
    } else {
        // If the order was a Sell...
        // Retrieve a reference to the corresponding asks price level.
        PriceLevel& level = asks_levels_[price_index];
        // Remove the order from the level queue.
        level.remove(o);
        // If the level is now empty...
        if (level.head == nullptr) {
            // If the emptied level was the best ask, scan upwards to find the next active index.
            if (price_index == best_ask_index_) {
                while (best_ask_index_ < static_cast<int>(NUM_LEVELS) && asks_levels_[best_ask_index_].head == nullptr) {
                    ++best_ask_index_;
                }
            }
        }
    }

    // Release the Order object block back to the pool.
    pool_.release(o);

    // Return true to indicate successful cancellation.
    return true;
}

// Return true if both sides of the book have no active orders.
bool OrderBook::empty() const {
    return best_bid_index_ < 0 && best_ask_index_ >= static_cast<int>(NUM_LEVELS);
}

// Convert bids levels to a sorted map representation for compatibility with unit/differential tests.
std::map<Price, std::list<Order>, std::greater<Price>> OrderBook::bids() const {
    // Create an empty sorted map.
    std::map<Price, std::list<Order>, std::greater<Price>> result;
    // Iterate from the best bid index down to zero.
    for (int i = best_bid_index_; i >= 0; --i) {
        const PriceLevel& level = bids_levels_[i];
        if (level.head != nullptr) {
            Price price = i + MIN_PRICE;
            std::list<Order>& order_list = result[price];
            Order* current = level.head;
            while (current != nullptr) {
                order_list.push_back(*current);
                current = current->next;
            }
        }
    }
    return result;
}

// Convert asks levels to a sorted map representation for compatibility with unit/differential tests.
std::map<Price, std::list<Order>> OrderBook::asks() const {
    // Create an empty sorted map.
    std::map<Price, std::list<Order>> result;
    // Iterate from the best ask index up to maximum index boundary.
    for (size_t i = best_ask_index_; i < NUM_LEVELS; ++i) {
        const PriceLevel& level = asks_levels_[i];
        if (level.head != nullptr) {
            Price price = i + MIN_PRICE;
            std::list<Order>& order_list = result[price];
            Order* current = level.head;
            while (current != nullptr) {
                order_list.push_back(*current);
                current = current->next;
            }
        }
    }
    return result;
}

} // namespace lob
