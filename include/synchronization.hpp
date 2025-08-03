#ifndef HW5_SYNCHRONIZATION_HPP
#define HW5_SYNCHRONIZATION_HPP

#include <atomic>
#include <iostream>
#include <limits>
#include <mutex>
#include <tbb/spin_mutex.h>
#include <tbb/spin_rw_mutex.h>


// How to install:
// https://askubuntu.com/questions/1170054/install-newest-tbb-thread-building-blocks-on-ubuntu-18-04
#include <tbb/tbb.h>



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

  bool contains(T k) const {
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

  virtual bool contains(T k) const = 0;
  virtual void insert(T k) = 0;
  virtual void remove(T k) = 0;
};

// class M defines the used mutex, you can use the locks provided in tbb
template <class T, class M>
struct ListBasedSetCoarseLock : virtual public ListBasedSetSync<T> {
  // Coarse-Grained Locking: Single mutex protects entire data structure
  static constexpr char name[] = "coarse";

  struct Entry {
    T key;
    Entry *next;
  };

  Entry staticHead;
  Entry staticTail;
  mutable M mutex;

  ListBasedSetCoarseLock() {
    staticHead.key = std::numeric_limits<T>::min();
    staticHead.next = &staticTail;
    staticTail.key = std::numeric_limits<T>::max();
    staticTail.next = nullptr;
  }

  bool contains(T k) const {
    typename M::scoped_lock lock(mutex);
    Entry *curr = staticHead.next;
    while (curr->key < k)
      curr = curr->next;
    return (curr->key == k);
  }

  void insert(T k) {
    typename M::scoped_lock lock(mutex);
    Entry *pred = &staticHead;
    Entry *curr = staticHead.next;
    while (curr->key < k) {
      pred = curr;
      curr = curr->next;
    }
    if (curr->key == k)
      return;
    Entry *n = new Entry{k, curr};
    pred->next = n;
  }

  void remove(T k) {
    typename M::scoped_lock lock(mutex);
    Entry *pred = &staticHead;
    Entry *curr = staticHead.next;
    while (curr->key < k) {
      pred = curr;
      curr = curr->next;
    }
    if (curr->key == k) {
      pred->next = curr->next;
    }
  }
};

// class M defines the used mutex
template <class T, class M>
struct ListBasedSetCoarseLockRW : virtual public ListBasedSetSync<T> {
  // Coarse-Grained Locking with read/write lock
  static constexpr char name[] = "coarseRW";

  struct Entry {
    T key;
    Entry *next;
  };

  Entry staticHead;
  Entry staticTail;
  mutable M rwMutex;

  ListBasedSetCoarseLockRW() {
    staticHead.key = std::numeric_limits<T>::min();
    staticHead.next = &staticTail;
    staticTail.key = std::numeric_limits<T>::max();
    staticTail.next = nullptr;
  }

  bool contains(T k) const {
    typename M::scoped_lock lock(rwMutex, false); // read lock
    Entry *curr = staticHead.next;
    while (curr->key < k)
      curr = curr->next;
    return (curr->key == k);
  }

  void insert(T k) {
    typename M::scoped_lock lock(rwMutex, true); // write lock
    Entry *pred = &staticHead;
    Entry *curr = staticHead.next;
    while (curr->key < k) {
      pred = curr;
      curr = curr->next;
    }
    if (curr->key == k)
      return;
    Entry *n = new Entry{k, curr};
    pred->next = n;
  }

  void remove(T k) {
    typename M::scoped_lock lock(rwMutex, true); // write lock
    Entry *pred = &staticHead;
    Entry *curr = staticHead.next;
    while (curr->key < k) {
      pred = curr;
      curr = curr->next;
    }
    if (curr->key == k) {
      pred->next = curr->next;
    }
  }
};

// class M defines the used mutex
template <class T, class M>
struct ListBasedSetLockCoupling : virtual public ListBasedSetSync<T> {
  static constexpr char name[] = "lockCoupling";

  struct Entry {
    T key;
    Entry *next;
    mutable M mutex;
  };

  Entry staticHead;
  Entry staticTail;

  ListBasedSetLockCoupling() {
    staticHead.key = std::numeric_limits<T>::min();
    staticHead.next = &staticTail;
    staticTail.key = std::numeric_limits<T>::max();
    staticTail.next = nullptr;
  }

  bool contains(T k) const {
    const Entry *pred = &staticHead;
    pred->mutex.lock();
    const Entry *curr = pred->next;
    
    while (curr->key < k) {
      curr->mutex.lock();
      pred->mutex.unlock();
      pred = curr;
      curr = curr->next;
    }
    
    curr->mutex.lock();
    pred->mutex.unlock();
    bool result = (curr->key == k);
    curr->mutex.unlock();
    return result;
  }

  void insert(T k) {
    Entry *pred = &staticHead;
    pred->mutex.lock();
    Entry *curr = pred->next;
    
    while (curr->key < k) {
      curr->mutex.lock();
      pred->mutex.unlock();
      pred = curr;
      curr = curr->next;
    }
    
    curr->mutex.lock();
    if (curr->key == k) {
      pred->mutex.unlock();
      curr->mutex.unlock();
      return;
    }
      
    Entry *n = new Entry{k, curr, M()};
    pred->next = n;
    pred->mutex.unlock();
    curr->mutex.unlock();
  }

