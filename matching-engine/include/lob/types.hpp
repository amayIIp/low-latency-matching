// Guard this header file from being included multiple times to avoid duplicate declarations.
#ifndef LOB_TYPES_HPP
#define LOB_TYPES_HPP

// Include standard integer types to guarantee exact byte sizes across compilers.
#include <cstdint>
// Include string library for conversion helpers.
#include <string>
// Include array to construct a fixed-size buffer for trade logs.
#include <array>
// Include vector to compare with std::vector in testing.
#include <vector>

// Keep all our matching engine type definitions grouped together inside the 'lob' namespace.
namespace lob {

// Define OrderId as an unsigned 64-bit integer, providing a huge space of positive-only IDs.
using OrderId = uint64_t;

// Define Price as a signed 64-bit integer. Cents or ticks are integers (e.g. $10.50 represented as 105000 with a tick multiplier of 10,000). Signed allows representation of relative pricing differentials if needed.
using Price = int64_t;

// Define Qty as a signed 64-bit integer representing order quantities in units, shares, or lots.
using Qty = int64_t;

// Define an enumeration class for buy/sell directions, using uint8_t for minimal memory footprint.
enum class Side : uint8_t {
    Buy,  // Represents an order to buy an asset.
    Sell  // Represents an order to sell an asset.
};

// Convert Side to string for printing, optimized by inlining the function.
inline std::string side_to_string(Side side) {
    // Branch on the enum value
    switch (side) {
        // Return string representations matching the enum state
        case Side::Buy:
            return "BUY";
        case Side::Sell:
            return "SELL";
        default:
            return "UNKNOWN";
    }
}

// Define the Trade structure representing execution matches between orders.
struct Trade {
    // The OrderId of the resting order that was already in the book (maker) (8 bytes).
    OrderId maker_id;
    // The OrderId of the incoming order that triggered the match (taker) (8 bytes).
    OrderId taker_id;
    // The price at which the match occurred (8 bytes).
    Price price;
    // The amount of quantity filled in this match (8 bytes).
    Qty qty;

    // Default constructor.
    Trade() : maker_id(0), taker_id(0), price(0), qty(0) {}

    // Parameterized constructor to initialize trade records.
    Trade(OrderId m_id, OrderId t_id, Price p, Qty q)
        : maker_id(m_id), taker_id(t_id), price(p), qty(q) {} // Assign input parameters

    // Comparison operator to check if two Trade objects are identical. Required for testing checks.
    bool operator==(const Trade& other) const {
        // Return true only if all fields of the trade match exactly.
        return maker_id == other.maker_id &&
               taker_id == other.taker_id &&
               price == other.price &&
               qty == other.qty;
    }
};

// Define a custom, fixed-capacity vector to hold Trade records with zero dynamic heap allocations.
// This is critical for meeting low-latency requirements since it operates purely on the stack.
class TradeVector {
private:
    // The maximum number of trades we can record from a single incoming order.
    static constexpr size_t MAX_TRADES = 64;
    // Stack-allocated array to store the Trade objects.
    std::array<Trade, MAX_TRADES> data_;
    // The current number of active trades stored in our array.
    size_t size_ = 0;

public:
    // Default constructor.
    TradeVector() = default;

    // Emplace a new Trade record directly into our array at the current size index.
    void emplace_back(OrderId maker, OrderId taker, Price p, Qty q) {
        // Check if we still have available space in our fixed-capacity array.
        if (size_ < MAX_TRADES) {
            // Assign the trade details at the current index and increment size.
            data_[size_++] = Trade(maker, taker, p, q);
        }
    }

    // Return the number of active trades.
    size_t size() const {
        return size_;
    }

    // Return true if there are no trades.
    bool empty() const {
        return size_ == 0;
    }

    // Access trade at index (const version).
    const Trade& operator[](size_t index) const {
        return data_[index];
    }

    // Return pointer to the start of the array data (for iteration support).
    const Trade* begin() const {
        return &data_[0];
    }

    // Return pointer to the end of the active data (for iteration support).
    const Trade* end() const {
        return &data_[size_];
    }
};

// Helper operator to compare standard std::vector<Trade> with our zero-allocation TradeVector.
// This is required so that differential testing assertions compile and execute correctly.
inline bool operator==(const std::vector<Trade>& lhs, const TradeVector& rhs) {
    // If sizes are different, they are not equal.
    if (lhs.size() != rhs.size()) {
        return false;
    }
    // Loop through each element to compare them.
    for (size_t i = 0; i < lhs.size(); ++i) {
        // If any pair is not equal, return false.
        if (!(lhs[i] == rhs[i])) {
            return false;
        }
    }
    // All checks passed; they are identical.
    return true;
}

// Inequality helper.
inline bool operator!=(const std::vector<Trade>& lhs, const TradeVector& rhs) {
    return !(lhs == rhs);
}

} // namespace lob

// End of the header guard condition
#endif // LOB_TYPES_HPP
