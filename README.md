# Low-Latency Limit Order Book (LOB) Matching Engine

**Status: Complete (All Phases 0-6 Passed & Documented)**

A high-performance C++20 Limit Order Book (LOB) matching engine designed for high-frequency trading (HFT) platforms. The core engine achieves a peak throughput of **16.9 Million orders/second** with a median latency of **65 nanoseconds** (and tail latency of **1.48 microseconds** at p99.9) on commodity hardware under realistic market distributions. The system features a lock-free Single-Producer Single-Consumer (SPSC) queue that decouples ingestion from matching to enable parallelized packet decoding. The entire matching path is completely **zero-allocation** at runtime.

---

## System Architecture

The pipeline separates order reception (e.g., from network ports) from matching logic using a lock-free ring buffer:

```
[Producer Thread]  ======>  [SPSC Ring Buffer]  ======>  [Consumer Thread]  ======>  [Trade Execution]
(Ingests/Decodes            (Intrusive, Lock-free        (Pops Orders,               (Stack-allocated
 Network Orders)             FIFO Queue with              Modifies Engine             TradeVector
                             Index Caching)               Ladders in O(1))            Output, No Heap)
```

---

## Design Decisions & Tradeoffs

| Component | Architecture Design | One-line "Why" | Limits & Tradeoffs |
| :--- | :--- | :--- | :--- |
| **Pricing** | Fixed-point Integer Ticks | Prevents rounding errors introduced by IEEE-754 floating-point double representations. | Requires deciding a tick multiplier (e.g. 1 tick = $0.0001) upfront. |
| **Price Levels** | Array-Indexed Ladders | Replaces O(log N) sorted trees (`std::map`) with O(1) array access for maximum cache hits. | Assumes a bounded price range (e.g., 8000–12000 ticks) corresponding to exchange price bands. |
| **Memory** | Pre-allocated Object Pool | Eliminates runtime OS malloc/new calls to prevent non-deterministic heap allocation latency. | Sized at startup (`POOL_CAPACITY = 1.1M`). Overflows must be rejected or warm-resized. |
| **Queues** | Intrusive Doubly-Linked Lists | Reuses Order memory blocks as linked list nodes, avoiding `std::list` heap allocations. | Iterators are raw pointers, requiring strict owner checks to prevent dangling pointer errors. |
| **Ingestion** | Lock-Free SPSC Ring Buffer | Decouples network packet parsing from matching using memory barriers (`acquire`/`release`). | Limited to Single-Producer Single-Consumer. Multiple producers require sharding or MPSC. |
| **Output** | Stack-Allocated `TradeVector` | Avoids heap allocations from returning variable-size trade vectors by value. | Capacity is capped at 64 trades per incoming order (excess trades are clamped). |

---

## Benchmark Methodology & Results

1. **Synthetic Market Load**: Replays 1,000,000 orders using a random walk fair-price model with orders clustering using normal distributions and quantities following log-normal distributions.
2. **Warmup**: Discards the first 10,000 orders to pre-heat CPU caches and branch predictors.
3. **Hardware Config**: Intel(R) Core(TM) i5-13450HX pinned via `pthread_setaffinity_np` running WSL2 Ubuntu.

### Performance Summary

| Configuration | Average Throughput | Median (p50) Latency | Tail (p99.9) Latency |
| :--- | :--- | :--- | :--- |
| **Direct Call (Single Thread)** | **16.93M orders/sec** | **65 ns** | **1,483 ns** |
| **Pipelined Ingestion (SPSC)** | **7.57M orders/sec** | **N/A** | **N/A** |

*For more details on methodology, hardware configurations, and reproduction steps, refer to [docs/benchmarks.md](matching-engine/docs/benchmarks.md) and [docs/latency_report.csv](matching-engine/docs/latency_report.csv).*

---

## "What I'd Do for Production" (Future Enhancements)

To scale this matching engine to production standards, the following enhancements are typically required:

1. **Kernel Bypass Networking (DPDK / OpenOnload)**: Standard Linux kernel TCP/IP stacks introduce 2–5 microseconds of system call overhead. Re-writing the network receiver with DPDK or Solarflare OpenOnload bypasses the kernel, reading UDP multicast packets directly from the NIC ring buffer to userspace in under 100ns.
2. **NUMA Node Pinning**: Matching threads and ingestion rings must be pinned to the same NUMA (Non-Uniform Memory Access) node socket as the network card interface to prevent CPU socket interconnect (UPI/QPI) link latency.
3. **Symbol Sharding at Scale**: For exchanges matching hundreds of tickers, symbols are sharded across distinct SPSC ring-buffer paths. Each symbol runs its own matching thread, eliminating resource contention.
4. **Binary Protocol Parser (Simple Binary Encoding - SBE)**: Production inputs use binary transport protocols (like CME MDP 3.0 SBE) which are significantly faster to deserialize than text-based protocols.
5. **Persistence & Recovery (Write-Ahead Logging)**: Matching state must survive hardware failures. A low-latency ring buffer writes all incoming orders to a fast NVMe drive or SSD (Write-Ahead Log) before matching, allowing full recovery from state replay.

---

## One-Command Reproducibility

Build, test, and benchmark the repository in under 2 minutes:

```bash
# 1. Clone the repository and navigate to its root directory
# (Ensure you are on WSL2 or native Linux with C++20 support)

# 2. Run the test suite under Address and Undefined Behavior Sanitizers
./scripts/build_and_test.sh

# 3. Compile and execute the performance throughput/latency benchmarks
./scripts/run_benchmark.sh
```
