// Guard this header file from being included multiple times to avoid duplicate definition errors.
#ifndef LOB_ORDER_HPP
#define LOB_ORDER_HPP

// Include core types to define Order fields.
#include "types.hpp"

// Place all structures within the 'lob' namespace.
namespace lob {

// Define the Order structure. It now contains intrusive pointers next and prev.
// This allows us to link Order objects directly into doubly-linked queues and object pools
// without allocating external memory blocks.
struct Order {
    // Unique ID identifying the order (8 bytes).
    OrderId id;
    
    // Trade side: Buy or Sell (1 byte).
    Side side;
    
    // Price limit (8 bytes).
    Price price;
    
    // Remaining quantity of the order (8 bytes).
    Qty qty;
    
    // Time priority sequence number (8 bytes).
    uint64_t timestamp;

    // Intrusive pointer to the next Order element (8 bytes).
    // Used for the free list in the Object Pool and for FIFO queues at each price level.
    Order* next = nullptr;

    // Intrusive pointer to the previous Order element (8 bytes).
    // Used to build doubly-linked lists at each price level for fast unlinking.
    Order* prev = nullptr;

    // Default constructor: Initializes member variables to zero/null.
    Order()
        : id(0), side(Side::Buy), price(0), qty(0), timestamp(0), next(nullptr), prev(nullptr) {} // Set all values to default state

    // Parameterized constructor: Constructs an order with specific attributes.
    Order(OrderId o_id, Side o_side, Price o_price, Qty o_qty, uint64_t o_ts)
        : id(o_id), side(o_side), price(o_price), qty(o_qty), timestamp(o_ts), next(nullptr), prev(nullptr) {} // Initialize fields

    // Comparison operator to check if two Order objects are identical. Used in unit and differential tests.
    bool operator==(const Order& other) const {
        // Return true only if all essential fields are identical.
        return id == other.id &&
               side == other.side &&
               price == other.price &&
               qty == other.qty &&
               timestamp == other.timestamp;
    }
};

} // namespace lob

// End of header guard condition
#endif // LOB_ORDER_HPP
