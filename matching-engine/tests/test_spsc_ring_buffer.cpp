
#include <gtest/gtest.h>

#include "lob/spsc_ring_buffer.hpp"

#include <thread>

#include <atomic>

#include <vector>

#include <iostream>

#include <pthread.h>
#include <sched.h>

void pin_thread(int core_id) {

    cpu_set_t cpuset;

    CPU_ZERO(&cpuset);

    CPU_SET(core_id, &cpuset);

    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

TEST(SPSCRingBufferTests, BasicPushPop) {

    lob::SPSCRingBuffer<int, 4> buffer;

    EXPECT_TRUE(buffer.empty());

    EXPECT_TRUE(buffer.push(10));

    EXPECT_FALSE(buffer.empty());

    EXPECT_TRUE(buffer.push(20));
    EXPECT_TRUE(buffer.push(30));

    EXPECT_FALSE(buffer.push(40));

    int value = 0;

    EXPECT_TRUE(buffer.pop(value));
    EXPECT_EQ(value, 10);

    EXPECT_TRUE(buffer.pop(value));
    EXPECT_EQ(value, 20);

    EXPECT_TRUE(buffer.pop(value));
    EXPECT_EQ(value, 30);

    EXPECT_TRUE(buffer.empty());

    EXPECT_FALSE(buffer.pop(value));
}

TEST(SPSCRingBufferTests, WrapAroundCorrectness) {

    lob::SPSCRingBuffer<int, 4> buffer;

    EXPECT_TRUE(buffer.push(1));
    EXPECT_TRUE(buffer.push(2));
    EXPECT_TRUE(buffer.push(3));

    int value = 0;
    EXPECT_TRUE(buffer.pop(value));
    EXPECT_EQ(value, 1);

    EXPECT_TRUE(buffer.push(4));

    EXPECT_TRUE(buffer.pop(value));
    EXPECT_EQ(value, 2);
    EXPECT_TRUE(buffer.pop(value));
    EXPECT_EQ(value, 3);
    EXPECT_TRUE(buffer.pop(value));
    EXPECT_EQ(value, 4);

    EXPECT_TRUE(buffer.empty());
}

TEST(SPSCRingBufferTests, ConcurrentStressTest) {

    lob::SPSCRingBuffer<uint64_t, 1024> buffer;

    const uint64_t total_items = 5000000;

    std::atomic<bool> failure_detected{false};

    std::thread producer([&]() {

        pin_thread(1);

        for (uint64_t i = 1; i <= total_items; ++i) {

            while (!buffer.push(i)) {

                if (failure_detected.load(std::memory_order_relaxed)) {
                    return;
                }

                std::this_thread::yield();
            }
        }
    });

    std::thread consumer([&]() {

        pin_thread(2);

        uint64_t expected_value = 1;

        while (expected_value <= total_items) {
            uint64_t val = 0;

            if (buffer.pop(val)) {

                if (val != expected_value) {

                    std::cerr << "Error: FIFO sequence broken. Expected " << expected_value << ", got " << val << "\n";
                    failure_detected.store(true, std::memory_order_relaxed);
                    return;
                }

                expected_value++;
            } else {

                std::this_thread::yield();
            }
        }
    });

    producer.join();

    consumer.join();

    EXPECT_FALSE(failure_detected.load());
}
