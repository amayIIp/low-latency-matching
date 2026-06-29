
#ifndef LOB_TYPES_HPP
#define LOB_TYPES_HPP

#include <cstdint>

#include <string>

#include <array>

#include <vector>

namespace lob {

using OrderId = uint64_t;

using Price = int64_t;

using Qty = int64_t;

enum class Side : uint8_t {
    Buy,
    Sell
};

inline std::string side_to_string(Side side) {

    switch (side) {

        case Side::Buy:
            return "BUY";
        case Side::Sell:
            return "SELL";
        default:
            return "UNKNOWN";
    }
}

struct Trade {

    OrderId maker_id;

    OrderId taker_id;

    Price price;

    Qty qty;

    Trade() : maker_id(0), taker_id(0), price(0), qty(0) {}

    Trade(OrderId m_id, OrderId t_id, Price p, Qty q)
        : maker_id(m_id), taker_id(t_id), price(p), qty(q) {}

    bool operator==(const Trade& other) const {

        return maker_id == other.maker_id &&
               taker_id == other.taker_id &&
               price == other.price &&
               qty == other.qty;
    }
};

class TradeVector {
private:

    static constexpr size_t MAX_TRADES = 64;

    std::array<Trade, MAX_TRADES> data_;

    size_t size_ = 0;

public:

    TradeVector() = default;

    void emplace_back(OrderId maker, OrderId taker, Price p, Qty q) {

        if (size_ < MAX_TRADES) {

            data_[size_++] = Trade(maker, taker, p, q);
        }
    }

    size_t size() const {
        return size_;
    }

    bool empty() const {
        return size_ == 0;
    }

    const Trade& operator[](size_t index) const {
        return data_[index];
    }

    const Trade* begin() const {
        return &data_[0];
    }

    const Trade* end() const {
        return &data_[size_];
    }
};

inline bool operator==(const std::vector<Trade>& lhs, const TradeVector& rhs) {

    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (size_t i = 0; i < lhs.size(); ++i) {

        if (!(lhs[i] == rhs[i])) {
            return false;
        }
    }

    return true;
}

inline bool operator!=(const std::vector<Trade>& lhs, const TradeVector& rhs) {
    return !(lhs == rhs);
}

}

#endif
