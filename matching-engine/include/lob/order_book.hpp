// Guard this header file from being included multiple times to avoid duplicate definitions.
#ifndef LOB_ORDER_BOOK_HPP
#define LOB_ORDER_BOOK_HPP

// Include the order definition header so that we can manage Order objects.
#include "order.hpp"
// Include the standard list header to store queues of orders at each price level with O(1) insertion/deletion.
#include <list>
// Include the standard map header to sort price levels.
#include <map>
// Include the standard unordered_map header for constant-time order lookups.
#include <unordered_map>
// Include the standard vector header to return lists of executed trades.
#include <vector>

// Place all classes inside the 'lob' namespace.
namespace lob {

// Define the Trade structure representing an execution match between two orders.
struct Trade {
    // The OrderId of the resting order that was already in the book (maker) (8 bytes).
    OrderId maker_id;
    // The OrderId of the incoming order that triggered the match (taker) (8 bytes).
    OrderId taker_id;
    // The price at which the match occurred (8 bytes).
    Price price;
    // The amount of quantity filled in this match (8 bytes).
    Qty qty;

    // Parameterized constructor to initialize trade records.
    Trade(OrderId m_id, OrderId t_id, Price p, Qty q)
        : maker_id(m_id), taker_id(t_id), price(p), qty(q) {} // Assign input parameters to corresponding member fields

    // Comparison operator to check if two Trade objects are identical. Required for testing expectations in GoogleTest.
    bool operator==(const Trade& other) const {
        // Compare each field of the trade structure, returning true only if all fields match.
        return maker_id == other.maker_id &&
               taker_id == other.taker_id &&
               price == other.price &&
               qty == other.qty;
    }
};

// Define the structure representing the location of an order in the book.
// Storing an iterator to the std::list allows us to achieve O(1) order cancellation.
struct OrderLocation {
    // The side of the order (Buy or Sell).
    Side side;
    // The price level of the order.
    Price price;
    // The iterator pointing to the specific Order element in the std::list at that price level.
    std::list<Order>::iterator list_it;
};

// Define the main OrderBook class to implement core matching engine operations.
class OrderBook {
public:
    // Constructor to initialize an empty order book.
    OrderBook();

    // Destructor to clean up resources.
    ~OrderBook();

    // Processes a new order: matches it against the opposite side, generates trades, and rests any unfilled quantity.
    std::vector<Trade> addOrder(Order order);

    // Cancels an order by its unique ID in O(1) time using the location lookup index.
    bool cancelOrder(OrderId id);

    // Returns true if there are no active bids or asks in the book.
    bool empty() const;

    // Helper method to retrieve bids map (used for unit tests and printing book state).
    const std::map<Price, std::list<Order>, std::greater<Price>>& bids() const {
        return bids_;
    }

    // Helper method to retrieve asks map (used for unit tests and printing book state).
    const std::map<Price, std::list<Order>>& asks() const {
        return asks_;
    }

private:
    // Bids storage: Maps a price to a list of Orders. std::greater<Price> ensures highest bid is at bids_.begin().
    std::map<Price, std::list<Order>, std::greater<Price>> bids_;

    // Asks storage: Maps a price to a list of Orders. Default std::less<Price> ensures lowest ask is at asks_.begin().
    std::map<Price, std::list<Order>> asks_;

    // Fast order location index: Maps OrderId to OrderLocation for O(1) lookup during cancellation.
    std::unordered_map<OrderId, OrderLocation> order_index_;
};

} // namespace lob

// End of header guard condition
#endif // LOB_ORDER_BOOK_HPP
