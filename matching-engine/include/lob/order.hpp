
#ifndef LOB_ORDER_HPP
#define LOB_ORDER_HPP

#include "types.hpp"

namespace lob {

struct Order {

    OrderId id;

    Side side;

    Price price;

    Qty qty;

    uint64_t timestamp;

    Order* next = nullptr;

    Order* prev = nullptr;

    Order()
        : id(0), side(Side::Buy), price(0), qty(0), timestamp(0), next(nullptr), prev(nullptr) {}

    Order(OrderId o_id, Side o_side, Price o_price, Qty o_qty, uint64_t o_ts)
        : id(o_id), side(o_side), price(o_price), qty(o_qty), timestamp(o_ts), next(nullptr), prev(nullptr) {}

    bool operator==(const Order& other) const {

        return id == other.id &&
               side == other.side &&
               price == other.price &&
               qty == other.qty &&
               timestamp == other.timestamp;
    }
};

}

#endif
