
#include "lob/order_book.hpp"

#include <algorithm>

#ifdef PROVE_ZERO_ALLOC

#include <cstdlib>

#include <iostream>

namespace lob {

std::atomic<bool> allocations_forbidden{false};
}

void* operator new(size_t size) {

    if (lob::allocations_forbidden.load(std::memory_order_relaxed)) {

        std::cerr << "CRITICAL ERROR: Dynamic memory allocation of " << size
                  << " bytes attempted while allocations_forbidden is ACTIVE!\n";

        std::abort();
    }

    return std::malloc(size);
}

void* operator new[](size_t size) {
    if (lob::allocations_forbidden.load(std::memory_order_relaxed)) {
        std::cerr << "CRITICAL ERROR: Dynamic memory allocation of " << size
                  << " bytes attempted while allocations_forbidden is ACTIVE!\n";
        std::abort();
    }
    return std::malloc(size);
}

void* operator new(size_t size, const std::nothrow_t&) noexcept {
    if (lob::allocations_forbidden.load(std::memory_order_relaxed)) {
        std::cerr << "CRITICAL ERROR: Dynamic memory allocation of " << size
                  << " bytes attempted while allocations_forbidden is ACTIVE!\n";
        std::abort();
    }
    return std::malloc(size);
}

void* operator new[](size_t size, const std::nothrow_t&) noexcept {
    if (lob::allocations_forbidden.load(std::memory_order_relaxed)) {
        std::cerr << "CRITICAL ERROR: Dynamic memory allocation of " << size
                  << " bytes attempted while allocations_forbidden is ACTIVE!\n";
        std::abort();
    }
    return std::malloc(size);
}

void operator delete(void* p) noexcept {

    std::free(p);
}

void operator delete[](void* p) noexcept {
    std::free(p);
}

void operator delete(void* p, size_t) noexcept {

    std::free(p);
}

void operator delete[](void* p, size_t) noexcept {
    std::free(p);
}

void operator delete(void* p, const std::nothrow_t&) noexcept {
    std::free(p);
}

void operator delete[](void* p, const std::nothrow_t&) noexcept {
    std::free(p);
}
#endif

namespace lob {

OrderBook::OrderBook()

    : pool_(POOL_CAPACITY),

      bids_levels_(NUM_LEVELS),

      asks_levels_(NUM_LEVELS),

      order_index_(2000000, nullptr),

      best_bid_index_(-1),

