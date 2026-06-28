// Guard this header file from being included multiple times to avoid duplicate definitions.
#ifndef LOB_ORDER_BOOK_HPP
#define LOB_ORDER_BOOK_HPP

// Include order definitions to access the intrusive Order struct.
#include "order.hpp"
// Include atomic for the zero-allocation check flag.
#include <atomic>
// Include vector for pre-allocating contiguous memory pools and levels.
#include <vector>
// Include map to return compatible sorted structures for testing.
#include <map>
// Include list for building test-compatible list copies.
#include <list>

// Place all our engine declarations inside the 'lob' namespace.
namespace lob {

#ifdef PROVE_ZERO_ALLOC
// Declare the global flag that disables heap allocations when set to true.
extern std::atomic<bool> allocations_forbidden;
#endif

// Represents a price level queue using intrusive pointers in the Order objects.
struct PriceLevel {
    // Pointer to the first order in the queue (highest priority).
    Order* head = nullptr;
    // Pointer to the last order in the queue (lowest priority).
    Order* tail = nullptr;
    // Total aggregated quantity of all active orders at this price level.
    Qty total_qty = 0;

    // Append a new order to the back of the queue (FIFO time priority).
    void push_back(Order* o) {
        // The new order's prev points to the current tail.
        o->prev = tail;
        // The new order's next points to null since it's the last element.
        o->next = nullptr;
        // If a tail exists, link its next pointer to the new order.
        if (tail) {
            tail->next = o;
        } else {
            // If no tail exists, this is the first order; set head to it.
            head = o;
        }
        // Update the tail of the level to the new order.
        tail = o;
        // Increase the total volume at this price level.
        total_qty += o->qty;
    }

    // Remove an order from this price level queue.
    void remove(Order* o) {
        // If this order has a predecessor, link it to this order's successor.
        if (o->prev) {
            o->prev->next = o->next;
        } else {
            // Otherwise, this order was the head; set head to the successor.
            head = o->next;
        }
        // If this order has a successor, link it to this order's predecessor.
        if (o->next) {
            o->next->prev = o->prev;
        } else {
            // Otherwise, this order was the tail; set tail to the predecessor.
            tail = o->prev;
        }
        // Decrease the total volume at this price level.
        total_qty -= o->qty;
        // Reset the unlinked order's pointers.
        o->next = nullptr;
        o->prev = nullptr;
    }
};

// Object Pool implementation to allocate all Order structures at startup.
class OrderPool {
private:
    // Contiguous vector storage pre-allocated once on the heap.
    std::vector<Order> storage_;
    // Pointer to the first free Order structure in our pool.
    Order* free_list_head_ = nullptr;

public:
    // Constructor. Takes the maximum capacity of orders the pool can hold.
    OrderPool(size_t capacity) {
        // Resize storage to pre-allocate capacity elements.
        storage_.resize(capacity);
        // Link each Order to the next one in memory to build the initial free list.
        for (size_t i = 0; i + 1 < capacity; ++i) {
            storage_[i].next = &storage_[i + 1];
        }
        // Set the head of the free list to the first element in storage.
        free_list_head_ = &storage_[0];
    }

    // Acquire an Order structure from the free list, resetting its properties.
    Order* acquire(OrderId id, Side side, Price price, Qty qty, uint64_t timestamp) {
        // If the free list is empty, return null.
        if (!free_list_head_) {
            return nullptr;
        }
        // Save pointer to the head of the free list.
        Order* o = free_list_head_;
        // Advance the free list head pointer to the next free structure.
        free_list_head_ = free_list_head_->next;
        // Assign new properties to the acquired Order.
        o->id = id;
        o->side = side;
        o->price = price;
        o->qty = qty;
        o->timestamp = timestamp;
        // Nullify intrusive links to make the Order clean.
        o->next = nullptr;
        o->prev = nullptr;
        // Return the clean Order pointer.
        return o;
    }

    // Release an Order structure back to the free list.
    void release(Order* o) {
        // Point the released Order's next pointer to the current free list head.
        o->next = free_list_head_;
        // Set the free list head to the newly returned Order.
        free_list_head_ = o;
    }
};

// Optimized OrderBook class.
class OrderBook {
public:
    // Define the fixed price boundaries for the array ladders.
    static constexpr Price MIN_PRICE = 8000;
    static constexpr Price MAX_PRICE = 12000;
    // Size of the array index (4001 slots).
    static constexpr size_t NUM_LEVELS = MAX_PRICE - MIN_PRICE + 1;
    // Pre-allocated capacity for the order object pool.
    static constexpr size_t POOL_CAPACITY = 1100000;

    // Constructor.
    OrderBook();
    // Destructor.
    ~OrderBook();

    // Process a new order using fast array-indexing and object pooling.
    // Returns our zero-allocation TradeVector container by value.
    TradeVector addOrder(Order order);

    // Cancel an order in O(1) time using intrusive doubly-linked deletion.
    bool cancelOrder(OrderId id);

    // Returns true if the order book is empty on both sides.
    bool empty() const;

    // Returns a copy of the bids book in a map format (for test compatibility).
    std::map<Price, std::list<Order>, std::greater<Price>> bids() const;

    // Returns a copy of the asks book in a map format (for test compatibility).
    std::map<Price, std::list<Order>> asks() const;

private:
    // The pre-allocated pool of Order objects.
    OrderPool pool_;

    // Fast array-indexed bids levels.
    std::vector<PriceLevel> bids_levels_;
    // Fast array-indexed asks levels.
    std::vector<PriceLevel> asks_levels_;

    // Pre-allocated vector mapping OrderId to active Order pointer.
    // This replaces std::unordered_map to guarantee zero runtime heap allocations.
    std::vector<Order*> order_index_;

    // Cached best bid price index (represented as price - MIN_PRICE).
    // Initialized to -1 when empty.
    int best_bid_index_ = -1;

    // Cached best ask price index (represented as price - MIN_PRICE).
    // Initialized to NUM_LEVELS when empty.
    int best_ask_index_ = NUM_LEVELS;

    // Helper functions to specialize matching logic per side and eliminate branch checks.
    __attribute__((always_inline)) inline TradeVector addBuyOrder(Order order);
    __attribute__((always_inline)) inline TradeVector addSellOrder(Order order);
};

} // namespace lob

// End of header guard condition
#endif // LOB_ORDER_BOOK_HPP