  void remove(T k) {
    Entry *pred = &staticHead;
    pred->mutex.lock();
    Entry *curr = pred->next;
    
    while (curr->key < k) {
      curr->mutex.lock();
      pred->mutex.unlock();
      pred = curr;
      curr = curr->next;
    }
    
    curr->mutex.lock();
    if (curr->key == k) {
      pred->next = curr->next;
    }
    pred->mutex.unlock();
    curr->mutex.unlock();
  }
};

// class M defines the used mutex
template <class T, class M>
struct ListBasedSetLockCouplingRW : virtual public ListBasedSetSync<T> {
  // Lock Coupling with read/write locks
  static constexpr char name[] = "lockCouplingRW";

  struct Entry {
    T key;
    Entry *next;
    mutable M rwMutex;
  };

  Entry staticHead;
  Entry staticTail;

  ListBasedSetLockCouplingRW() {
    staticHead.key = std::numeric_limits<T>::min();
    staticHead.next = &staticTail;
    staticTail.key = std::numeric_limits<T>::max();
    staticTail.next = nullptr;
  }

  bool contains(T k) const {
    const Entry *pred = &staticHead;
    pred->rwMutex.lock_shared();
    const Entry *curr = pred->next;
    
    while (curr->key < k) {
      curr->rwMutex.lock_shared();
      pred->rwMutex.unlock_shared();
      pred = curr;
      curr = curr->next;
    }
    
    curr->rwMutex.lock_shared();
    pred->rwMutex.unlock_shared();
    bool result = (curr->key == k);
    curr->rwMutex.unlock_shared();
    return result;
  }

  void insert(T k) {
    Entry *pred = &staticHead;
    pred->rwMutex.lock();
    Entry *curr = pred->next;
    
    while (curr->key < k) {
      curr->rwMutex.lock();
      pred->rwMutex.unlock();
      pred = curr;
      curr = curr->next;
    }
    
    curr->rwMutex.lock();
    if (curr->key == k) {
      pred->rwMutex.unlock();
      curr->rwMutex.unlock();
      return;
    }
      
    Entry *n = new Entry{k, curr, M()};
    pred->next = n;
    pred->rwMutex.unlock();
    curr->rwMutex.unlock();
  }

  void remove(T k) {
    Entry *pred = &staticHead;
    pred->rwMutex.lock();
    Entry *curr = pred->next;
    
    while (curr->key < k) {
      curr->rwMutex.lock();
      pred->rwMutex.unlock();
      pred = curr;
      curr = curr->next;
    }
    
    curr->rwMutex.lock();
    if (curr->key == k) {
      pred->next = curr->next;
    }
    pred->rwMutex.unlock();
    curr->rwMutex.unlock();
  }
};

// class M defines the used mutex
template <class T, class M>
struct ListBasedSetOptimistic : virtual public ListBasedSetSync<T> {
  // Optimistic Locking: Search without locks, then validate
  static constexpr char name[] = "optimistic";

  struct Entry {
    T key;
    std::atomic<Entry*> next;  // Atomic pointer for race-free operations
    Entry() : key(), next(nullptr) {}
    Entry(T k, Entry* n) : key(k), next(n) {}
  };

  Entry staticHead;
  Entry staticTail;
  mutable M mutex;

  ListBasedSetOptimistic() {
    staticHead.key = std::numeric_limits<T>::min();
    staticHead.next.store(&staticTail, std::memory_order_relaxed);
    staticTail.key = std::numeric_limits<T>::max();
    staticTail.next.store(nullptr, std::memory_order_relaxed);
  }

  // Validate the entire path from head to the target position
  bool validatePath(const Entry* pred, const Entry* curr, T k) const {
    const Entry* checkPred = &staticHead;
    const Entry* checkCurr = checkPred->next.load(std::memory_order_acquire);
    
    // Traverse the list to find the position
    while (checkCurr->key < k) {
      checkPred = checkCurr;
      checkCurr = checkCurr->next.load(std::memory_order_acquire);
    }
    
    // Check if we reached the same position
    return (checkPred == pred && checkCurr == curr);
  }

  bool contains(T k) const {
    while (true) {
      // Optimistic search without locks
      const Entry *pred = &staticHead;
      const Entry *curr = pred->next.load(std::memory_order_acquire);
      
      while (curr->key < k) {
        pred = curr;
        curr = curr->next.load(std::memory_order_acquire);
      }
      
      // Validate the entire path
      if (validatePath(pred, curr, k)) {
        return (curr->key == k);
      }
      
      // Validation failed - retry
      // No need for pessimistic fallback for contains
    }
  }

