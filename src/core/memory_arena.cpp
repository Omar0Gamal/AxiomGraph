#include "memory_arena.hpp"
#include <atomic>
#include <vector>

namespace axiomgraph {

MemoryArena::MemoryArena() : next_id_(0), active_nodes_(0) {}

class SpinLockGuard {
    std::atomic_flag& flag_;
public:
    SpinLockGuard(std::atomic_flag& flag) : flag_(flag) {
        while (flag_.test_and_set(std::memory_order_acquire)) {}
    }
    ~SpinLockGuard() {
        flag_.clear(std::memory_order_release);
    }
};

NodeID MemoryArena::allocate_id() {
    SpinLockGuard lock(lock_);
    NodeID id;
    if (!free_list_.empty()) {
        id = free_list_.back();
        free_list_.pop_back();
    } else {
        id = next_id_++;
    }
    active_nodes_++;
    return id;
}

void MemoryArena::free_id(NodeID id) {
    SpinLockGuard lock(lock_);
    free_list_.push_back(id);
    active_nodes_--;
}

size_t MemoryArena::active_count() const {
    SpinLockGuard lock(lock_);
    return active_nodes_;
}

size_t MemoryArena::capacity() const {
    SpinLockGuard lock(lock_);
    return next_id_;
}

} // namespace axiomgraph
