#pragma once
#include "vector_buffer.hpp"
#include <vector>
#include <string>
#include <memory>

namespace axiomgraph {

struct CagraConfig {
    int graph_degree = 32;
    int intermediate_graph_degree = 64;
};

struct VamanaConfig {
    int graph_degree = 32;
};

class GPUIndex {
public:
    GPUIndex();
    ~GPUIndex();

    void build_cagra_index(const VectorBuffer& vectors, const CagraConfig& config);
    void build_vamana_index(const VectorBuffer& vectors, const VamanaConfig& config);
    
    // Returns indices of top_k results.
    std::vector<NodeID> search(const float* query, int top_k, const std::vector<NodeID>& allowed_ids = {});

    void save(const std::string& path) const;
    void load(const std::string& path);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace axiomgraph
