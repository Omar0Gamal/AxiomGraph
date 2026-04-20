#pragma once
#include "hot_graph.hpp"
#include <vector>
#include <string>

namespace axiomgraph {

class CSRGraph {
public:
    CSRGraph() = default;
    ~CSRGraph() = default;

    void build_from_hot(const HotGraph& hot);

    std::vector<NodeID> get_neighbors(NodeID source) const;

    const std::vector<NodeID>& row_ptr() const { return row_ptr_; }
    const std::vector<NodeID>& col_ind() const { return col_ind_; }
    const std::vector<float>& weights() const { return weights_; }
    const std::vector<uint32_t>& type_hashes() const { return type_hashes_; }

    void save(const std::string& path) const;
    void load(const std::string& path);

private:
    std::vector<NodeID> row_ptr_;
    std::vector<NodeID> col_ind_;
    std::vector<float> weights_;
    std::vector<uint32_t> type_hashes_;
};

} // namespace axiomgraph
