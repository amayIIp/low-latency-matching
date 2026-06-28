// Include the order book header file so we can implement its declared member functions.
#include "lob/order_book.hpp"

// Place all our implementation code inside the 'lob' namespace to match the declaration.
namespace lob {

// Implement the constructor for the OrderBook class.
OrderBook::OrderBook() {
    // Currently, there is no special setup needed, so the body is empty.
    // The member maps and unordered_map are automatically initialized by their default constructors.
}

// Implement the destructor for the OrderBook class.
OrderBook::~OrderBook() {
    // Currently, there are no raw resources or pointers to clean up, so the body is empty.
}

// Implement the add_order method to insert a new order and match it if possible.
std::vector<Trade> OrderBook::add_order(const Order& order) {
    // Create an empty vector to store and return any trades that might execute during matching.
    std::vector<Trade> executed_trades;

    // For now, this is a skeleton implementation:
    // Insert the order into the fast-lookup map so we know it exists.
    order_map_[order.id] = order;

    // Check if the order is on the Buy side.
    if (order.side == Side::Buy) {
        // Add the order to the bids map at the specified price level.
        bids_[order.price].push_back(order);
    } else {
        // Add the order to the asks map at the specified price level.
        asks_[order.price].push_back(order);
    }

    // Return the list of executed trades (which is empty in this initial skeleton).
    return executed_trades;
}

// Implement the cancel_order method to remove an order from the book by its unique ID.
bool OrderBook::cancel_order(OrderId order_id) {
    // Search the fast-lookup map to see if the order exists.
    auto lookup_iterator = order_map_.find(order_id);

    // If the iterator equals end(), it means the order was not found.
    if (lookup_iterator == order_map_.end()) {
        // Return false to indicate that the order could not be cancelled.
        return false;
    }

    // Retrieve a copy of the order to know its price and side for removal from the maps.
    Order order_to_remove = lookup_iterator->second;

    // Erase the order from the fast-lookup map.
    order_map_.erase(lookup_iterator);

    // Check if the order was a Buy order.
    if (order_to_remove.side == Side::Buy) {
        // Find the price level in the bids map.
        auto bids_iterator = bids_.find(order_to_remove.price);
        // If the price level exists, search for the order within that level.
        if (bids_iterator != bids_.end()) {
            // Get a reference to the vector of orders at this price.
            auto& order_list = bids_iterator->second;
            // Loop through the list to find the matching order ID.
            for (auto list_iterator = order_list.begin(); list_iterator != order_list.end(); ++list_iterator) {
                // If we find the correct order, remove it from the list.
                if (list_iterator->id == order_id) {
                    // Erase the order from the vector.
                    order_list.erase(list_iterator);
                    // Break the loop since we found and removed the order.
                    break;
                }
            }
            // If the price level is now empty, clean up the map entry to save memory.
            if (order_list.empty()) {
                // Remove the price level key from the bids map.
                bids_.erase(bids_iterator);
            }
        }
    } else {
        // Find the price level in the asks map.
        auto asks_iterator = asks_.find(order_to_remove.price);
        // If the price level exists, search for the order within that level.
        if (asks_iterator != asks_.end()) {
            // Get a reference to the vector of orders at this price.
            auto& order_list = asks_iterator->second;
            // Loop through the list to find the matching order ID.
            for (auto list_iterator = order_list.begin(); list_iterator != order_list.end(); ++list_iterator) {
                // If we find the correct order, remove it from the list.
                if (list_iterator->id == order_id) {
                    // Erase the order from the vector.
                    order_list.erase(list_iterator);
                    // Break the loop since we found and removed the order.
                    break;
                }
            }
            // If the price level is now empty, clean up the map entry to save memory.
            if (order_list.empty()) {
                // Remove the price level key from the asks map.
                asks_.erase(asks_iterator);
            }
        }
    }

    // Return true to indicate that the order was successfully found and cancelled.
    return true;
}

// Implement the empty method to check if the order book has no active bids and asks.
bool OrderBook::empty() const {
    // Return true only if both the bids map and asks map are empty.
    return bids_.empty() && asks_.empty();
}

} // namespace lob
