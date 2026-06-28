// Guard this header file from being included multiple times to avoid compilation errors due to duplicate definitions.
#ifndef LOB_ORDER_HPP
#define LOB_ORDER_HPP

// Include our custom types definition (Price, Qty, Side, OrderId) so we can define the structure properties.
#include "types.hpp"

// Place all code inside the 'lob' namespace to group it and prevent name clashes with other systems.
namespace lob {

// Define a structure to represent a single customer order in our matching engine.
// In high-frequency trading, keep structures compact and clean to maximize CPU cache efficiency (preventing cache misses).
struct Order {
    // The unique identifier assigned to this specific order (8 bytes).
    OrderId id;
    
    // The price limit at which the order should be executed (8 bytes).
    Price price;
    
    // The number of shares/contracts requested in this order (8 bytes).
    Qty qty;
    
    // The direction of the order: Buy or Sell (1 byte).
    Side side;

    // Default constructor: Initializes a blank order with zeros and default side Buy.
    // This allows us to instantiate empty Order objects in arrays or maps.
    Order() 
        : id(0), price(0), qty(0), side(Side::Buy) {} // Set each member variable to its initial zero/default state

    // Parameterized constructor: Conveniently construct an order with specific attributes.
    Order(OrderId order_id, Price order_price, Qty order_qty, Side order_side)
        : id(order_id), price(order_price), qty(order_qty), side(order_side) {} // Assign input parameters to corresponding member fields
};

} // namespace lob

// End of the header guard condition
#endif // LOB_ORDER_HPP
