#include "graph_engine.hpp"
#include <unordered_set>

namespace axiomgraph {

GraphEngine::GraphEngine(const CSRGraph& graph, GPUIndex& index) 
    : graph_(graph), index_(index) {}

std::vector<NodeID> GraphEngine::search_with_context(const float* query, int top_k, int hops) {
    auto initial_results = index_.search(query, top_k);
    
    std::unordered_set<NodeID> context_nodes(initial_results.begin(), initial_results.end());
    std::vector<NodeID> current_frontier = initial_results;
    
    for (int h = 0; h < hops; ++h) {
        std::vector<NodeID> next_frontier;
        for (NodeID node : current_frontier) {
            auto neighbors = graph_.get_neighbors(node);
            for (NodeID nbr : neighbors) {
                if (context_nodes.insert(nbr).second) {
                    next_frontier.push_back(nbr);
                }
            }
        }
        current_frontier = std::move(next_frontier);
    }
    
    return std::vector<NodeID>(context_nodes.begin(), context_nodes.end());
}

} // namespace axiomgraph
