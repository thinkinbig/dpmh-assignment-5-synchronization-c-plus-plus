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
  // Coarse-Grained Locking: Single mutex protects entire data structure
  static constexpr char name[] = "coarse";

  struct Entry {
    T key;
    Entry *next;
  };

  Entry staticHead;
  Entry staticTail;
  M mutex;

  ListBasedSetCoarseLock() {
    staticHead.key = std::numeric_limits<T>::min();
    staticHead.next = &staticTail;
    staticTail.key = std::numeric_limits<T>::max();
    staticTail.next = nullptr;
  }

  bool contains(T k) {
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
  M rwMutex;

  ListBasedSetCoarseLockRW() {
    staticHead.key = std::numeric_limits<T>::min();
    staticHead.next = &staticTail;
    staticTail.key = std::numeric_limits<T>::max();
    staticTail.next = nullptr;
  }

  bool contains(T k) {
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
    M mutex;
  };

  Entry staticHead;
  Entry staticTail;

  ListBasedSetLockCoupling() {
    staticHead.key = std::numeric_limits<T>::min();
    staticHead.next = &staticTail;
    staticTail.key = std::numeric_limits<T>::max();
    staticTail.next = nullptr;
  }

  bool contains(T k) {
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
    M rwMutex;
  };

  Entry staticHead;
  Entry staticTail;

  ListBasedSetLockCouplingRW() {
    staticHead.key = std::numeric_limits<T>::min();
    staticHead.next = &staticTail;
    staticTail.key = std::numeric_limits<T>::max();
    staticTail.next = nullptr;
  }

  bool contains(T k) {
    Entry *pred = &staticHead;
    pred->rwMutex.lock_shared();
    Entry *curr = pred->next;
    
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
    Entry *next;
    std::atomic<int> version{0};
    M mutex;
  };

  Entry staticHead;
  Entry staticTail;

  ListBasedSetOptimistic() {
    staticHead.key = std::numeric_limits<T>::min();
    staticHead.next = &staticTail;
    staticTail.key = std::numeric_limits<T>::max();
    staticTail.next = nullptr;
  }

  void acquireOrderedLocks(Entry *a, Entry *b, 
                          typename M::scoped_lock &lockA, 
                          typename M::scoped_lock &lockB) {
    if (a < b) {
      lockA.acquire(a->mutex);
      lockB.acquire(b->mutex);
    } else {
      lockB.acquire(b->mutex);
      lockA.acquire(a->mutex);
    }
  }

  bool validate(Entry *pred, Entry *curr, int pred_version, int curr_version) {
    return pred->version.load() == pred_version &&
           curr->version.load() == curr_version &&
           pred->next == curr;
  }

  bool contains(T k) {
    while (true) {
      Entry *pred = &staticHead;
      Entry *curr = pred->next;
      while (curr->key < k) {
        pred = curr;
        curr = curr->next;
      o

      int pred_version = pred->version.load();
      int curr_version = curr->version.load();
      
      typename M::scoped_lock predLock, currLock;
      acquireOrderedLocks(pred, curr, predLock, currLock);
      
      if (validate(pred, curr, pred_version, curr_version)) {
        return (curr->key == k);
      }
    }
  }

  void insert(T k) {
    while (true) {
      Entry *pred = &staticHead;
      Entry *curr = pred->next;
      while (curr->key < k) {
        pred = curr;
        curr = curr->next;
      }

      int prev_version = pred->version.load();
      int curr_version = curr->version.load();
      
      typename M::scoped_lock predLock, currLock;
      acquireOrderedLocks(pred, curr, predLock, currLock);
      
      if (!validate(pred, curr, pred_version, curr_version)) {
        continue;
      }

      if (curr->key==k) {
        return;
      }

      Entry *n = new Entry(k, curr, M());
      pred->next = n;

      pred->version.fetch_add(1);

      return;

    }
  }

  void remove(T k) {
    while (true) {
      Entry *pred = &staticHead;
      Entry *curr = pred->next;
      while (curr->key < k) {
        pred = curr;
        curr = curr->next;
      }

      int prev_version = prev->version.load();
      int curr_version = curr->version.load();
      
      typename M::scoped_lock predLock, currLock;
      acquireOrderedLocks(pred, curr, predLock, currLock);
      

      if (!validate(pred, curr, prev_version, curr_version)) {
        continue;
      }

      if (curr->key != key) {
        break;
      }

      prev->next = curr->next;
      delete curr;
      prev->version.fetch_add(1)o

      return;
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
    Entry *next;
    std::atomic<int> version{0};
    M mutex;
  };

  Entry staticHead;
  Entry staticTail;

  ListBasedSetOptimisticLockCoupling() {
    staticHead.key = std::numeric_limits<T>::min();
    staticHead.next = &staticTail;
    staticTail.key = std::numeric_limits<T>::max();
    staticTail.next = nullptr;
  }

  bool contains(T k) {
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

#endif // HW5_SYNCHRONIZATION_HPP