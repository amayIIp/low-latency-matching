// Guard this header file from being included multiple times in the same file compilation, which would cause duplicate definition errors.
#ifndef LOB_TYPES_HPP
#define LOB_TYPES_HPP

// Include the standard integer library to use fixed-size integer types (like uint64_t) for precision and predictability.
#include <cstdint>
// Include the string library to allow naming or formatting sides if needed.
#include <string>

// Place all our Limit Order Book (LOB) types in a namespace named 'lob' to prevent name collisions with other libraries.
namespace lob {

// Define OrderId as an alias for a 64-bit unsigned integer. Unsigned means it only holds positive numbers, giving us a huge range of unique IDs.
using OrderId = uint64_t;

// Define Price as an alias for a 64-bit unsigned integer. In low-latency trading, we avoid floating-point types (like double) because they introduce rounding errors and slow down calculations. Instead, we use fixed-point arithmetic (e.g., representing $10.50 as 105000 with a multiplier of 10,000).
using Price = uint64_t;

// Define Qty (Quantity) as an alias for a 64-bit unsigned integer to represent the amount of shares or contracts.
using Qty = uint64_t;

// Define an enumeration class to represent the buy or sell side of an order. We use a uint8_t underlying type to minimize memory footprint to 1 byte.
enum class Side : uint8_t {
    Buy,  // Represents a bid order where a trader wants to buy an asset.
    Sell  // Represents an ask order where a trader wants to sell an asset.
};

// Inline helper function to convert Side enum to a human-readable string representation. Inline helps compiler optimize away function call overhead.
inline std::string side_to_string(Side side) {
    // Check if the side is Buy using a switch-case statement
    switch (side) {
        // If Side is Buy, return the string "BUY"
        case Side::Buy:
            return "BUY";
        // If Side is Sell, return the string "SELL"
        case Side::Sell:
            return "SELL";
        // Default fallback (should not be reached, but good for safety)
        default:
            return "UNKNOWN";
    }
}

} // namespace lob

// End of the header guard condition
#endif // LOB_TYPES_HPP
