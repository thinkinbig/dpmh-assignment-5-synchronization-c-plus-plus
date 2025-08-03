#include "synchronization.hpp"
#include "perfEvent.hpp"

#include <algorithm>
#include <atomic>

// How to install:
// https://askubuntu.com/questions/1170054/install-newest-tbb-thread-building-blocks-on-ubuntu-18-04
#include <tbb/tbb.h>

using namespace std;

template <class L>
void bench(const uint32_t domain, const std::vector<uint32_t> init,
           const std::vector<uint32_t> work, const std::vector<int> &threads,
           bool &printHeader) {
  // tbb::task_scheduler_init taskScheduler;
  L s;
  for (auto insert : init)
    s.insert(insert);

  /*--------------------------- Read-only workload ---------------------------*/
  for (int t : threads) {
    tbb::task_arena arena(t);  // Create a task arena with the desired number of threads
    // taskScheduler.terminate();
    // taskScheduler.initialize(t);
    atomic<uint32_t> total(0);
    BenchmarkParameters params;
    params.setParam("workload", "contains");
    params.setParam("approach", L::name);
    params.setParam("threads", t);
    params.setParam("domain", domain);
    PerfEventBlock b(1, params, printHeader);
    printHeader = false;
    auto thread_contains = [&](const tbb::blocked_range<uint32_t> &range) {
      // Count the number of keys contained in the list-based set
      uint32_t sum = 0;
      for (uint32_t i = range.begin(); i < range.end(); i++)
        sum += s.contains(work[i]);
      // Add the thread's matches to the global counter
      total += sum;
    };

    arena.execute([&]() {
      tbb::parallel_for(tbb::blocked_range<uint32_t>(0, work.size()), thread_contains);
    });
  }
  
  /*----------------------------- Mixed workload -----------------------------*/
  if (L::synchronized) {
    for (int t : threads) {
      tbb::task_arena arena(t);  // Create a task arena with the desired number of threads
      // taskScheduler.terminate();
      // taskScheduler.initialize(t);
      atomic<uint32_t> total(0);
      BenchmarkParameters params;
      params.setParam("workload", "mix");
      params.setParam("approach", L::name);
      params.setParam("threads", t);
      params.setParam("domain", domain);
      PerfEventBlock b(1, params, printHeader);
      printHeader = false;
      auto thread_contains = [&](const tbb::blocked_range<uint32_t> &range) {
        // Count the number of keys contained in the list-based set
        uint32_t sum = 0;
        for (uint32_t i = range.begin(); i < range.end(); i++) {
          if (i % 10 < 6) { // Decide the executed action for this key
            sum += s.contains(work[i]);
          } else if (i % 10 < 8) {
            s.insert(work[i]);
          } else {
            s.remove(work[i]);
          }
        }

        total += sum;
      };
      arena.execute([&]() {
        tbb::parallel_for(tbb::blocked_range<uint32_t>(0, work.size()), thread_contains);
      });
    }
  } 
  else {

    // Thread-safe global counter for the number of keys contained in the set
    atomic<uint32_t> total(0);

    // Block for profiling
    BenchmarkParameters params;
    params.setParam("workload", "mix");
    params.setParam("approach", "nosync");
    params.setParam("threads", "1");
    params.setParam("domain", domain);
    PerfEventBlock b(1, params, printHeader);

    uint32_t sum = 0;

    for (uint32_t i = 0; i < work.size(); i++) {
      if (i % 10 < 6) { // Decide the executed action for this key
        sum += s.contains(work[i]);
      } else if (i % 10 < 8) {
        s.insert(work[i]);
      } else {
        s.remove(work[i]);
      }
    }

    total += sum;
  }
}

int main(int argc, char **argv) {
  using mutex = tbb::spin_mutex;
  using mutex_rw = tbb::spin_rw_mutex;
  bool printHeader = true;
  // Number of (non-unique) values, which are inserted into the list. Feel free to change it
  uint32_t operations = 10e5;
  // Vector containing the number of threads for the experiments
  // ToDo: adapt for used machine, test more software threads than available hardware threads as well
  std::vector<int> threads = {1,  2,  4,  6,  8,  10};
  
  // Define the domain range of the values. ToDo: Play around with this parameter.
  for (auto domain : {16, 64, 256, 1024}) {
    // Vector containing the dataset
    vector<uint32_t> work(operations);

    for (uint32_t i = 0; i < operations; i++)
      work[i] = random() % domain; // Create elements & map them to the domain

    // Initialize the list-based set
    std::vector<uint32_t> init;

    // Init list-based set. Insert only a small subset into the list-based set.
    for (uint32_t i = 0; i < domain * 0.75; i++)
      init.push_back(random() % domain);

    // Serial execution
    bench<ListBasedSetNoSync<int64_t>>(domain, init, work, threads,
                                       printHeader);
    printHeader = false;

    // Multi-threaded execution
    bench<ListBasedSetCoarseLock<int64_t, mutex>>(domain, init, work, threads, printHeader);
    bench<ListBasedSetCoarseLockRW<int64_t, mutex_rw>>(domain, init, work, threads, printHeader);
    bench<ListBasedSetLockCoupling<int64_t, mutex>>(domain, init, work, threads, printHeader);
    bench<ListBasedSetLockCouplingRW<int64_t, mutex_rw>>(domain, init, work, threads, printHeader);
    bench<ListBasedSetOptimistic<int64_t, mutex>>(domain, init, work, threads, printHeader);
    bench<ListBasedSetOptimisticLockCoupling<int64_t, mutex>>(domain, init, work, threads, printHeader);
  }

  return 0;
}