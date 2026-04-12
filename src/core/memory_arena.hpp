#pragma once
#include <cstdint>
#include <queue>
#include <vector>
#include <atomic>
#include <mutex>

namespace axiomgraph {

using NodeID = uint32_t;

class MemoryArena {
public:
    MemoryArena();
    ~MemoryArena() = default;

    NodeID allocate_id();
    void free_id(NodeID id);
    size_t active_count() const;
    size_t capacity() const;

private:
    mutable std::atomic_flag lock_ = ATOMIC_FLAG_INIT;
    NodeID next_id_ = 0;
    std::vector<NodeID> free_list_;
    size_t active_nodes_;
};

} // namespace axiomgraph
