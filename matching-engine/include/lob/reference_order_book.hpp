
#ifndef LOB_REFERENCE_ORDER_BOOK_HPP
#define LOB_REFERENCE_ORDER_BOOK_HPP

#include "order.hpp"

#include <list>

#include <map>

#include <unordered_map>

#include <vector>

namespace lob {

struct ReferenceOrderLocation {

    Side side;

    Price price;

    std::list<Order>::iterator list_it;
};

class ReferenceOrderBook {
public:

    ReferenceOrderBook();

    ~ReferenceOrderBook();

    std::vector<Trade> addOrder(Order order);

    bool cancelOrder(OrderId id);

    bool empty() const;

    const std::map<Price, std::list<Order>, std::greater<Price>>& bids() const {
        return bids_;
    }

    const std::map<Price, std::list<Order>>& asks() const {
        return asks_;
    }

private:

    std::map<Price, std::list<Order>, std::greater<Price>> bids_;

    std::map<Price, std::list<Order>> asks_;

    std::unordered_map<OrderId, ReferenceOrderLocation> order_index_;
};

}

#endif
