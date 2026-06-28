// Guard this header file from being included multiple times in the same compilation unit to avoid duplicate declarations.
#ifndef LOB_TYPES_HPP
#define LOB_TYPES_HPP

// Include standard integer types to guarantee exact byte sizes across compilers and operating systems.
#include <cstdint>
// Include the string library to facilitate converting enumerations to human-readable strings.
#include <string>

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
// Placing this in types.hpp prevents duplicate definitions in reference and optimized books.
struct Trade {
    // The OrderId of the resting order that was already in the book (maker) (8 bytes).
    OrderId maker_id;
    // The OrderId of the incoming order that triggered the match (taker) (8 bytes).
    OrderId taker_id;
    // The price at which the match occurred (8 bytes).
    Price price;
    // The amount of quantity filled in this match (8 bytes).
    Qty qty;

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

} // namespace lob

// End of the header guard condition
#endif // LOB_TYPES_HPP
