#pragma once
#include "gpu_index.hpp"
#include "csr_graph.hpp"
#include <vector>

namespace axiomgraph {

class GraphEngine {
public:
    GraphEngine(const CSRGraph& graph, GPUIndex& index);
    
    // Finds nearest vectors, then traverses 'hops' deep in the CSR graph
    // Returns unique NodeIDs in the local context
    std::vector<NodeID> search_with_context(const float* query, int top_k, int hops);

private:
    const CSRGraph& graph_;
    GPUIndex& index_;
};

} // namespace axiomgraph
