
#include "lob/reference_order_book.hpp"

#include <algorithm>

namespace lob {

ReferenceOrderBook::ReferenceOrderBook() {}

ReferenceOrderBook::~ReferenceOrderBook() {}

std::vector<Trade> ReferenceOrderBook::addOrder(Order incoming) {

    std::vector<Trade> trades;

    if (incoming.side == Side::Buy) {

        while (incoming.qty > 0 && !asks_.empty()) {

            auto best_level_it = asks_.begin();
            Price best_ask_price = best_level_it->first;

            if (incoming.price < best_ask_price) {
                break;
            }

            auto& orders_list = best_level_it->second;

            while (incoming.qty > 0 && !orders_list.empty()) {
                auto& resting = orders_list.front();
                Qty fill_qty = std::min(incoming.qty, resting.qty);

                trades.emplace_back(resting.id, incoming.id, best_ask_price, fill_qty);

                incoming.qty -= fill_qty;
                resting.qty -= fill_qty;

                if (resting.qty == 0) {
                    order_index_.erase(resting.id);
                    orders_list.pop_front();
                }
            }

            if (orders_list.empty()) {
                asks_.erase(best_level_it);
            }
        }

        if (incoming.qty > 0) {
            auto& level_list = bids_[incoming.price];
            level_list.push_back(incoming);
            auto new_order_it = std::prev(level_list.end());
            order_index_[incoming.id] = ReferenceOrderLocation{Side::Buy, incoming.price, new_order_it};
        }
    } else {

        while (incoming.qty > 0 && !bids_.empty()) {

            auto best_level_it = bids_.begin();
            Price best_bid_price = best_level_it->first;

            if (incoming.price > best_bid_price) {
                break;
            }

            auto& orders_list = best_level_it->second;

            while (incoming.qty > 0 && !orders_list.empty()) {
                auto& resting = orders_list.front();
                Qty fill_qty = std::min(incoming.qty, resting.qty);

                trades.emplace_back(resting.id, incoming.id, best_bid_price, fill_qty);

                incoming.qty -= fill_qty;
                resting.qty -= fill_qty;

                if (resting.qty == 0) {
                    order_index_.erase(resting.id);
                    orders_list.pop_front();
                }
            }

            if (orders_list.empty()) {
                bids_.erase(best_level_it);
            }
        }

        if (incoming.qty > 0) {
            auto& level_list = asks_[incoming.price];
            level_list.push_back(incoming);
            auto new_order_it = std::prev(level_list.end());
            order_index_[incoming.id] = ReferenceOrderLocation{Side::Sell, incoming.price, new_order_it};
        }
    }

    return trades;
}

bool ReferenceOrderBook::cancelOrder(OrderId id) {

    auto lookup_it = order_index_.find(id);
    if (lookup_it == order_index_.end()) {
        return false;
    }

    ReferenceOrderLocation location = lookup_it->second;
    order_index_.erase(lookup_it);

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

bool ReferenceOrderBook::empty() const {
    return bids_.empty() && asks_.empty();
}

}
