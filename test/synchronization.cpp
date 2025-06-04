#include <gtest/gtest.h>
#include <bits/stdc++.h>

#include "synchronization.hpp"

template <class L>
void test() {
  /*----------------------------- Single-threaded -----------------------------*/
  std::vector<int64_t> v(1024);
  std::iota(std::begin(v), std::end(v), 0);
  std::random_shuffle(std::begin(v), std::end(v));

  L s;

  for (auto value : v) {
    s.insert(value);
    ASSERT_TRUE(s.contains(value));
  }

  for (auto value : v) {
    s.remove(value);
    ASSERT_FALSE(s.contains(value));
  }

  /*-------------------------- Multi-threaded insert --------------------------*/
  auto thread_check_insert = [&](const tbb::blocked_range<int64_t> &range) {
    for (int64_t i = range.begin(); i < range.end(); i++) {
      s.insert(v[i]);
    }
  };
  tbb::parallel_for(tbb::blocked_range<int64_t>(0, v.size()), thread_check_insert);

  for (auto value : v) {
    ASSERT_TRUE(s.contains(value));
  }

  /*-------------------------- Multi-threaded delete --------------------------*/
  auto thread_check_remove = [&](const tbb::blocked_range<int64_t> &range) {
    for (int64_t i = range.begin(); i < range.end(); i++) {
      s.remove(v[i]);
    }

  };
  tbb::parallel_for(tbb::blocked_range<int64_t>(0, v.size()), thread_check_remove);
  for (auto value : v) {
    ASSERT_FALSE(s.contains(value));
  }

  /*-------------------------- Multi-threaded mixed ---------------------------*/
  auto thread_check_insert_and_remove = [&](const tbb::blocked_range<int64_t> &range) {
    for (int64_t i = range.begin(); i < range.end(); i++) {
      s.insert(v[i]);
    }
    for (int64_t i = range.begin(); i < range.end(); i++) {
      s.remove(v[i]);
    }

  };
  tbb::parallel_for(tbb::blocked_range<int64_t>(0, v.size()), thread_check_insert_and_remove);
  for (auto value : v) {
    ASSERT_FALSE(s.contains(value));
  }
}

TEST(synchronization, ListBasedSetCoarseLock) {
  test<ListBasedSetCoarseLock<int64_t, tbb::spin_mutex>>();
}

TEST(synchronization, ListBasedSetCoarseLockRW) {
  test<ListBasedSetCoarseLockRW<int64_t, tbb::spin_rw_mutex>>();
}

TEST(synchronization, ListBasedSetLockCoupling) {
  test<ListBasedSetLockCoupling<int64_t, tbb::spin_mutex>>();
}

TEST(synchronization, ListBasedSetLockCouplingRW) {
  test<ListBasedSetLockCouplingRW<int64_t, tbb::spin_rw_mutex>>();
}

TEST(synchronization, ListBasedSetOptimistic) {
  test<ListBasedSetOptimistic<int64_t, tbb::spin_mutex>>();
}

TEST(synchronization, ListBasedSetOptimisticLockCoupling) {
  test<ListBasedSetOptimisticLockCoupling<int64_t, tbb::spin_mutex>>();
}