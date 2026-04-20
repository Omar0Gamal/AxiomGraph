#include "csr_graph.hpp"
#include <shared_mutex>
#include <fstream>
#include <stdexcept>

namespace axiomgraph {

void CSRGraph::build_from_hot(const HotGraph& hot) {
    const auto& adj_list = hot.get_adjacency_list();
    std::shared_lock<std::shared_mutex> lock(hot.get_rw_mutex());

    size_t num_nodes = adj_list.size();
    size_t num_edges = 0;
    for (const auto& edges : adj_list) {
        num_edges += edges.size();
    }

    row_ptr_.clear();
    col_ind_.clear();
    weights_.clear();
    type_hashes_.clear();

    row_ptr_.reserve(num_nodes + 1);
    col_ind_.reserve(num_edges);
    weights_.reserve(num_edges);
    type_hashes_.reserve(num_edges);

    NodeID current_edge_idx = 0;
    for (size_t i = 0; i < num_nodes; ++i) {
        row_ptr_.push_back(current_edge_idx);
        for (const auto& edge : adj_list[i]) {
            col_ind_.push_back(edge.target);
            weights_.push_back(edge.weight);
            type_hashes_.push_back(edge.type_hash);
            current_edge_idx++;
        }
    }
    row_ptr_.push_back(current_edge_idx);
}

std::vector<NodeID> CSRGraph::get_neighbors(NodeID source) const {
    std::vector<NodeID> neighbors;
    if (source + 1 < row_ptr_.size()) {
        NodeID start = row_ptr_[source];
        NodeID end = row_ptr_[source + 1];
        neighbors.reserve(end - start);
        for (NodeID i = start; i < end; ++i) {
            neighbors.push_back(col_ind_[i]);
        }
    }
    return neighbors;
}

void CSRGraph::save(const std::string& path) const {
    std::ofstream os(path, std::ios::binary);
    if (!os) throw std::runtime_error("Cannot open file for writing: " + path);
    
    size_t rows = row_ptr_.size();
    size_t cols = col_ind_.size();
    
    os.write(reinterpret_cast<const char*>(&rows), sizeof(rows));
    os.write(reinterpret_cast<const char*>(&cols), sizeof(cols));
    
    if (rows > 0) os.write(reinterpret_cast<const char*>(row_ptr_.data()), rows * sizeof(NodeID));
    if (cols > 0) {
        os.write(reinterpret_cast<const char*>(col_ind_.data()), cols * sizeof(NodeID));
        os.write(reinterpret_cast<const char*>(weights_.data()), cols * sizeof(float));
        os.write(reinterpret_cast<const char*>(type_hashes_.data()), cols * sizeof(uint32_t));
    }
}

void CSRGraph::load(const std::string& path) {
    std::ifstream is(path, std::ios::binary);
    if (!is) throw std::runtime_error("Cannot open file for reading: " + path);
    
    size_t rows = 0, cols = 0;
    is.read(reinterpret_cast<char*>(&rows), sizeof(rows));
    is.read(reinterpret_cast<char*>(&cols), sizeof(cols));
    
    row_ptr_.resize(rows);
    col_ind_.resize(cols);
    weights_.resize(cols);
    type_hashes_.resize(cols);
    
    if (rows > 0) is.read(reinterpret_cast<char*>(row_ptr_.data()), rows * sizeof(NodeID));
    if (cols > 0) {
        is.read(reinterpret_cast<char*>(col_ind_.data()), cols * sizeof(NodeID));
        is.read(reinterpret_cast<char*>(weights_.data()), cols * sizeof(float));
        is.read(reinterpret_cast<char*>(type_hashes_.data()), cols * sizeof(uint32_t));
    }
}

} // namespace axiomgraph
