// Include standard library utilities to output information to the console
#include <iostream>
// Include file stream library to output CSV latency reports
#include <fstream>
// Include vector library to hold preloaded orders and timed metrics
#include <vector>
// Include chrono library to perform high-resolution timing measurements
#include <chrono>
// Include algorithm for sorting and min elements
#include <algorithm>
// Include cmath for math calculations
#include <cmath>
// Include random to generate deterministic mock market flows
#include <random>
// Include numeric to sum elements
#include <numeric>
// Include thread library to manage the producer and consumer threads
#include <thread>
// Include atomic for atomic variables shared across threads
#include <atomic>
// Include the optimized order book header
#include "lob/order_book.hpp"
// Include our custom lock-free SPSC ring buffer header
#include "lob/spsc_ring_buffer.hpp"
// Include POSIX thread scheduler bindings for thread core pinning
#include <pthread.h>
#include <sched.h>

// Helper function to pin the current thread to a specific CPU core ID
void pin_thread(int core_id) {
    // Create a CPU set structure
    cpu_set_t cpuset;
    // Clear all cores from the set
    CPU_ZERO(&cpuset);
    // Add the target core_id to the set
    CPU_SET(core_id, &cpuset);
    // Bind the calling thread to the configured core set
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

// Generate a sequence of N realistic market orders
std::vector<lob::Order> generateRealisticOrders(uint32_t seed, size_t num_orders) {
    // Initialize Merseene Twister RNG engine
    std::mt19937 rng(seed);
    // Set a starting fair price tick level at 10000
    double fair_price = 10000.0;
    // Normal distribution for price offsets (standard deviation of 15 ticks)
    std::normal_distribution<double> price_offset_dist(0.0, 15.0);
    // Uniform distribution to select Side (Buy or Sell)
    std::uniform_int_distribution<int> side_dist(0, 1);
    // Uniform distribution for log quantities
    std::uniform_real_distribution<double> log_qty_dist(0.0, 5.5);
    // Uniform distribution for the fair price random walk step
    std::uniform_real_distribution<double> walk_dist(-1.0, 1.0);

    // Vector to store our mock orders
    std::vector<lob::Order> orders;
    orders.reserve(num_orders);
    
    // Set initial sequence timestamp
    uint64_t timestamp = 0;
    
    // Loop to construct N randomized orders
    for (size_t index = 0; index < num_orders; ++index) {
        // Step the fair price based on random walk
        fair_price += walk_dist(rng);
        
        // Clamp the fair price to stay within a reasonable boundary
        if (fair_price < 8500.0) {
            fair_price = 8500.0;
        }
        if (fair_price > 11500.0) {
            fair_price = 11500.0;
        }

        // Sample a side from distribution
        lob::Side side = (side_dist(rng) == 0) ? lob::Side::Buy : lob::Side::Sell;

        // Generate price offset around the moving fair price
        double offset = price_offset_dist(rng);
        
        // Bias offsets slightly based on side
        if (side == lob::Side::Buy) {
            offset -= 2.0;
        } else {
            offset += 2.0;
        }
        
        // Compute the final price tick rounded to the nearest integer
        lob::Price price = static_cast<lob::Price>(std::round(fair_price + offset));
        
        // Clamp the price to fit inside our engine's pre-allocated ladder range [8000, 12000]
        if (price < 8000) {
            price = 8000;
        }
        if (price > 12000) {
            price = 12000;
        }

        // Generate quantity using exponential function
        lob::Qty qty = static_cast<lob::Qty>(std::round(std::exp(log_qty_dist(rng)))) + 1;

        // Construct and insert the order into the vector
        orders.emplace_back(index + 1, side, price, qty, ++timestamp);
    }
    
    // Return the pre-generated order list
    return orders;
}

// Main execution entry point for our benchmarking utility
int main() {
    // Log the initiation of the benchmark
    std::cout << "Starting matching-engine benchmark framework..." << std::endl;

    // Define the total number of orders (1 million)
    const size_t num_orders = 1000000;
    // Define the warmup count (first 10,000 orders)
    const size_t warmup_orders = 10000;
    
    // Pre-generate the load into memory
    auto orders = generateRealisticOrders(12345, num_orders);
    
    // Log complete load generation
    std::cout << "Pre-generated " << num_orders << " realistic orders. Initiating OrderBook..." << std::endl;

    // Construct the optimized OrderBook instance
    lob::OrderBook book;

    // Process warmup orders to load CPU caches and populate pools
    for (size_t i = 0; i < warmup_orders; ++i) {
        book.addOrder(orders[i]);
    }

    // Pre-allocate a vector to hold latency times for each non-warmup order
    std::vector<double> latencies;
    latencies.reserve(num_orders - warmup_orders);

#ifdef PROVE_ZERO_ALLOC
    // Activate the zero-allocation check flag
    lob::allocations_forbidden.store(true, std::memory_order_relaxed);
#endif

    // Loop to execute and measure the latency of each order in the hot path (Direct Call)
    for (size_t i = warmup_orders; i < num_orders; ++i) {
        // Record high-resolution clock before processing
        auto start = std::chrono::high_resolution_clock::now();
        
        // Execute the matching logic
        book.addOrder(orders[i]);
        
        // Record high-resolution clock after processing
        auto end = std::chrono::high_resolution_clock::now();
        
        // Compute elapsed time in nanoseconds
        double duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        
        // Store the result
        latencies.push_back(duration_ns);
    }

#ifdef PROVE_ZERO_ALLOC
    // Deactivate the allocation restriction flag
    lob::allocations_forbidden.store(false, std::memory_order_relaxed);
#endif

    // Log that latency timing is complete
    std::cout << "Direct Call Latency measurement completed. Sorting data..." << std::endl;

    // Sort latencies in ascending order to extract percentiles
    std::sort(latencies.begin(), latencies.end());

    // Calculate count size
    size_t n = latencies.size();
    // Fetch p50, p99, and p99.9 latency times
    double p50 = latencies[static_cast<size_t>(n * 0.50)];
    double p99 = latencies[static_cast<size_t>(n * 0.99)];
    double p999 = latencies[static_cast<size_t>(n * 0.999)];

    // Print calculated percentiles to console
    std::cout << "Direct Call Latency Percentiles:\n";
    std::cout << "  50th Percentile (Median): " << p50 << " ns\n";
    std::cout << "  99th Percentile:          " << p99 << " ns\n";
    std::cout << "  99.9th Percentile:        " << p999 << " ns\n";

    // Setup histogram bins: 10ns step from 0ns to 5000ns
    const double bin_step = 10.0;
    const size_t num_bins = 500;
    // Array to hold counts for each bin
    std::vector<size_t> bins(num_bins, 0);
    // Counter for outlier latencies that exceed 5000ns
    size_t outliers = 0;

    // Iterate through all latency samples to assign them to bins
    for (double latency : latencies) {
        // Calculate the target bin index
        size_t bin_index = static_cast<size_t>(latency / bin_step);
        // If the index falls inside our histogram range...
        if (bin_index < num_bins) {
            // Increment that bin's count
            bins[bin_index]++;
        } else {
            // Otherwise increment the outlier count
            outliers++;
        }
    }

    // Define the target filepath for the report output
    const std::string csv_path = "/mnt/d/Low Latency Matching Engine/matching-engine/docs/latency_report.csv";
    // Open the CSV file for writing
    std::ofstream csv_file(csv_path);
    // If the file opened successfully...
    if (csv_file.is_open()) {
        // Write the percentiles header and values
        csv_file << "Percentile,Latency_ns\n";
        csv_file << "p50," << p50 << "\n";
        csv_file << "p99," << p99 << "\n";
        csv_file << "p999," << p999 << "\n\n";

        // Write the histogram table header
        csv_file << "Bucket_Lower_ns,Bucket_Upper_ns,Count\n";
        // Loop to write each bin row
        for (size_t i = 0; i < num_bins; ++i) {
            csv_file << (i * bin_step) << "," << ((i + 1) * bin_step) << "," << bins[i] << "\n";
        }
        // Write the final outlier bucket row
        csv_file << "5000,Infinity," << outliers << "\n";
        // Close the file stream
        csv_file.close();
        // Log output success
        std::cout << "Saved latency and histogram report to: " << csv_path << std::endl;
    }

    // Log start of throughput runs
    std::cout << "\n--- DIRECT CALL THROUGHPUT BENCHMARK (5 Runs) ---\n";

    // Vector to collect throughput values from each run
    std::vector<double> direct_throughputs;
    direct_throughputs.reserve(5);

    // Loop to perform 5 separate throughput measurement runs (Direct Call)
    for (int run = 1; run <= 5; ++run) {
        // Construct a fresh OrderBook to ensure a clean state
        lob::OrderBook run_book;
        
        // Record high-resolution clock before processing the entire batch
        auto start_run = std::chrono::high_resolution_clock::now();
        
        // Loop to process all 1,000,000 orders sequentially
        for (const auto& o : orders) {
            run_book.addOrder(o);
        }
        
        // Record high-resolution clock after processing
        auto end_run = std::chrono::high_resolution_clock::now();
        
        // Calculate the elapsed run duration in seconds
        double run_duration_s = std::chrono::duration_cast<std::chrono::microseconds>(end_run - start_run).count() / 1000000.0;
        
        // Calculate throughput
        double tp = static_cast<double>(num_orders) / run_duration_s;
        direct_throughputs.push_back(tp);
        
        // Output result
        std::cout << "  Run " << run << " Throughput: " << tp << " orders/second (elapsed: " << run_duration_s << " s)\n";
    }

    // Compute statistical summary of the direct throughput runs
    double direct_sum = std::accumulate(direct_throughputs.begin(), direct_throughputs.end(), 0.0);
    double direct_mean = direct_sum / direct_throughputs.size();

    // Log start of concurrent pipelined throughput runs
    std::cout << "\n--- PIPELINED INGESTION THROUGHPUT BENCHMARK (5 Runs) ---\n";

    // Vector to collect pipelined throughput values from each run
    std::vector<double> pipelined_throughputs;
    pipelined_throughputs.reserve(5);

    // Loop to perform 5 separate pipelined throughput measurement runs
    for (int run = 1; run <= 5; ++run) {
        // Shared ring buffer with capacity 1024 elements (32 KB, fits perfectly in L1 cache)
        lob::SPSCRingBuffer<lob::Order, 1024> queue;
        // Construct a fresh OrderBook
        lob::OrderBook pipeline_book;
        
        // Atomic start signal to synchronize threads
        std::atomic<bool> start_signal{false};
        // Atomic counter to track consumer progress
        std::atomic<size_t> processed_count{0};
        
        // Spawn the consumer thread (pinned to core 2)
        std::thread consumer_thread([&]() {
            // Pin the consumer thread to CPU core 2
            pin_thread(2);
            
            // Wait until the start signal is set by the main thread
            while (!start_signal.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            
            // Local counter
            size_t count = 0;
            // Variable to hold popped order
            lob::Order order;
            // Process orders until 1,000,000 orders are popped and matched
            while (count < num_orders) {
                // If an order was successfully popped...
                if (queue.pop(order)) {
                    // Match the order
                    pipeline_book.addOrder(order);
                    // Increment the count
                    count++;
                } else {
                    // Spin-wait with pause instruction to prevent context switching
                    #if defined(__x86_64__) || defined(_M_X64)
                    asm volatile("pause" ::: "memory");
                    #else
                    std::this_thread::yield();
                    #endif
                }
            }
            // Update the shared atomic counter to signal completion
            processed_count.store(count, std::memory_order_release);
        });

        // Spawn the producer thread (pinned to core 1)
        std::thread producer_thread([&]() {
            // Pin the producer thread to CPU core 1
            pin_thread(1);
            
            // Wait until the start signal is set by the main thread
            while (!start_signal.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            
            // Push all pre-generated orders into the ring buffer
            for (const auto& order : orders) {
                // Keep retrying if the queue is full
                while (!queue.push(order)) {
                    #if defined(__x86_64__) || defined(_M_X64)
                    asm volatile("pause" ::: "memory");
                    #else
                    std::this_thread::yield();
                    #endif
                }
            }
        });

        // Wait to ensure thread startup and core pinning are complete before timing
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // Record high-resolution clock before starting the run
        auto start_run = std::chrono::high_resolution_clock::now();
        // Signal both threads to begin execution
        start_signal.store(true, std::memory_order_release);

#ifdef PROVE_ZERO_ALLOC
        // Disable any dynamic memory allocations during the hot pipeline matching path
        lob::allocations_forbidden.store(true, std::memory_order_relaxed);
#endif

        // Wait until the consumer thread has processed all 1,000,000 orders
        while (processed_count.load(std::memory_order_acquire) < num_orders) {
            #if defined(__x86_64__) || defined(_M_X64)
            asm volatile("pause" ::: "memory");
            #else
            std::this_thread::yield();
            #endif
        }

#ifdef PROVE_ZERO_ALLOC
        // Re-enable memory allocations
        lob::allocations_forbidden.store(false, std::memory_order_relaxed);
#endif

        // Record high-resolution clock after consumer completes
        auto end_run = std::chrono::high_resolution_clock::now();

        // Join both execution threads
        producer_thread.join();
        consumer_thread.join();

        // Calculate the elapsed run duration in seconds
        double run_duration_s = std::chrono::duration_cast<std::chrono::microseconds>(end_run - start_run).count() / 1000000.0;
        // Calculate pipelined throughput
        double tp = static_cast<double>(num_orders) / run_duration_s;
        pipelined_throughputs.push_back(tp);

        // Output result
        std::cout << "  Run " << run << " Pipelined Throughput: " << tp << " orders/second (elapsed: " << run_duration_s << " s)\n";
    }

    // Compute statistical summary of the pipelined throughput runs
    double pipelined_sum = std::accumulate(pipelined_throughputs.begin(), pipelined_throughputs.end(), 0.0);
    double pipelined_mean = pipelined_sum / pipelined_throughputs.size();

    // Print summary comparison
    std::cout << "\nBenchmark Summary Comparison:\n";
    std::cout << "  Direct Call Baseline Average Throughput: " << direct_mean << " orders/sec\n";
    std::cout << "  Pipelined Ingestion (SPSC) Average Throughput: " << pipelined_mean << " orders/sec\n";

    // Return 0 indicating successful completion
    return 0;
}
