#include "hot_graph.hpp"
#include <fstream>
#include <stdexcept>
#include <algorithm>

namespace axiomgraph {

void HotGraph::add_edge(NodeID source, NodeID target, float weight, uint32_t type_hash) {
    {
        std::shared_lock<std::shared_mutex> lock(resize_mutex_);
        if (source < adjacency_list_.size()) {
            std::unique_lock<std::shared_mutex> stripe_lock(stripe_mutexes_[source % NUM_STRIPES]);
            adjacency_list_[source].push_back({target, weight, type_hash});
            return;
        }
    }
    std::unique_lock<std::shared_mutex> lock(resize_mutex_);
    if (source >= adjacency_list_.size()) {
        adjacency_list_.resize(source + 1);
    }
    std::unique_lock<std::shared_mutex> stripe_lock(stripe_mutexes_[source % NUM_STRIPES]);
    adjacency_list_[source].push_back({target, weight, type_hash});
}

void HotGraph::remove_node(NodeID source) {
    std::shared_lock<std::shared_mutex> lock(resize_mutex_);
    if (source < adjacency_list_.size()) {
        std::unique_lock<std::shared_mutex> stripe_lock(stripe_mutexes_[source % NUM_STRIPES]);
        adjacency_list_[source].clear();
    }
    // Removing incoming edges from all other nodes
    for (size_t i = 0; i < adjacency_list_.size(); ++i) {
        std::unique_lock<std::shared_mutex> stripe_lock(stripe_mutexes_[i % NUM_STRIPES]);
        auto& edges = adjacency_list_[i];
        edges.erase(std::remove_if(edges.begin(), edges.end(),
            [source](const Edge& e) { return e.target == source; }), edges.end());
    }
}

void HotGraph::clear() {
    std::unique_lock<std::shared_mutex> lock(resize_mutex_);
    for (size_t i = 0; i < NUM_STRIPES; ++i) {
        std::unique_lock<std::shared_mutex> stripe_lock(stripe_mutexes_[i]);
    }
    adjacency_list_.clear();
}

std::vector<Edge> HotGraph::get_neighbors(NodeID source) const {
    std::shared_lock<std::shared_mutex> lock(resize_mutex_);
    if (source < adjacency_list_.size()) {
        std::shared_lock<std::shared_mutex> stripe_lock(stripe_mutexes_[source % NUM_STRIPES]);
        return adjacency_list_[source];
    }
    return {};
}

size_t HotGraph::size() const {
    std::shared_lock<std::shared_mutex> lock(resize_mutex_);
    return adjacency_list_.size();
}

const std::vector<std::vector<Edge>>& HotGraph::get_adjacency_list() const {
    return adjacency_list_;
}

std::shared_mutex& HotGraph::get_rw_mutex() const {
    return resize_mutex_;
}

void HotGraph::save(const std::string& path) const {
    std::shared_lock<std::shared_mutex> lock(resize_mutex_);
    for (size_t i = 0; i < NUM_STRIPES; ++i) {
        std::shared_lock<std::shared_mutex> stripe_lock(stripe_mutexes_[i]);
    }
    std::ofstream os(path, std::ios::binary);
    if (!os) throw std::runtime_error("Cannot open file for writing: " + path);
    size_t num_nodes = adjacency_list_.size();
    os.write(reinterpret_cast<const char*>(&num_nodes), sizeof(num_nodes));
    for (const auto& edges : adjacency_list_) {
        size_t num_edges = edges.size();
        os.write(reinterpret_cast<const char*>(&num_edges), sizeof(num_edges));
        if (num_edges > 0) {
            os.write(reinterpret_cast<const char*>(edges.data()), num_edges * sizeof(Edge));
        }
    }
}

void HotGraph::load(const std::string& path) {
    std::unique_lock<std::shared_mutex> lock(resize_mutex_);
    for (size_t i = 0; i < NUM_STRIPES; ++i) {
        std::unique_lock<std::shared_mutex> stripe_lock(stripe_mutexes_[i]);
    }
    std::ifstream is(path, std::ios::binary);
    if (!is) throw std::runtime_error("Cannot open file for reading: " + path);
    size_t num_nodes = 0;
    is.read(reinterpret_cast<char*>(&num_nodes), sizeof(num_nodes));
    adjacency_list_.resize(num_nodes);
    for (size_t i = 0; i < num_nodes; ++i) {
        size_t num_edges = 0;
        is.read(reinterpret_cast<char*>(&num_edges), sizeof(num_edges));
        adjacency_list_[i].resize(num_edges);
        if (num_edges > 0) {
            is.read(reinterpret_cast<char*>(adjacency_list_[i].data()), num_edges * sizeof(Edge));
        }
    }
}

} // namespace axiomgraph
