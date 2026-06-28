// Guard this header file from being included multiple times to avoid duplicate definitions during compilation.
#ifndef LOB_ORDER_BOOK_HPP
#define LOB_ORDER_BOOK_HPP

// Include the order definition header so that the OrderBook can manage Order instances.
#include "order.hpp"
// Include the standard vector library to support lists of order events or trades.
#include <vector>
// Include standard map library to store price levels ordered by price.
#include <map>
// Include standard unordered_map library to quickly look up orders by their unique ID in constant time.
#include <unordered_map>

// Place the OrderBook class in the 'lob' namespace to stay consistent with other engine components.
namespace lob {

// Define a structure to represent a trade execution that occurs when a Buy and Sell order match.
struct Trade {
    // The OrderId of the incoming/maker order that got matched.
    OrderId maker_id;
    // The OrderId of the outgoing/taker order that triggered the match.
    OrderId taker_id;
    // The execution price where the trade occurred.
    Price price;
    // The quantity filled/exchanged in this trade.
    Qty qty;

    // Constructor to initialize all trade details at once.
    Trade(OrderId m_id, OrderId t_id, Price p, Qty q)
        : maker_id(m_id), taker_id(t_id), price(p), qty(q) {} // Assign input parameters to corresponding member fields
};

// Define the main OrderBook class responsible for matching bids (buys) and asks (sells).
class OrderBook {
public:
    // Constructor to initialize an empty order book.
    OrderBook();

    // Destructor to clean up resources when the order book is destroyed.
    ~OrderBook();

    // Process and add a new order to the book, returning any trades that resulted from matching.
    std::vector<Trade> add_order(const Order& order);

    // Cancel/remove an existing order from the book by its unique ID. Returns true if found and removed.
    bool cancel_order(OrderId order_id);

    // Check if the order book is empty on both bids and asks sides.
    bool empty() const;

private:
    // Define a custom comparator to sort bids in descending order (highest price first, since buyers want high prices).
    struct BidComp {
        // Overload the call operator to compare two prices.
        bool operator()(const Price& a, const Price& b) const {
            // Return true if price a is greater than price b to sort highest to lowest.
            return a > b;
        }
    };

    // Bids storage: Maps a price to a list (vector) of Orders at that price level, sorted descending.
    std::map<Price, std::vector<Order>, BidComp> bids_;

    // Asks storage: Maps a price to a list (vector) of Orders at that price level, sorted ascending (lowest price first, since sellers want low prices).
    std::map<Price, std::vector<Order>> asks_;

    // Order lookup: Fast hash map mapping an OrderId to its current active order data for constant-time lookups.
    std::unordered_map<OrderId, Order> order_map_;
};

} // namespace lob

// End of the header guard condition
#endif // LOB_ORDER_BOOK_HPP