      best_ask_index_(NUM_LEVELS) {

}

OrderBook::~OrderBook() {

}

TradeVector OrderBook::addOrder(Order order) {

    if (order.price < MIN_PRICE || order.price > MAX_PRICE) {

        return TradeVector();
    }

    if (order.side == Side::Buy) {

        return addBuyOrder(order);
    } else {

        return addSellOrder(order);
    }
}

__attribute__((always_inline)) inline TradeVector OrderBook::addBuyOrder(Order order) {

    TradeVector trades;

    int price_index = order.price - MIN_PRICE;

    while (order.qty > 0 && best_ask_index_ < static_cast<int>(NUM_LEVELS)) {

        if (price_index < best_ask_index_) {

            break;
        }

        PriceLevel& best_level = asks_levels_[best_ask_index_];

        Order* resting = best_level.head;

        while (order.qty > 0 && resting != nullptr) {

            Qty fill_qty = std::min(order.qty, resting->qty);

            trades.emplace_back(resting->id, order.id, best_ask_index_ + MIN_PRICE, fill_qty);

            order.qty -= fill_qty;

            resting->qty -= fill_qty;

            Order* next_resting = resting->next;

            if (resting->qty == 0) {

                if (resting->id < order_index_.size()) {
                    order_index_[resting->id] = nullptr;
                }

                best_level.remove(resting);

                pool_.release(resting);
            }

            resting = next_resting;
        }

        if (best_level.head == nullptr) {

            while (best_ask_index_ < static_cast<int>(NUM_LEVELS) && asks_levels_[best_ask_index_].head == nullptr) {

                ++best_ask_index_;
            }
        }
    }

    if (order.qty > 0) {

        Order* rested = pool_.acquire(order.id, Side::Buy, order.price, order.qty, order.timestamp);

        bids_levels_[price_index].push_back(rested);

        if (order.id >= order_index_.size()) {

            order_index_.resize(order.id * 2, nullptr);
        }

        order_index_[order.id] = rested;

        if (price_index > best_bid_index_) {
            best_bid_index_ = price_index;
        }
    }

    return trades;
}

__attribute__((always_inline)) inline TradeVector OrderBook::addSellOrder(Order order) {

    TradeVector trades;

    int price_index = order.price - MIN_PRICE;

    while (order.qty > 0 && best_bid_index_ >= 0) {

        if (price_index > best_bid_index_) {

            break;
        }

        PriceLevel& best_level = bids_levels_[best_bid_index_];

        Order* resting = best_level.head;

        while (order.qty > 0 && resting != nullptr) {

            Qty fill_qty = std::min(order.qty, resting->qty);

            trades.emplace_back(resting->id, order.id, best_bid_index_ + MIN_PRICE, fill_qty);

            order.qty -= fill_qty;

            resting->qty -= fill_qty;

            Order* next_resting = resting->next;

            if (resting->qty == 0) {

                if (resting->id < order_index_.size()) {
                    order_index_[resting->id] = nullptr;
                }

                best_level.remove(resting);

                pool_.release(resting);
            }

            resting = next_resting;
        }

        if (best_level.head == nullptr) {

            while (best_bid_index_ >= 0 && bids_levels_[best_bid_index_].head == nullptr) {

                --best_bid_index_;
            }
        }
    }

    if (order.qty > 0) {

        Order* rested = pool_.acquire(order.id, Side::Sell, order.price, order.qty, order.timestamp);

        asks_levels_[price_index].push_back(rested);

        if (order.id >= order_index_.size()) {

            order_index_.resize(order.id * 2, nullptr);
        }

        order_index_[order.id] = rested;

        if (price_index < best_ask_index_) {
            best_ask_index_ = price_index;
        }
    }

    return trades;
}

bool OrderBook::cancelOrder(OrderId id) {

    if (id >= order_index_.size()) {
        return false;
    }

    Order* o = order_index_[id];

    if (o == nullptr) {

        return false;
    }

    order_index_[id] = nullptr;

    int price_index = o->price - MIN_PRICE;

    if (o->side == Side::Buy) {

        PriceLevel& level = bids_levels_[price_index];

        level.remove(o);

        if (level.head == nullptr) {

            if (price_index == best_bid_index_) {
                while (best_bid_index_ >= 0 && bids_levels_[best_bid_index_].head == nullptr) {
                    --best_bid_index_;
                }
            }
        }
    } else {

        PriceLevel& level = asks_levels_[price_index];

        level.remove(o);

        if (level.head == nullptr) {

            if (price_index == best_ask_index_) {
                while (best_ask_index_ < static_cast<int>(NUM_LEVELS) && asks_levels_[best_ask_index_].head == nullptr) {
                    ++best_ask_index_;
                }
            }
        }
    }

    pool_.release(o);

    return true;
}

bool OrderBook::empty() const {
    return best_bid_index_ < 0 && best_ask_index_ >= static_cast<int>(NUM_LEVELS);
}

std::map<Price, std::list<Order>, std::greater<Price>> OrderBook::bids() const {

    std::map<Price, std::list<Order>, std::greater<Price>> result;

    for (int i = best_bid_index_; i >= 0; --i) {
        const PriceLevel& level = bids_levels_[i];
        if (level.head != nullptr) {
            Price price = i + MIN_PRICE;
            std::list<Order>& order_list = result[price];
            Order* current = level.head;
            while (current != nullptr) {
                order_list.push_back(*current);
                current = current->next;
            }
        }
    }
    return result;
}

std::map<Price, std::list<Order>> OrderBook::asks() const {

    std::map<Price, std::list<Order>> result;

    for (size_t i = best_ask_index_; i < NUM_LEVELS; ++i) {
        const PriceLevel& level = asks_levels_[i];
        if (level.head != nullptr) {
            Price price = i + MIN_PRICE;
            std::list<Order>& order_list = result[price];
            Order* current = level.head;
            while (current != nullptr) {
                order_list.push_back(*current);
                current = current->next;
            }
        }
    }
    return result;
}

}
