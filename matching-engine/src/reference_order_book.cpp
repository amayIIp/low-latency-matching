// Include the reference order book header.
#include "lob/reference_order_book.hpp"
// Include algorithm for std::min.
#include <algorithm>

// Place the code inside the 'lob' namespace.
namespace lob {

// Constructor.
ReferenceOrderBook::ReferenceOrderBook() {}

// Destructor.
ReferenceOrderBook::~ReferenceOrderBook() {}

// Add order implementation using sorted maps and list iterators (Phase 1).
std::vector<Trade> ReferenceOrderBook::addOrder(Order incoming) {
    // Create vector to hold execution reports.
    std::vector<Trade> trades;

    // Check if the order is a Buy order.
    if (incoming.side == Side::Buy) {
        // Match against asks while quantity remains and asks are not empty.
        while (incoming.qty > 0 && !asks_.empty()) {
            // Retrieve best ask level.
            auto best_level_it = asks_.begin();
            Price best_ask_price = best_level_it->first;

            // Check if price crosses.
            if (incoming.price < best_ask_price) {
                break;
            }

            // Get list of orders at this level.
            auto& orders_list = best_level_it->second;

            // Match against resting orders in FIFO order.
            while (incoming.qty > 0 && !orders_list.empty()) {
                auto& resting = orders_list.front();
                Qty fill_qty = std::min(incoming.qty, resting.qty);

                // Record trade match.
                trades.emplace_back(resting.id, incoming.id, best_ask_price, fill_qty);

                // Update quantities.
                incoming.qty -= fill_qty;
                resting.qty -= fill_qty;

                // Remove resting order if fully filled.
                if (resting.qty == 0) {
                    order_index_.erase(resting.id);
                    orders_list.pop_front();
                }
            }

            // Erase price level if empty.
            if (orders_list.empty()) {
                asks_.erase(best_level_it);
            }
        }

        // Rest remaining quantity in the book.
        if (incoming.qty > 0) {
            auto& level_list = bids_[incoming.price];
            level_list.push_back(incoming);
            auto new_order_it = std::prev(level_list.end());
            order_index_[incoming.id] = ReferenceOrderLocation{Side::Buy, incoming.price, new_order_it};
        }
    } else {
        // Match against bids while quantity remains and bids are not empty.
        while (incoming.qty > 0 && !bids_.empty()) {
            // Retrieve best bid level.
            auto best_level_it = bids_.begin();
            Price best_bid_price = best_level_it->first;

            // Check if price crosses.
            if (incoming.price > best_bid_price) {
                break;
            }

            // Get list of orders at this level.
            auto& orders_list = best_level_it->second;

            // Match against resting orders in FIFO order.
            while (incoming.qty > 0 && !orders_list.empty()) {
                auto& resting = orders_list.front();
                Qty fill_qty = std::min(incoming.qty, resting.qty);

                // Record trade match.
                trades.emplace_back(resting.id, incoming.id, best_bid_price, fill_qty);

                // Update quantities.
                incoming.qty -= fill_qty;
                resting.qty -= fill_qty;

                // Remove resting order if fully filled.
                if (resting.qty == 0) {
                    order_index_.erase(resting.id);
                    orders_list.pop_front();
                }
            }

            // Erase price level if empty.
            if (orders_list.empty()) {
                bids_.erase(best_level_it);
            }
        }

        // Rest remaining quantity in the book.
        if (incoming.qty > 0) {
            auto& level_list = asks_[incoming.price];
            level_list.push_back(incoming);
            auto new_order_it = std::prev(level_list.end());
            order_index_[incoming.id] = ReferenceOrderLocation{Side::Sell, incoming.price, new_order_it};
        }
    }

    // Return executions list.
    return trades;
}

// Cancel order implementation.
bool ReferenceOrderBook::cancelOrder(OrderId id) {
    // Look up order location in index.
    auto lookup_it = order_index_.find(id);
    if (lookup_it == order_index_.end()) {
        return false;
    }

    // Get location information.
    ReferenceOrderLocation location = lookup_it->second;
    order_index_.erase(lookup_it);

    // Erase from corresponding map and price list.
    if (location.side == Side::Buy) {
        auto level_it = bids_.find(location.price);
        if (level_it != bids_.end()) {
            level_it->second.erase(location.list_it);
            if (level_it->second.empty()) {
                bids_.erase(level_it);
            }
        }
    } else {
        auto level_it = asks_.find(location.price);
        if (level_it != asks_.end()) {
            level_it->second.erase(location.list_it);
            if (level_it->second.empty()) {
                asks_.erase(level_it);
            }
        }
    }

    return true;
}

// Empty status check.
bool ReferenceOrderBook::empty() const {
    return bids_.empty() && asks_.empty();
}

} // namespace lob
