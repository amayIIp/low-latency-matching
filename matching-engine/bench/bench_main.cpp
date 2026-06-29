
#include <iostream>

#include <fstream>

#include <vector>

#include <chrono>

#include <algorithm>

#include <cmath>

#include <random>

#include <numeric>

#include <thread>

#include <atomic>

#include "lob/order_book.hpp"

#include "lob/spsc_ring_buffer.hpp"

#include <pthread.h>
#include <sched.h>

void pin_thread(int core_id) {

    cpu_set_t cpuset;

    CPU_ZERO(&cpuset);

    CPU_SET(core_id, &cpuset);

    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

std::vector<lob::Order> generateRealisticOrders(uint32_t seed, size_t num_orders) {

    std::mt19937 rng(seed);

    double fair_price = 10000.0;

    std::normal_distribution<double> price_offset_dist(0.0, 15.0);

    std::uniform_int_distribution<int> side_dist(0, 1);

    std::uniform_real_distribution<double> log_qty_dist(0.0, 5.5);

    std::uniform_real_distribution<double> walk_dist(-1.0, 1.0);

    std::vector<lob::Order> orders;
    orders.reserve(num_orders);

    uint64_t timestamp = 0;

    for (size_t index = 0; index < num_orders; ++index) {

        fair_price += walk_dist(rng);

        if (fair_price < 8500.0) {
            fair_price = 8500.0;
        }
        if (fair_price > 11500.0) {
            fair_price = 11500.0;
        }

        lob::Side side = (side_dist(rng) == 0) ? lob::Side::Buy : lob::Side::Sell;

        double offset = price_offset_dist(rng);

        if (side == lob::Side::Buy) {
            offset -= 2.0;
        } else {
            offset += 2.0;
        }

        lob::Price price = static_cast<lob::Price>(std::round(fair_price + offset));

        if (price < 8000) {
            price = 8000;
        }
        if (price > 12000) {
            price = 12000;
        }

        lob::Qty qty = static_cast<lob::Qty>(std::round(std::exp(log_qty_dist(rng)))) + 1;

        orders.emplace_back(index + 1, side, price, qty, ++timestamp);
    }

    return orders;
}

int main() {

    std::cout << "Starting matching-engine benchmark framework..." << std::endl;

    const size_t num_orders = 1000000;

    const size_t warmup_orders = 10000;

    auto orders = generateRealisticOrders(12345, num_orders);

    std::cout << "Pre-generated " << num_orders << " realistic orders. Initiating OrderBook..." << std::endl;

    lob::OrderBook book;

    for (size_t i = 0; i < warmup_orders; ++i) {
        book.addOrder(orders[i]);
    }

    std::vector<double> latencies;
    latencies.reserve(num_orders - warmup_orders);

#ifdef PROVE_ZERO_ALLOC

    lob::allocations_forbidden.store(true, std::memory_order_relaxed);
#endif

    for (size_t i = warmup_orders; i < num_orders; ++i) {

        auto start = std::chrono::high_resolution_clock::now();

        book.addOrder(orders[i]);

        auto end = std::chrono::high_resolution_clock::now();

        double duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        latencies.push_back(duration_ns);
    }

#ifdef PROVE_ZERO_ALLOC

    lob::allocations_forbidden.store(false, std::memory_order_relaxed);
#endif

    std::cout << "Direct Call Latency measurement completed. Sorting data..." << std::endl;

    std::sort(latencies.begin(), latencies.end());

    size_t n = latencies.size();

    double p50 = latencies[static_cast<size_t>(n * 0.50)];
    double p99 = latencies[static_cast<size_t>(n * 0.99)];
    double p999 = latencies[static_cast<size_t>(n * 0.999)];

    std::cout << "Direct Call Latency Percentiles:\n";
    std::cout << "  50th Percentile (Median): " << p50 << " ns\n";
    std::cout << "  99th Percentile:          " << p99 << " ns\n";
    std::cout << "  99.9th Percentile:        " << p999 << " ns\n";

    const double bin_step = 10.0;
    const size_t num_bins = 500;

    std::vector<size_t> bins(num_bins, 0);

    size_t outliers = 0;

    for (double latency : latencies) {

        size_t bin_index = static_cast<size_t>(latency / bin_step);

        if (bin_index < num_bins) {

            bins[bin_index]++;
        } else {

            outliers++;
        }
    }

    const std::string csv_path = "/mnt/d/Low Latency Matching Engine/matching-engine/docs/latency_report.csv";

    std::ofstream csv_file(csv_path);

    if (csv_file.is_open()) {

        csv_file << "Percentile,Latency_ns\n";
        csv_file << "p50," << p50 << "\n";
        csv_file << "p99," << p99 << "\n";
        csv_file << "p999," << p999 << "\n\n";

        csv_file << "Bucket_Lower_ns,Bucket_Upper_ns,Count\n";

        for (size_t i = 0; i < num_bins; ++i) {
            csv_file << (i * bin_step) << "," << ((i + 1) * bin_step) << "," << bins[i] << "\n";
        }

        csv_file << "5000,Infinity," << outliers << "\n";

        csv_file.close();

        std::cout << "Saved latency and histogram report to: " << csv_path << std::endl;
    }

    std::cout << "\n--- DIRECT CALL THROUGHPUT BENCHMARK (5 Runs) ---\n";

    std::vector<double> direct_throughputs;
    direct_throughputs.reserve(5);

    for (int run = 1; run <= 5; ++run) {

        lob::OrderBook run_book;

        auto start_run = std::chrono::high_resolution_clock::now();

        for (const auto& o : orders) {
            run_book.addOrder(o);
        }

        auto end_run = std::chrono::high_resolution_clock::now();

        double run_duration_s = std::chrono::duration_cast<std::chrono::microseconds>(end_run - start_run).count() / 1000000.0;

        double tp = static_cast<double>(num_orders) / run_duration_s;
        direct_throughputs.push_back(tp);

        std::cout << "  Run " << run << " Throughput: " << tp << " orders/second (elapsed: " << run_duration_s << " s)\n";
    }

    double direct_sum = std::accumulate(direct_throughputs.begin(), direct_throughputs.end(), 0.0);
    double direct_mean = direct_sum / direct_throughputs.size();

    std::cout << "\n--- PIPELINED INGESTION THROUGHPUT BENCHMARK (5 Runs) ---\n";

    std::vector<double> pipelined_throughputs;
    pipelined_throughputs.reserve(5);

    for (int run = 1; run <= 5; ++run) {

        lob::SPSCRingBuffer<lob::Order, 1024> queue;

        lob::OrderBook pipeline_book;

        std::atomic<bool> start_signal{false};

        std::atomic<size_t> processed_count{0};

        std::thread consumer_thread([&]() {

            pin_thread(2);

            while (!start_signal.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }

            size_t count = 0;

            lob::Order order;

            while (count < num_orders) {

                if (queue.pop(order)) {

                    pipeline_book.addOrder(order);

                    count++;
                } else {

                    #if defined(__x86_64__) || defined(_M_X64)
                    asm volatile("pause" ::: "memory");
                    #else
                    std::this_thread::yield();
                    #endif
                }
            }

            processed_count.store(count, std::memory_order_release);
        });

        std::thread producer_thread([&]() {

            pin_thread(1);

            while (!start_signal.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }

            for (const auto& order : orders) {

                while (!queue.push(order)) {
                    #if defined(__x86_64__) || defined(_M_X64)
                    asm volatile("pause" ::: "memory");
                    #else
                    std::this_thread::yield();
                    #endif
                }
            }
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        auto start_run = std::chrono::high_resolution_clock::now();

        start_signal.store(true, std::memory_order_release);

#ifdef PROVE_ZERO_ALLOC

        lob::allocations_forbidden.store(true, std::memory_order_relaxed);
#endif

        while (processed_count.load(std::memory_order_acquire) < num_orders) {
            #if defined(__x86_64__) || defined(_M_X64)
            asm volatile("pause" ::: "memory");
            #else
            std::this_thread::yield();
            #endif
        }

#ifdef PROVE_ZERO_ALLOC

        lob::allocations_forbidden.store(false, std::memory_order_relaxed);
#endif

        auto end_run = std::chrono::high_resolution_clock::now();

        producer_thread.join();
        consumer_thread.join();

        double run_duration_s = std::chrono::duration_cast<std::chrono::microseconds>(end_run - start_run).count() / 1000000.0;

        double tp = static_cast<double>(num_orders) / run_duration_s;
        pipelined_throughputs.push_back(tp);

        std::cout << "  Run " << run << " Pipelined Throughput: " << tp << " orders/second (elapsed: " << run_duration_s << " s)\n";
    }

    double pipelined_sum = std::accumulate(pipelined_throughputs.begin(), pipelined_throughputs.end(), 0.0);
    double pipelined_mean = pipelined_sum / pipelined_throughputs.size();

    std::cout << "\nBenchmark Summary Comparison:\n";
    std::cout << "  Direct Call Baseline Average Throughput: " << direct_mean << " orders/sec\n";
    std::cout << "  Pipelined Ingestion (SPSC) Average Throughput: " << pipelined_mean << " orders/sec\n";

    return 0;
}
