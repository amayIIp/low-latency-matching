// Include the GoogleTest framework to support defining test cases and assertions
#include <gtest/gtest.h>
// Include our custom lock-free SPSC ring buffer implementation
#include "lob/spsc_ring_buffer.hpp"
// Include thread library to manage concurrent execution paths
#include <thread>
// Include atomic to coordinate state flags between producer and consumer threads
#include <atomic>
// Include vector to manage lists of threads or test data
#include <vector>
// Include iostream to log test progress to console
#include <iostream>
// Include standard POSIX scheduling headers for CPU core pinning
#include <pthread.h>
#include <sched.h>

// Helper function to pin the calling thread to a specific CPU core
// This prevents the OS scheduler from migrating the thread, reducing latency spikes.
void pin_thread(int core_id) {
    // Create a CPU core set structure
    cpu_set_t cpuset;
    // Clear all cores from the set
    CPU_ZERO(&cpuset);
    // Add the target core_id to the set
    CPU_SET(core_id, &cpuset);
    // Set the affinity of the calling thread to the specified core
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

// Unit test verifying basic SPSC ring buffer push and pop operations
TEST(SPSCRingBufferTests, BasicPushPop) {
    // Instantiate a ring buffer of capacity 4 (must be power of two)
    lob::SPSCRingBuffer<int, 4> buffer;

    // Verify it starts empty
    EXPECT_TRUE(buffer.empty());

    // Push first item (10)
    EXPECT_TRUE(buffer.push(10));
    // Verify it is no longer empty
    EXPECT_FALSE(buffer.empty());

    // Push more items to fill it near capacity
    EXPECT_TRUE(buffer.push(20));
    EXPECT_TRUE(buffer.push(30));

    // Try to push a 4th item. Since capacity is 4, head wrapping logic allows at most capacity-1 items (3 items) active
    // to distinguish between full and empty states. Thus, the 4th push should fail.
    EXPECT_FALSE(buffer.push(40));

    // Declare a variable to receive the popped value
    int value = 0;
    // Pop the first item and verify it is 10 (FIFO order)
    EXPECT_TRUE(buffer.pop(value));
    EXPECT_EQ(value, 10);

    // Pop the second item
    EXPECT_TRUE(buffer.pop(value));
    EXPECT_EQ(value, 20);

    // Pop the third item
    EXPECT_TRUE(buffer.pop(value));
    EXPECT_EQ(value, 30);

    // Verify the buffer is now empty
    EXPECT_TRUE(buffer.empty());

    // Verify popping from an empty buffer returns false
    EXPECT_FALSE(buffer.pop(value));
}

// Unit test verifying correct wrap-around indexing behavior of the ring buffer
TEST(SPSCRingBufferTests, WrapAroundCorrectness) {
    // Instantiate a ring buffer of capacity 4
    lob::SPSCRingBuffer<int, 4> buffer;

    // Fill the buffer to its maximum active size (3 items)
    EXPECT_TRUE(buffer.push(1));
    EXPECT_TRUE(buffer.push(2));
    EXPECT_TRUE(buffer.push(3));

    // Pop one item out to free up a slot
    int value = 0;
    EXPECT_TRUE(buffer.pop(value));
    EXPECT_EQ(value, 1);

    // Push another item (4). This triggers internal head index wrapping around the capacity boundary
    EXPECT_TRUE(buffer.push(4));

    // Pop remaining items and verify they are returned in correct FIFO sequence
    EXPECT_TRUE(buffer.pop(value));
    EXPECT_EQ(value, 2);
    EXPECT_TRUE(buffer.pop(value));
    EXPECT_EQ(value, 3);
    EXPECT_TRUE(buffer.pop(value));
    EXPECT_EQ(value, 4);

    // Verify buffer is empty
    EXPECT_TRUE(buffer.empty());
}

// Sustained stress test to run the SPSC buffer with concurrent threads
// Verifies no race conditions, no data corruption, and strictly ordered FIFO transfer.
TEST(SPSCRingBufferTests, ConcurrentStressTest) {
    // Instantiate a ring buffer of capacity 1024
    lob::SPSCRingBuffer<uint64_t, 1024> buffer;
    
    // Total number of items to transfer between threads during the stress test
    const uint64_t total_items = 5000000; // 5 million items
    
    // Shared atomic flag to notify threads of completion or failures
    std::atomic<bool> failure_detected{false};

    // Spawn the producer thread
    std::thread producer([&]() {
        // Pin the producer thread to CPU core 1
        pin_thread(1);
        
        // Loop to push items from 1 up to total_items
        for (uint64_t i = 1; i <= total_items; ++i) {
            // Keep spinning/retrying if the buffer is full
            while (!buffer.push(i)) {
                // If the consumer detected an error, exit immediately to prevent hang
                if (failure_detected.load(std::memory_order_relaxed)) {
                    return;
                }
                // Yield CPU control slightly to prevent aggressive thread lock
                std::this_thread::yield();
            }
        }
    });

    // Spawn the consumer thread
    std::thread consumer([&]() {
        // Pin the consumer thread to CPU core 2
        pin_thread(2);
        
        // Track the next value we expect to receive (starting at 1)
        uint64_t expected_value = 1;
        
        // Loop until we have successfully received all items
        while (expected_value <= total_items) {
            uint64_t val = 0;
            // Attempt to pop an item from the buffer
            if (buffer.pop(val)) {
                // Verify the item value matches the expected FIFO sequence
                if (val != expected_value) {
                    // Log error and trigger failure flag
                    std::cerr << "Error: FIFO sequence broken. Expected " << expected_value << ", got " << val << "\n";
                    failure_detected.store(true, std::memory_order_relaxed);
                    return;
                }
                // Increment expected value for the next check
                expected_value++;
            } else {
                // Yield CPU control slightly if the buffer is empty
                std::this_thread::yield();
            }
        }
    });

    // Wait for the producer thread to complete
    producer.join();
    // Wait for the consumer thread to complete
    consumer.join();

    // Assert that no sequence failure was flagged by the consumer
    EXPECT_FALSE(failure_detected.load());
}