  void insert(T k) {
    while (true) {
      // Optimistic search without locks
      Entry *pred = &staticHead;
      Entry *curr = pred->next.load(std::memory_order_acquire);
      
      while (curr->key < k) {
        pred = curr;
        curr = curr->next.load(std::memory_order_acquire);
      }
      
      // Check if key already exists
      if (curr->key == k) {
        return;
      }
      
      // Validate the entire path
      if (validatePath(pred, curr, k)) {
        // Create new node
        Entry *n = new Entry{k, curr};
        
        // Try optimistic modification with atomic compare-and-swap
        Entry* expected = curr;
        if (pred->next.compare_exchange_weak(expected, n, 
                                           std::memory_order_release,
                                           std::memory_order_relaxed)) {
          return; // Success
        }
        // CAS failed, retry
        delete n;
        continue;
      }
      
      // Validation failed - retry
    }
  }

  void remove(T k) {
    // Use pessimistic locking for remove to ensure correctness under high contention
    typename M::scoped_lock lock(mutex);
    
    Entry *pred = &staticHead;
    Entry *curr = pred->next.load(std::memory_order_acquire);
    
    while (curr->key < k) {
      pred = curr;
      curr = curr->next.load(std::memory_order_acquire);
    }
    
    if (curr->key == k) {
      Entry* next = curr->next.load(std::memory_order_acquire);
      pred->next.store(next, std::memory_order_release);
      // NOTE: This is a memory leak. In a real-world concurrent data
      // structure, we would need a safe memory reclamation scheme.
    }
  }
};

// class M defines the used mutex
template <class T, class M>
struct ListBasedSetOptimisticLockCoupling : virtual public ListBasedSetSync<T> {
  // Optimistic Lock Coupling: Combine optimistic and lock coupling
  static constexpr char name[] = "optimisticLockCoupling";

  struct Entry {
    T key;
    std::atomic<Entry*> next;  // Atomic pointer for optimistic operations
    std::atomic<int> version{0};
    mutable M mutex;

    Entry() : key(), next(nullptr) {}
    Entry(T k, Entry* n) : key(k), next(n) {}
  };

  Entry staticHead;
  Entry staticTail;

  ListBasedSetOptimisticLockCoupling() {
    staticHead.key = std::numeric_limits<T>::min();
    staticHead.next.store(&staticTail, std::memory_order_relaxed);
    staticTail.key = std::numeric_limits<T>::max();
    staticTail.next.store(nullptr, std::memory_order_relaxed);
  }

  bool contains(T k) const {
    while (true) {
      const Entry *pred = &staticHead;
      int pred_version = pred->version.load(std::memory_order_acquire);
      const Entry *curr = pred->next.load(std::memory_order_acquire);
      
      while (curr->key < k) {
        pred = curr;
        pred_version = pred->version.load(std::memory_order_acquire);
        curr = curr->next.load(std::memory_order_acquire);
      }
      
      if (pred->version.load(std::memory_order_acquire) == pred_version) {
        return (curr->key == k);
      }
    }
  }

  void insert(T k) {
    while (true) {
      Entry *pred = &staticHead;
      int pred_version = pred->version.load(std::memory_order_acquire);
      Entry *curr = pred->next.load(std::memory_order_acquire);
      int curr_version = curr->version.load(std::memory_order_acquire);

      while (curr->key < k) {
        pred = curr;
        pred_version = pred->version.load(std::memory_order_acquire);
        curr = curr->next.load(std::memory_order_acquire);
        curr_version = curr->version.load(std::memory_order_acquire);
      }

      pred->mutex.lock();
      curr->mutex.lock();
      if (validate(pred, pred_version, curr, curr_version)) {
        if (curr->key == k) {
          curr->mutex.unlock();
          pred->mutex.unlock();
          return;
        }
        Entry *n = new Entry(k, curr);
        pred->next.store(n, std::memory_order_release);
        pred->version.fetch_add(1, std::memory_order_release);
        curr->mutex.unlock();
        pred->mutex.unlock();
        return;
      }
      curr->mutex.unlock();
      pred->mutex.unlock();
    }
  }

  bool validate(Entry* pred, int pred_version, Entry* curr, int curr_version) {
    return pred->version.load(std::memory_order_acquire) == pred_version &&
           curr->version.load(std::memory_order_acquire) == curr_version &&
           pred->next.load(std::memory_order_acquire) == curr;
  }

  void remove(T k) {
    Entry* pred = &staticHead;
    pred->mutex.lock();
    Entry* curr = pred->next.load(std::memory_order_acquire);

    while (curr->key < k) {
      curr->mutex.lock();
      pred->mutex.unlock();
      pred = curr;
      curr = pred->next.load(std::memory_order_acquire);
    }

    curr->mutex.lock();
    if (curr->key == k) {
      pred->next.store(curr->next.load(std::memory_order_relaxed),
                       std::memory_order_release);
      pred->version.fetch_add(1, std::memory_order_release);
      // NOTE: This is a memory leak. In a real-world concurrent data
      // structure, we would need a safe memory reclamation scheme like
      // epoch-based reclamation or hazard pointers to free `curr`.
      // For this assignment, we accept the leak, consistent with other
      // implementations in this file.
    }
    curr->mutex.unlock();
    pred->mutex.unlock();
  }
};

#endif // HW5_SYNCHRONIZATION_HPP
