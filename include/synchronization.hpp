#ifndef HW5_SYNCHRONIZATION_HPP
#define HW5_SYNCHRONIZATION_HPP

#include "perfEvent.hpp"
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstring>
#include <immintrin.h>
#include <iostream>
#include <sys/mman.h>

// How to install:
// https://askubuntu.com/questions/1170054/install-newest-tbb-thread-building-blocks-on-ubuntu-18-04
#include <tbb/tbb.h>

using namespace std;

template <class T> struct ListBasedSetNoSync {
  static constexpr bool synchronized = false;
  static constexpr char name[] = "nosync";

  struct Entry {
    T key;
    Entry *next;
  };

  Entry staticHead;
  Entry staticTail;

  ListBasedSetNoSync() {
    staticHead.key = std::numeric_limits<T>::min();
    staticHead.next = &staticTail;
    staticTail.key = std::numeric_limits<T>::max();
    staticTail.next = nullptr;
  }

  bool contains(T k) {
    Entry *curr = staticHead.next;
    // Iterate over all elements whose key is smaller, we stop at the first
    // element with key >= k
    while (curr->key < k)
      curr = curr->next;
    // Check if this element is the searched key and return the result
    return (curr->key == k);
  }

  void insert(T k) {
    // Start inserting a new element from the head
    Entry *pred = &staticHead;
    Entry *curr = staticHead.next;
    // Iterate over all elements whose key is smaller, we stop at the first
    // element with key >= k
    while (curr->key < k) {
      pred = curr;
      curr = curr->next;
    }

    // As we have a set, there is nothing to do if the key is already contained
    if (curr->key == k)
      return;

    // Create a new list element for the new key
    Entry *n = new Entry{k, curr};
    // Insert the element in the list
    pred->next = n;
  }

  void remove(T k) {
    Entry *pred = &staticHead;
    Entry *curr = staticHead.next;
    // Iterate over all elements whose key is smaller, we stop at the first
    // element with key >= k
    while (curr->key < k) {
      pred = curr;
      curr = curr->next;
    }

    // If the element has the searched key, remove it
    if (curr->key == k) {
      pred->next = curr->next;
      // ignore reclamation for now
    }
  }
};

// Defines all required functions for the synchronized implementations
template <class T> struct ListBasedSetSync {
  static constexpr bool synchronized = true;

  virtual bool contains(T k) = 0;
  virtual void insert(T k) = 0;
  virtual void remove(T k) = 0;
};

// class M defines the used mutex, you can use the locks provided in tbb
template <class T, class M>
struct ListBasedSetCoarseLock : virtual public ListBasedSetSync<T> {
  // ToDo: implement ListBasedSetNoSync using Coarse-Grained Locking
  // To implement it you can copy ListBasedSetNoSync and extend it for the needed locking.
  static constexpr char name[] = "coarse";

  ListBasedSetCoarseLock() { }

  bool contains(T k) { return false; }
  void insert(T k) { }
  void remove(T k) { }
};

// class M defines the used mutex
template <class T, class M>
struct ListBasedSetCoarseLockRW : virtual public ListBasedSetSync<T> {
  // ToDo: implement ListBasedSetNoSync using Coarse-Grained Locking with read/write lock
  static constexpr char name[] = "coarseRW";

  ListBasedSetCoarseLockRW() { }

  bool contains(T k) { return false; }
  void insert(T k) { }
  void remove(T k) { }
};

// class M defines the used mutex
template <class T, class M>
struct ListBasedSetLockCoupling : virtual public ListBasedSetSync<T> {
  // ToDo: implement ListBasedSetNoSync using Lock Coupling
  static constexpr char name[] = "lockCoupling";

  ListBasedSetLockCoupling() { }

  bool contains(T k) { return false; }
  void insert(T k) { }
  void remove(T k) { }
};

// class M defines the used mutex
template <class T, class M>
struct ListBasedSetLockCouplingRW : virtual public ListBasedSetSync<T> {
  // ToDo: implement ListBasedSetNoSync using Lock Coupling with read/write locks
  static constexpr char name[] = "lockCouplingRW";

  ListBasedSetLockCouplingRW() { }

  bool contains(T k) { return false; }
  void insert(T k) { }
  void remove(T k) { }
};

// class M defines the used mutex
template <class T, class M>
struct ListBasedSetOptimistic : virtual public ListBasedSetSync<T> {
  // ToDo: implement ListBasedSetNoSync using Optimistic Locking
  static constexpr char name[] = "optimistic";

  ListBasedSetOptimistic() { }

  bool contains(T k) { return false; }
  void insert(T k) { }
  void remove(T k) { }
};

// class M defines the used mutex
template <class T, class M>
struct ListBasedSetOptimisticLockCoupling : virtual public ListBasedSetSync<T> {
  // ToDo: implement ListBasedSetNoSync using Optimistic Lock Coupling
  static constexpr char name[] = "optimisticLockCoupling";

  ListBasedSetOptimisticLockCoupling() { }

  bool contains(T k) { return false; }
  void insert(T k) { }
  void remove(T k) { }
};

#endif // HW5_SYNCHRONIZATION_HPP
