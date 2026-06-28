// Include the standard input/output stream library to log messages to the console
#include <iostream>
// Include the file stream library to output the latency report CSV file
#include <fstream>
// Include standard vector to manage collections of orders, latencies, and throughputs
#include <vector>
// Include chrono to measure high-resolution timestamps in nanoseconds and microseconds
#include <chrono>
// Include algorithm for std::sort and std::min
#include <algorithm>
// Include cmath to perform exponential and rounding operations for log-normal quantities
#include <cmath>
// Include random to generate deterministic mock market order sequences
#include <random>
// Include numeric to calculate sum and averages of throughputs
#include <numeric>
// Include the optimized order book header file
#include "lob/order_book.hpp"

// Generate a sequence of N realistic market orders using a random walk for prices and log-normal quantities
std::vector<lob::Order> generateRealisticOrders(uint32_t seed, size_t num_orders) {
    // Initialize standard Merseene Twister RNG engine with our seed
    std::mt19937 rng(seed);
    
    // Set a starting fair price tick level at 10000
    double fair_price = 10000.0;
    
    // Normal distribution for price offsets around the moving fair price (standard deviation of 15 ticks)
    std::normal_distribution<double> price_offset_dist(0.0, 15.0);
    
    // Uniform distribution to select Buy (0) or Sell (1) with equal probability
    std::uniform_int_distribution<int> side_dist(0, 1);
    
    // Uniform distribution for log quantities (giving log-normal style layout)
    std::uniform_real_distribution<double> log_qty_dist(0.0, 5.5); // exp(5.5) ~ 244 lots
    
    // Uniform distribution for the fair price random walk step (-1.0 to +1.0 ticks per order)
    std::uniform_real_distribution<double> walk_dist(-1.0, 1.0);

    // Vector to store our mock orders
    std::vector<lob::Order> orders;
    // Pre-allocate the vector capacity to prevent dynamic growth overhead
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
        
        // For Buy orders, bias slightly below the fair price
        if (side == lob::Side::Buy) {
            offset -= 2.0;
        } else {
            // For Sell orders, bias slightly above the fair price
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

        // Generate quantity using exponential function to get a log-normal distribution
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
    // Define the warmup count (first 10,000 orders will be discarded from latency statistics)
    const size_t warmup_orders = 10000;
    
    // Pre-generate the load into memory to avoid measuring RNG computation times
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
    // Activate the zero-allocation check flag to crash on any dynamic new/malloc call
    lob::allocations_forbidden.store(true, std::memory_order_relaxed);
#endif

    // Loop to execute and measure the latency of each order in the hot path
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
    std::cout << "Latency measurement completed. Sorting data..." << std::endl;

    // Sort latencies in ascending order to extract percentiles
    std::sort(latencies.begin(), latencies.end());

    // Calculate count size
    size_t n = latencies.size();
    // Fetch p50, p99, and p99.9 latency times
    double p50 = latencies[static_cast<size_t>(n * 0.50)];
    double p99 = latencies[static_cast<size_t>(n * 0.99)];
    double p999 = latencies[static_cast<size_t>(n * 0.999)];

    // Print calculated percentiles to console
    std::cout << "Latency Percentiles:\n";
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

    // Define the target filepath for the report output (persistent drive path)
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
    } else {
        // Log file open error
        std::cerr << "Error: Could not save CSV report to: " << csv_path << std::endl;
    }

    // Log start of throughput runs
    std::cout << "Starting 5 throughput execution runs to measure consistency..." << std::endl;

    // Vector to collect throughput values from each run
    std::vector<double> throughputs;
    throughputs.reserve(5);

    // Loop to perform 5 separate throughput measurement runs
    for (int run = 1; run <= 5; ++run) {
        // Construct a fresh OrderBook to ensure a clean state
        lob::OrderBook run_book;
        
        // Record high-resolution clock before processing the entire batch
        auto start_run = std::chrono::high_resolution_clock::now();
        
        // Loop to process all 1,000,000 orders sequentially
        for (const auto& o : orders) {
            run_book.addOrder(o);
        }
        
        // Record high-resolution clock after processing the entire batch
        auto end_run = std::chrono::high_resolution_clock::now();
        
        // Calculate the elapsed run duration in seconds
        double run_duration_s = std::chrono::duration_cast<std::chrono::microseconds>(end_run - start_run).count() / 1000000.0;
        
        // Calculate throughput: orders processed per second
        double tp = static_cast<double>(num_orders) / run_duration_s;
        // Store the result
        throughputs.push_back(tp);
        
        // Output the result of the current run
        std::cout << "  Run " << run << " Throughput: " << tp << " orders/second (elapsed: " << run_duration_s << " s)\n";
    }

    // Compute statistical summary of the throughput runs
    double sum = std::accumulate(throughputs.begin(), throughputs.end(), 0.0);
    double mean = sum / throughputs.size();
    double min_tp = *std::min_element(throughputs.begin(), throughputs.end());
    double max_tp = *std::max_element(throughputs.begin(), throughputs.end());

    // Print summary stats
    std::cout << "\nThroughput Summary:\n";
    std::cout << "  Average Throughput: " << mean << " orders/sec\n";
    std::cout << "  Minimum Throughput: " << min_tp << " orders/sec\n";
    std::cout << "  Maximum Throughput: " << max_tp << " orders/sec\n";

    // Return 0 indicating successful completion
    return 0;
}
