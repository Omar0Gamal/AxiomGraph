#pragma once
#include "vector_buffer.hpp"
#include <vector>
#include <string>

namespace hnswlib {
    template<typename dist_t> class HierarchicalNSW;
    class L2Space;
}

namespace axiomgraph {

class CPUIndex {
public:
    CPUIndex();
    ~CPUIndex();

    void build_index(const VectorBuffer& vectors);
    std::vector<NodeID> search(const VectorBuffer& vectors, const float* query, int top_k, const std::vector<NodeID>& allowed_ids) const;

    void save(const std::string& path) const;
    void load(const std::string& path);

private:
    bool is_built_ = false;
    size_t dim_ = 0;
    size_t max_elements_ = 0;
    hnswlib::L2Space* space_ = nullptr;
    hnswlib::HierarchicalNSW<float>* index_ = nullptr;
};

} // namespace axiomgraph
