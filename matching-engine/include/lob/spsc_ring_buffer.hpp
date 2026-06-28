// Guard this header file from being included multiple times to avoid duplicate definitions.
#ifndef LOB_SPSC_RING_BUFFER_HPP
#define LOB_SPSC_RING_BUFFER_HPP

// Include atomic library to support lock-free multi-threaded synchronization.
#include <atomic>
// Include array library to declare the fixed-size ring buffer array.
#include <array>
// Include cstddef for size_t.
#include <cstddef>

// Place the ring buffer in the 'lob' namespace.
namespace lob {

// Define a Single-Producer Single-Consumer (SPSC) lock-free ring buffer template class.
// Capacity must be a power of two to optimize index wrapping using bitwise AND operations.
template <typename T, size_t Capacity>
class SPSCRingBuffer {
    // Assert at compile time that the capacity is indeed a power of two.
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");

    // Align the head index to a 64-byte boundary to prevent false sharing with other cache lines.
    // head_ represents the next index where the producer thread will write.
    alignas(64) std::atomic<size_t> head_{0};

    // Align the tail index to a 64-byte boundary to prevent false sharing with head_ or the buffer array.
    // tail_ represents the next index from which the consumer thread will read.
    alignas(64) std::atomic<size_t> tail_{0};

    // Align the buffer array to prevent sharing cache lines with head_ or tail_.
    // This contiguous storage holds the items.
    alignas(64) std::array<T, Capacity> buffer_;

public:
    // Constructor. Initialized with head and tail set to zero.
    SPSCRingBuffer() : head_(0), tail_(0) {}

    // Destructor.
    ~SPSCRingBuffer() = default;

    // Push an item into the ring buffer. Called exclusively by the single producer thread.
    // Returns true if the item was successfully added, or false if the buffer is full.
    bool push(const T& item) {
        // Load the producer's current head index using relaxed ordering (no sync needed for producer's own index).
        size_t head = head_.load(std::memory_order_relaxed);
        // Calculate the next candidate write index using bitwise AND for fast wrapping.
        size_t next_head = (head + 1) & (Capacity - 1);
        
        // If the next write index matches the consumer's current tail, the buffer is full.
        // We use acquire ordering on the load of tail_ to ensure we see the consumer's reads.
        if (next_head == tail_.load(std::memory_order_acquire)) {
            // Buffer is full; return false to signal that the item was not added.
            return false;
        }
        
        // Write the item into the buffer array at the current head index slot.
        buffer_[head] = item;
        // Store the updated head index using release ordering.
        // This ensures the item write becomes visible to the consumer thread when it loads head_.
        head_.store(next_head, std::memory_order_release);
        // Return true to indicate the item was successfully pushed.
        return true;
    }

    // Pop an item from the ring buffer. Called exclusively by the single consumer thread.
    // Returns true if an item was successfully retrieved, or false if the buffer is empty.
    bool pop(T& item) {
        // Load the consumer's current tail index using relaxed ordering.
        size_t tail = tail_.load(std::memory_order_relaxed);
        
        // If the current tail index equals the producer's current head, the buffer is empty.
        // We use acquire ordering on the load of head_ to ensure we see the producer's writes.
        if (tail == head_.load(std::memory_order_acquire)) {
            // Buffer is empty; return false.
            return false;
        }
        
        // Copy the item from the buffer array at the current tail index slot.
        item = buffer_[tail];
        // Store the updated tail index using release ordering.
        // This makes the slot available to the producer again.
        tail_.store((tail + 1) & (Capacity - 1), std::memory_order_release);
        // Return true to indicate an item was successfully popped.
        return true;
    }

    // Check if the ring buffer is empty (helper method).
    bool empty() const {
        // Buffer is empty if head matches tail (using relaxed ordering for status check).
        return head_.load(std::memory_order_relaxed) == tail_.load(std::memory_order_relaxed);
    }
};

} // namespace lob

// End of header guard condition
#endif // LOB_SPSC_RING_BUFFER_HPP
