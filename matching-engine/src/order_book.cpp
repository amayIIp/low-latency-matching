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

// Global override of the standard single-object 'delete' operator.
void operator delete(void* p) noexcept {
    // Free the memory block using free.
    std::free(p);
}

// Global override of the sized single-object 'delete' operator.
void operator delete(void* p, size_t) noexcept {
    // Free the memory block using free.
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
      // Set the initial best bid index to -1 (indicating empty bids book).
      best_bid_index_(-1),
      // Set the initial best ask index to NUM_LEVELS (indicating empty asks book).
      best_ask_index_(NUM_LEVELS) {
    // Body is empty since initialization lists set up all vectors and pool.
}

// Implement the destructor.
OrderBook::~OrderBook() {
    // No raw resources or pointers need manual release since pool_ handles the vector cleanup.
}

// Process and match an incoming order using O(1) array index retrieval.
std::vector<Trade> OrderBook::addOrder(Order order) {
    // Create an output vector to store trade executions.
    std::vector<Trade> trades;

    // Check if the order price is outside our pre-allocated bounds.
    if (order.price < MIN_PRICE || order.price > MAX_PRICE) {
        // Reject the order immediately by returning no trades.
        return trades;
    }

    // Convert the price tick to a relative array index.
    int price_index = order.price - MIN_PRICE;

    // If the order is a Buy...
    if (order.side == Side::Buy) {
        // While the incoming Buy order has remaining quantity and the asks side is not empty...
        while (order.qty > 0 && best_ask_index_ < static_cast<int>(NUM_LEVELS)) {
            // Check if the incoming Buy price index is lower than the best Ask price index (no price cross).
            if (price_index < best_ask_index_) {
                // Stop matching; no further matches are possible.
                break;
            }

            // Get a reference to the active best ask price level in the asks array.
            PriceLevel& best_level = asks_levels_[best_ask_index_];
            // Start iterating from the head of the price queue (earliest time priority).
            Order* resting = best_level.head;

            // While we still have incoming quantity and there are resting orders in this price level...
            while (order.qty > 0 && resting != nullptr) {
                // Determine the transaction fill quantity.
                Qty fill_qty = std::min(order.qty, resting->qty);

                // Add a new Trade execution record (resting order is maker, incoming is taker).
                trades.emplace_back(resting->id, order.id, best_ask_index_ + MIN_PRICE, fill_qty);

                // Reduce the remaining quantity of the incoming order.
                order.qty -= fill_qty;
                // Reduce the remaining quantity of the resting order.
                resting->qty -= fill_qty;

                // Cache a pointer to the next order in queue before any deletion takes place.
                Order* next_resting = resting->next;

                // If the resting order has been fully filled (quantity drops to 0)...
                if (resting->qty == 0) {
                    // Erase its record from the fast cancellation lookup index.
                    order_index_.erase(resting->id);
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
                    // Increment the index to look at the next tick level.
                    ++best_ask_index_;
                }
            }
        }

        // If the incoming order still has quantity remaining, add it as a resting bid.
        if (order.qty > 0) {
            // Acquire a clean Order object from our pre-allocated pool.
            Order* rested = pool_.acquire(order.id, Side::Buy, order.price, order.qty, order.timestamp);
            // Append the pooled order to the back of the queue at its price index level.
            bids_levels_[price_index].push_back(rested);
            // Save a pointer in the fast cancel map.
            order_index_[order.id] = rested;

            // Update the cached best bid index if this new resting price is higher than current best.
            if (price_index > best_bid_index_) {
                best_bid_index_ = price_index;
            }
        }
    } else {
        // If the order is a Sell...
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

                // Add a new Trade execution record (resting order is maker, incoming is taker).
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
                    order_index_.erase(resting->id);
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
                    // Decrement the index to look at the next lower tick level.
                    --best_bid_index_;
                }
            }
        }

        // If the incoming order still has quantity remaining, add it as a resting ask.
        if (order.qty > 0) {
            // Acquire a clean Order object from our pre-allocated pool.
            Order* rested = pool_.acquire(order.id, Side::Sell, order.price, order.qty, order.timestamp);
            // Append the pooled order to the back of the queue at its price index level.
            asks_levels_[price_index].push_back(rested);
            // Save a pointer in the fast cancel map.
            order_index_[order.id] = rested;

            // Update the cached best ask index if this new resting price is lower than current best.
            if (price_index < best_ask_index_) {
                best_ask_index_ = price_index;
            }
        }
    }

    // Return the list of matches.
    return trades;
}

// Cancel an order in O(1) time using intrusive doubly-linked deletion.
bool OrderBook::cancelOrder(OrderId id) {
    // Look up the order pointer in our lookup index.
    auto lookup_it = order_index_.find(id);
    // If the order ID is not found...
    if (lookup_it == order_index_.end()) {
        // Return false indicating cancellation failed.
        return false;
    }

    // Retrieve the pointer to the active pooled Order object.
    Order* o = lookup_it->second;
    // Erase the ID record from the fast cancel map.
    order_index_.erase(lookup_it);

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

    // Release the Order object block back to the pool to make it available for future orders.
    pool_.release(o);

    // Return true to indicate successful cancellation.
    return true;
}

// Return true if both sides of the book have no active orders.
bool OrderBook::empty() const {
    // Return true if best bid index is negative and best ask index is out of bounds.
    return best_bid_index_ < 0 && best_ask_index_ >= static_cast<int>(NUM_LEVELS);
}

// Convert bids levels to a sorted map representation for compatibility with unit/differential tests.
std::map<Price, std::list<Order>, std::greater<Price>> OrderBook::bids() const {
    // Create an empty sorted map (highest price key first).
    std::map<Price, std::list<Order>, std::greater<Price>> result;
    // Iterate from the best bid index down to zero.
    for (int i = best_bid_index_; i >= 0; --i) {
        // Get the price level reference.
        const PriceLevel& level = bids_levels_[i];
        // If there are orders at this level...
        if (level.head != nullptr) {
            // Convert index back to price tick.
            Price price = i + MIN_PRICE;
            // Create a reference to the target list in the map.
            std::list<Order>& order_list = result[price];
            // Iterate through the doubly-linked queue.
            Order* current = level.head;
            while (current != nullptr) {
                // Add a copy of the order to our return list.
                order_list.push_back(*current);
                // Move to next order.
                current = current->next;
            }
        }
    }
    // Return the sorted map.
    return result;
}

// Convert asks levels to a sorted map representation for compatibility with unit/differential tests.
std::map<Price, std::list<Order>> OrderBook::asks() const {
    // Create an empty sorted map (lowest price key first).
    std::map<Price, std::list<Order>> result;
    // Iterate from the best ask index up to the maximum index boundary.
    for (size_t i = best_ask_index_; i < NUM_LEVELS; ++i) {
        // Get the price level reference.
        const PriceLevel& level = asks_levels_[i];
        // If there are orders at this level...
        if (level.head != nullptr) {
            // Convert index back to price tick.
            Price price = i + MIN_PRICE;
            // Create a reference to the target list in the map.
            std::list<Order>& order_list = result[price];
            // Iterate through the doubly-linked queue.
            Order* current = level.head;
            while (current != nullptr) {
                // Add a copy of the order to our return list.
                order_list.push_back(*current);
                // Move to next order.
                current = current->next;
            }
        }
    }
    // Return the sorted map.
    return result;
}

} // namespace lob
