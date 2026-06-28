// Guard this header file from being included multiple times to avoid duplicate definitions.
#ifndef LOB_REFERENCE_ORDER_BOOK_HPP
#define LOB_REFERENCE_ORDER_BOOK_HPP

// Include the order struct definition so the reference engine can manage orders.
#include "order.hpp"
// Include standard list for price level queue storage.
#include <list>
// Include standard map to sort price levels.
#include <map>
// Include standard unordered_map for constant-time order lookups.
#include <unordered_map>
// Include standard vector to return matched trade executions.
#include <vector>

// Place the reference engine in the 'lob' namespace.
namespace lob {



// Define the OrderLocation structure specific to the reference engine's std::list representation.
struct ReferenceOrderLocation {
    // The side of the order (Buy or Sell).
    Side side;
    // The price level where the order is queued.
    Price price;
    // Iterator pointing directly to the order element in the std::list.
    std::list<Order>::iterator list_it;
};

// Define the ReferenceOrderBook class to act as our Phase 1 ground-truth engine.
class ReferenceOrderBook {
public:
    // Constructor.
    ReferenceOrderBook();
    // Destructor.
    ~ReferenceOrderBook();

    // Process a new order using the map/list algorithm from Phase 1.
    std::vector<Trade> addOrder(Order order);

    // Cancel an order in the book by its unique ID.
    bool cancelOrder(OrderId id);

    // Returns true if both the bids and asks maps are empty.
    bool empty() const;

    // Retrieve the reference bids map.
    const std::map<Price, std::list<Order>, std::greater<Price>>& bids() const {
        return bids_;
    }

    // Retrieve the reference asks map.
    const std::map<Price, std::list<Order>>& asks() const {
        return asks_;
    }

private:
    // Bids storage using sorted map (highest price first).
    std::map<Price, std::list<Order>, std::greater<Price>> bids_;

    // Asks storage using sorted map (lowest price first).
    std::map<Price, std::list<Order>> asks_;

    // Fast lookup index for cancellations.
    std::unordered_map<OrderId, ReferenceOrderLocation> order_index_;
};

} // namespace lob

// End of header guard condition
#endif // LOB_REFERENCE_ORDER_BOOK_HPP
