#include "cpu_index.hpp"
#include "hnswlib/hnswlib/hnswlib.h"
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <iostream>

namespace axiomgraph {

CPUIndex::CPUIndex() {}

CPUIndex::~CPUIndex() {
    if (index_) delete index_;
    if (space_) delete space_;
}

void CPUIndex::build_index(const VectorBuffer& vectors) {
    if (index_) delete index_;
    if (space_) delete space_;

    dim_ = vectors.dimensions();
    max_elements_ = vectors.size();
    if (max_elements_ == 0) return;

    space_ = new hnswlib::L2Space(dim_);
    index_ = new hnswlib::HierarchicalNSW<float>(space_, max_elements_, 16, 200);

    const float* data = vectors.data();
    for (size_t i = 0; i < max_elements_; ++i) {
        index_->addPoint(data + i * dim_, i);
    }
    is_built_ = true;
}

class FilterID : public hnswlib::BaseFilterFunctor {
    const std::vector<NodeID>& allowed_;
public:
    FilterID(const std::vector<NodeID>& allowed) : allowed_(allowed) {}
    bool operator()(hnswlib::labeltype label_id) override {
        return std::binary_search(allowed_.begin(), allowed_.end(), static_cast<NodeID>(label_id));
    }
};

std::vector<NodeID> CPUIndex::search(const VectorBuffer& vectors, const float* query, int top_k, const std::vector<NodeID>& allowed_ids) const {
    if (!is_built_ || !index_) throw std::runtime_error("CPU Index not built");

    std::vector<NodeID> sorted_allowed = allowed_ids;
    std::sort(sorted_allowed.begin(), sorted_allowed.end());
    
    FilterID filter(sorted_allowed);
    hnswlib::BaseFilterFunctor* filter_ptr = allowed_ids.empty() ? nullptr : &filter;

    auto result = index_->searchKnn(query, top_k, filter_ptr);
    
    std::vector<NodeID> results;
    results.reserve(result.size());
    while (!result.empty()) {
        results.push_back(static_cast<NodeID>(result.top().second));
        result.pop();
    }
    std::reverse(results.begin(), results.end());
    return results;
}

void CPUIndex::save(const std::string& path) const {
    std::ofstream os(path + ".meta", std::ios::binary);
    if (!os) throw std::runtime_error("Cannot write CPU index meta");
    os.write(reinterpret_cast<const char*>(&is_built_), sizeof(is_built_));
    os.write(reinterpret_cast<const char*>(&dim_), sizeof(dim_));
    os.write(reinterpret_cast<const char*>(&max_elements_), sizeof(max_elements_));
    os.close();

    if (is_built_ && index_) {
        index_->saveIndex(path + ".hnsw");
    }
}

void CPUIndex::load(const std::string& path) {
    std::ifstream is(path + ".meta", std::ios::binary);
    if (!is) throw std::runtime_error("Cannot read CPU index meta");
    is.read(reinterpret_cast<char*>(&is_built_), sizeof(is_built_));
    is.read(reinterpret_cast<char*>(&dim_), sizeof(dim_));
    is.read(reinterpret_cast<char*>(&max_elements_), sizeof(max_elements_));
    is.close();

    if (is_built_) {
        if (index_) delete index_;
        if (space_) delete space_;
        space_ = new hnswlib::L2Space(dim_);
        index_ = new hnswlib::HierarchicalNSW<float>(space_, path + ".hnsw");
    }
}

} // namespace axiomgraph
