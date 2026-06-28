// Include the order book header file so we can implement the OrderBook members.
#include "lob/order_book.hpp"
// Include algorithm header for access to std::min function.
#include <algorithm>

// Place all our implementation code inside the 'lob' namespace.
namespace lob {

// Implement the OrderBook constructor.
OrderBook::OrderBook() {
    // Currently, there is no special setup required, so the body is empty.
    // The standard containers bids_, asks_, and order_index_ are constructed automatically.
}

// Implement the OrderBook destructor.
OrderBook::~OrderBook() {
    // No raw memory allocations or files are open, so the body is empty.
}

// Implement the addOrder method to insert/match an incoming order.
std::vector<Trade> OrderBook::addOrder(Order incoming) {
    // Create an empty vector to collect and return trade execution objects.
    std::vector<Trade> trades;

    // Check if the incoming order is a Buy order.
    if (incoming.side == Side::Buy) {
        // While the incoming buy order still has unfilled quantity and the asks side is not empty...
        while (incoming.qty > 0 && !asks_.empty()) {
            // Get an iterator to the lowest ask price level (first element of the asks map).
            auto best_level_it = asks_.begin();
            
            // Extract the price of this lowest ask level (since it's the first element).
            Price best_ask_price = best_level_it->first;

            // Check if the incoming buy price is lower than the best ask price (isMarketable check).
            if (incoming.price < best_ask_price) {
                // If buy price is less than sell price, no trade can happen; exit the matching loop.
                break;
            }

            // Get a reference to the list of resting sell orders at this price level.
            auto& orders_list = best_level_it->second;

            // While the incoming order still has quantity and there are resting orders at this price level...
            while (incoming.qty > 0 && !orders_list.empty()) {
                // Get a reference to the front order in the queue (earliest time priority).
                auto& resting = orders_list.front();
                
                // Calculate the match quantity: the minimum of what's incoming and what's resting.
                Qty fill_qty = std::min(incoming.qty, resting.qty);

                // Add a new Trade execution record (resting order ID is maker, incoming ID is taker).
                trades.emplace_back(resting.id, incoming.id, best_ask_price, fill_qty);

                // Reduce the remaining quantity of the incoming order.
                incoming.qty -= fill_qty;
                
                // Reduce the remaining quantity of the resting order.
                resting.qty -= fill_qty;

                // If the resting order is fully filled (quantity drops to 0)...
                if (resting.qty == 0) {
                    // Erase it from the O(1) cancel lookup index using its ID.
                    order_index_.erase(resting.id);
                    
                    // Remove it from the front of the queue.
                    orders_list.pop_front();
                }
            }

            // If all resting orders at this price level are gone...
            if (orders_list.empty()) {
                // Erase the price level from the asks map to clean up memory.
                asks_.erase(best_level_it);
            }
        }

        // If there is still quantity remaining on the incoming buy order, store it as a resting bid.
        if (incoming.qty > 0) {
            // Access or insert the list at the incoming order's price in the bids map.
            auto& level_list = bids_[incoming.price];
            
            // Append the order to the back of the queue (preserving time priority).
            level_list.push_back(incoming);
            
            // Get an iterator to the newly inserted order at the back of the list.
            auto new_order_it = std::prev(level_list.end());
            
            // Save the location details of this order in our lookup map for fast cancellation.
            order_index_[incoming.id] = OrderLocation{Side::Buy, incoming.price, new_order_it};
        }
    } else {
        // If the incoming order is a Sell order...
        // While the incoming sell order still has unfilled quantity and the bids side is not empty...
        while (incoming.qty > 0 && !bids_.empty()) {
            // Get an iterator to the highest bid price level (first element of the bids map, sorted descending).
            auto best_level_it = bids_.begin();
            
            // Extract the price of this highest bid level.
            Price best_bid_price = best_level_it->first;

            // Check if the incoming sell price is higher than the best bid price (isMarketable check).
            if (incoming.price > best_bid_price) {
                // If sell price is greater than buy price, no trade can happen; exit the matching loop.
                break;
            }

            // Get a reference to the list of resting buy orders at this price level.
            auto& orders_list = best_level_it->second;

            // While the incoming order still has quantity and there are resting orders at this price level...
            while (incoming.qty > 0 && !orders_list.empty()) {
                // Get a reference to the front order in the queue (earliest time priority).
                auto& resting = orders_list.front();
                
                // Calculate the match quantity: the minimum of what's incoming and what's resting.
                Qty fill_qty = std::min(incoming.qty, resting.qty);

                // Add a new Trade execution record (resting order ID is maker, incoming ID is taker).
                trades.emplace_back(resting.id, incoming.id, best_bid_price, fill_qty);

                // Reduce the remaining quantity of the incoming order.
                incoming.qty -= fill_qty;
                
                // Reduce the remaining quantity of the resting order.
                resting.qty -= fill_qty;

                // If the resting order is fully filled (quantity drops to 0)...
                if (resting.qty == 0) {
                    // Erase it from the O(1) cancel lookup index using its ID.
                    order_index_.erase(resting.id);
                    
                    // Remove it from the front of the queue.
                    orders_list.pop_front();
                }
            }

            // If all resting orders at this price level are gone...
            if (orders_list.empty()) {
                // Erase the price level from the bids map to clean up memory.
                bids_.erase(best_level_it);
            }
        }

        // If there is still quantity remaining on the incoming sell order, store it as a resting ask.
        if (incoming.qty > 0) {
            // Access or insert the list at the incoming order's price in the asks map.
            auto& level_list = asks_[incoming.price];
            
            // Append the order to the back of the queue (preserving time priority).
            level_list.push_back(incoming);
            
            // Get an iterator to the newly inserted order at the back of the list.
            auto new_order_it = std::prev(level_list.end());
            
            // Save the location details of this order in our lookup map for fast cancellation.
            order_index_[incoming.id] = OrderLocation{Side::Sell, incoming.price, new_order_it};
        }
    }

    // Return the list of generated trade executions.
    return trades;
}

// Implement the cancelOrder method to remove an active order from the book in O(1) time.
bool OrderBook::cancelOrder(OrderId id) {
    // Search the lookup map for the order ID.
    auto lookup_it = order_index_.find(id);
    
    // If the order ID is not found in our index...
    if (lookup_it == order_index_.end()) {
        // Return false to indicate the cancellation failed.
        return false;
    }

    // Retrieve the location details of the order.
    OrderLocation location = lookup_it->second;
    
    // Erase the order entry from the fast lookup map.
    order_index_.erase(lookup_it);

    // If the order is on the Buy side...
    if (location.side == Side::Buy) {
        // Find the price level in our bids map.
        auto level_it = bids_.find(location.price);
        
        // If the price level exists...
        if (level_it != bids_.end()) {
            // Erase the order directly from the list using the stored list iterator.
            level_it->second.erase(location.list_it);
            
            // If the price level's order queue is now completely empty...
            if (level_it->second.empty()) {
                // Erase the price level from the bids map to free memory.
                bids_.erase(level_it);
            }
        }
    } else {
        // If the order is on the Sell side...
        // Find the price level in our asks map.
        auto level_it = asks_.find(location.price);
        
        // If the price level exists...
        if (level_it != asks_.end()) {
            // Erase the order directly from the list using the stored list iterator.
            level_it->second.erase(location.list_it);
            
            // If the price level's order queue is now completely empty...
            if (level_it->second.empty()) {
                // Erase the price level from the asks map to free memory.
                asks_.erase(level_it);
            }
        }
    }

    // Return true to indicate the order was successfully cancelled.
    return true;
}

// Implement the empty method to check if the book has no orders.
bool OrderBook::empty() const {
    // Return true only if both bids map and asks map are empty.
    return bids_.empty() && asks_.empty();
}

} // namespace lob
