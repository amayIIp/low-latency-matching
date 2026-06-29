
#ifndef LOB_ORDER_BOOK_HPP
#define LOB_ORDER_BOOK_HPP

#include "order.hpp"

#include <atomic>

#include <vector>

#include <map>

#include <list>

namespace lob {

#ifdef PROVE_ZERO_ALLOC

extern std::atomic<bool> allocations_forbidden;
#endif

struct PriceLevel {

    Order* head = nullptr;

    Order* tail = nullptr;

    Qty total_qty = 0;

    void push_back(Order* o) {

        o->prev = tail;

        o->next = nullptr;

        if (tail) {
            tail->next = o;
        } else {

            head = o;
        }

        tail = o;

        total_qty += o->qty;
    }

    void remove(Order* o) {

        if (o->prev) {
            o->prev->next = o->next;
        } else {

            head = o->next;
        }

        if (o->next) {
            o->next->prev = o->prev;
        } else {

            tail = o->prev;
        }

        total_qty -= o->qty;

        o->next = nullptr;
        o->prev = nullptr;
    }
};

class OrderPool {
private:

    std::vector<Order> storage_;

    Order* free_list_head_ = nullptr;

public:

    OrderPool(size_t capacity) {

        storage_.resize(capacity);

        for (size_t i = 0; i + 1 < capacity; ++i) {
            storage_[i].next = &storage_[i + 1];
        }

        free_list_head_ = &storage_[0];
    }

    Order* acquire(OrderId id, Side side, Price price, Qty qty, uint64_t timestamp) {

        if (!free_list_head_) {
            return nullptr;
        }

        Order* o = free_list_head_;

        free_list_head_ = free_list_head_->next;

        o->id = id;
        o->side = side;
        o->price = price;
        o->qty = qty;
        o->timestamp = timestamp;

        o->next = nullptr;
        o->prev = nullptr;

        return o;
    }

    void release(Order* o) {

        o->next = free_list_head_;

        free_list_head_ = o;
    }
};

class OrderBook {
public:

    static constexpr Price MIN_PRICE = 8000;
    static constexpr Price MAX_PRICE = 12000;

    static constexpr size_t NUM_LEVELS = MAX_PRICE - MIN_PRICE + 1;

    static constexpr size_t POOL_CAPACITY = 1100000;

    OrderBook();

    ~OrderBook();

    TradeVector addOrder(Order order);

    bool cancelOrder(OrderId id);

    bool empty() const;

    std::map<Price, std::list<Order>, std::greater<Price>> bids() const;

    std::map<Price, std::list<Order>> asks() const;

private:

    OrderPool pool_;

    std::vector<PriceLevel> bids_levels_;

    std::vector<PriceLevel> asks_levels_;

    std::vector<Order*> order_index_;

    int best_bid_index_ = -1;

    int best_ask_index_ = NUM_LEVELS;

    __attribute__((always_inline)) inline TradeVector addBuyOrder(Order order);
    __attribute__((always_inline)) inline TradeVector addSellOrder(Order order);
};

}

#endif
