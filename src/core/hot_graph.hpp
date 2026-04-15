#pragma once
#include "memory_arena.hpp"
#include <vector>
#include <shared_mutex>
#include <string>

namespace axiomgraph {

struct Edge {
    NodeID target;
    float weight;
    uint32_t type_hash;
};

class HotGraph {
public:
    HotGraph() = default;
    ~HotGraph() = default;

    void add_edge(NodeID source, NodeID target, float weight, uint32_t type_hash);
    void remove_node(NodeID source);
    void clear();
    std::vector<Edge> get_neighbors(NodeID source) const;
    size_t size() const;

    const std::vector<std::vector<Edge>>& get_adjacency_list() const;
    std::shared_mutex& get_rw_mutex() const;

    void save(const std::string& path) const;
    void load(const std::string& path);

private:
    std::vector<std::vector<Edge>> adjacency_list_;
    mutable std::shared_mutex resize_mutex_;
    static constexpr size_t NUM_STRIPES = 1024;
    mutable std::shared_mutex stripe_mutexes_[NUM_STRIPES];
};

} // namespace axiomgraph
