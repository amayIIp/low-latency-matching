
#ifndef LOB_SPSC_RING_BUFFER_HPP
#define LOB_SPSC_RING_BUFFER_HPP

#include <atomic>

#include <array>

#include <cstddef>

namespace lob {

template <typename T, size_t Capacity>
class SPSCRingBuffer {

    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");

    alignas(64) std::atomic<size_t> head_{0};

    alignas(64) std::atomic<size_t> tail_{0};

    alignas(64) std::array<T, Capacity> buffer_;

    alignas(64) size_t tail_cached_ = 0;

    alignas(64) size_t head_cached_ = 0;

public:

    SPSCRingBuffer() : head_(0), tail_(0), tail_cached_(0), head_cached_(0) {}

    ~SPSCRingBuffer() = default;

    bool push(const T& item) {

        size_t head = head_.load(std::memory_order_relaxed);

        size_t next_head = (head + 1) & (Capacity - 1);

        if (next_head == tail_cached_) {

            tail_cached_ = tail_.load(std::memory_order_acquire);

            if (next_head == tail_cached_) {

                return false;
            }
        }

        buffer_[head] = item;

        head_.store(next_head, std::memory_order_release);

        return true;
    }

    bool pop(T& item) {

        size_t tail = tail_.load(std::memory_order_relaxed);

        if (tail == head_cached_) {

            head_cached_ = head_.load(std::memory_order_acquire);

            if (tail == head_cached_) {

                return false;
            }
        }

        item = buffer_[tail];

        tail_.store((tail + 1) & (Capacity - 1), std::memory_order_release);

        return true;
    }

    bool empty() const {

        return head_.load(std::memory_order_relaxed) == tail_.load(std::memory_order_relaxed);
    }
};

}

#endif
