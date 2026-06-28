// Guard this header file from being included multiple times to avoid duplicate definitions.
#ifndef LOB_ORDER_HPP
#define LOB_ORDER_HPP

// Include core types definitions to use OrderId, Price, Qty, and Side within our Order struct.
#include "types.hpp"

// Place all our code in the 'lob' namespace to stay organized.
namespace lob {

// Define the Order struct representing an active request in our matching engine.
struct Order {
    // Unique ID identifying the order (8 bytes).
    OrderId id;
    
    // Trade side: Buy or Sell (1 byte).
    Side side;
    
    // Price limit representing the boundary of execution (8 bytes).
    Price price;
    
    // Remaining quantity of the order that has not yet been matched/filled (8 bytes).
    Qty qty;
    
    // Time priority field represented as an incrementing sequence counter (8 bytes).
    uint64_t timestamp;

    // Default constructor: Initializes member variables to zero and Side::Buy.
    Order()
        : id(0), side(Side::Buy), price(0), qty(0), timestamp(0) {} // Assign initial zero values to all fields

    // Parameterized constructor: Constructs an order with specific attributes.
    Order(OrderId o_id, Side o_side, Price o_price, Qty o_qty, uint64_t o_ts)
        : id(o_id), side(o_side), price(o_price), qty(o_qty), timestamp(o_ts) {} // Assign parameters to their respective class properties

    // Comparison operator to check if two Order objects are identical. This is needed for unit testing and differential testing.
    bool operator==(const Order& other) const {
        // Compare each field to determine equality, returning true only if all fields match.
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
