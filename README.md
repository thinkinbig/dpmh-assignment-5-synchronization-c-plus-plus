# DPMH Assignment 5: Synchronization in C++

**Data Processing on Modern Hardware** — TUM

A study of **concurrency and synchronization primitives** in C++ and their performance characteristics in the context of database query processing.

## Topics Covered

- **Mutexes**: `std::mutex`, spinlocks, and their overhead
- **Atomic operations**: `std::atomic`, CAS loops
- **Read-Write locks**: `std::shared_mutex`
- **Lock-free data structures**
- **Futex-based** synchronization
- **Memory ordering** (sequentially consistent, acquire-release, relaxed)
- **False sharing** detection and mitigation
- Concurrent hash table and concurrent queue implementations
- **Scalability analysis** under contention

## Structure

```
├── src/           # C++ concurrent data structures
├── include/       # Headers
├── lib/           # External libraries
├── plot.py        # Scalability visualization
├── perf.csv       # Performance measurements
├── report.pdf     # Report with analysis
└── CMakeLists.txt # Build system
```

## Build & Run

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j
./synchronization_bench
python3 ../plot.py
```

## Key Results

Analysis of how different synchronization primitives scale from 1 to 64 threads, showing the overhead of cache coherence protocols and the benefits of lock-free designs.
